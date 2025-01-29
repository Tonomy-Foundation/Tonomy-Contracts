#include <staking.tmy/staking.tmy.hpp>

namespace stakingtoken
{
   void stakingToken::staketokens(name staker, asset quantity)
   {
      // eosio::require_auth(staker); // this is not needed as eosio.token::transfer checks the permission

      eosio::check(quantity.amount > 0, "Stake quantity must be greater than zero.");
      stakingToken::staking_allocations _staking(get_self(), staker.value);

      _staking.emplace(get_self(), [&](auto &row)
                       {
         row.id = _staking.available_primary_key();
         row.account_name = staker;
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

      staking_allocations _staking(get_self(), staker.value);
      auto itr = _staking.find(allocation_id);
      check(itr != _staking.end(), "Staking allocation not found");
      check(itr->account_name == staker, "Not authorized to unstake this allocation");
      check(!itr->unstake_requested, "Unstake already requested");

      _staking.modify(itr, eosio::same_payer, [&](auto &row)
                      {
         row.unstake_requested = true;
         row.unstake_time = eosio::current_time_point() + eosio::days(5); });
   }

   void stakingToken::releasetoken(name staker, uint64_t allocation_id)
   {
      require_auth(staker);

      staking_allocations _staking(get_self(), staker.value);
      auto itr = _staking.find(allocation_id);
      check(itr != _staking.end(), "Staking allocation not found");
      check(itr->account_name == staker, "Not authorized to finalize unstake for this allocation");
      check(itr->unstake_requested, "Unstake not requested");
      check(eosio::current_time_point() >= itr->unstake_time, "Unstaking period not yet completed");

      // Transfer tokens back to the account
      eosio::action(
          {get_self(), "active"_n},
          token_contract_name,
          "transfer"_n,
          std::make_tuple(get_self(), staker, itr->tokens_staked, std::string("unstake tokens")))
          .send(); // This will also run eosio::require_auth(get_self())

      _staking.erase(itr);
   }

}