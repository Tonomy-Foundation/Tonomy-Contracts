#include <staking.tmy/staking.tmy.hpp>

namespace stakingtoken
{
   void check_asset(const eosio::asset &asset)
   {
      auto sym = asset.symbol;
      eosio::check(sym.is_valid(), "invalid amount symbol");
      eosio::check(sym == stakingToken::system_resource_currency, "Symbol does not match system resource currency");
      eosio::check(sym.precision() == stakingToken::system_resource_currency.precision(), "Symbol precision does not match");
      eosio::check(asset.amount > 0, "Amount must be greater than 0");
   }

   void stakingToken::staketokens(name staker, asset quantity)
   {
      // eosio::require_auth(staker); // this is not needed as eosio.token::transfer checks the permission
      check_asset(quantity);

      stakingToken::staking_allocations staking_allocations_table(get_self(), staker.value);

      std::ptrdiff_t allocations_count = std::distance(staking_allocations_table.begin(), staking_allocations_table.end());
      eosio::check(allocations_count >= MAX_ALLOCATIONS, "Too many stakes received on this account.");

      staking_allocations_table.emplace(get_self(), [&](auto &row)
                       {
         row.id = staking_allocations_table.available_primary_key();
         row.staker = staker;
         row.tokens_staked = quantity;
         row.stake_time = eosio::current_time_point();
         row.unstake_time = eosio::time_point_sec(0); // not unstaked yet
         row.unstake_requested = false; });

      // Transfer tokens to the contract
      eosio::action(
          {staker, "active"_n},
          token_contract_name,
          "transfer"_n,
          std::make_tuple(staker, get_self(), quantity, std::string("stake tokens")))
          .send(); // This will also run eosio::require_auth(staker)
   }

   void stakingToken::requnstake(name staker, uint64_t allocation_id)
   {
      require_auth(staker);

      const eosio::time_point now = eosio::current_time_point();

      staking_allocations staking_allocations_table(get_self(), staker.value);
      auto itr = staking_allocations_table.find(allocation_id);
      check(itr != staking_allocations_table.end(), "Staking allocation not found");
      check(!itr->unstake_requested, "Unstake already requested");
      check(itr->stake_time + LOCKUP_PERIOD > now, "Tokens are still locked up");

      staking_allocations_table.modify(itr, eosio::same_payer, [&](auto &row)
      {
         row.unstake_requested = true;
         row.unstake_time = now; });
   }

   void stakingToken::releasetoken(name staker, uint64_t allocation_id)
   {
      require_auth(staker);

      staking_allocations staking_allocations_table(get_self(), staker.value);
      auto itr = staking_allocations_table.find(allocation_id);
      check(itr != staking_allocations_table.end(), "Staking allocation not found");
      check(itr->staker == staker, "Not authorized to finalize unstake for this allocation");
      check(itr->unstake_requested, "Unstake not requested");
      check(itr->unstake_time + RELEASE_PERIOD > eosio::current_time_point(), "Release period not yet completed");

      // Transfer tokens back to the account
      eosio::action(
          {get_self(), "active"_n},
          token_contract_name,
          "transfer"_n,
          std::make_tuple(get_self(), staker, itr->tokens_staked, std::string("unstake tokens")))
          .send(); // This will also run eosio::require_auth(get_self())

      staking_allocations_table.erase(itr);
   }

}