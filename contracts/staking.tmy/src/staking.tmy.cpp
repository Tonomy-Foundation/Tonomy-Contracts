#include <staking.tmy/staking.tmy.hpp>

namespace stakingtoken
{
   void stakingToken::staketokens(name account_name, asset quantity) {
      require_auth(account_name);
      eosio::check(quantity.amount > 0, "Stake quantity must be greater than zero.");
      stakingToken::staking_allocations _staking(get_self(), get_self().value);

      _staking.emplace(account_name, [&](auto& row) {
         row.id = _staking.available_primary_key();
         row.account_name = account_name;
         row.tokens_staked = quantity;
         row.stake_time = eosio::current_time_point();
         row.unstake_time = eosio::time_point_sec(0); // not unstaked yet
         row.unstake_requested = false;
      });

      // Transfer tokens to the contract
      eosio::action(
         eosio::permission_level{account_name, "active"_n},
         token_contract_name,
         "transfer"_n,
         std::make_tuple(account_name, get_self(), quantity, std::string("stake tokens"))
      ).send();
   }

   void stakingToken::requnstake(name account_name, uint64_t allocation_id) {
      require_auth(account_name);

      staking_allocations _staking(get_self(), get_self().value);
      auto itr = _staking.find(allocation_id);
      check(itr != _staking.end(), "Staking allocation not found");
      check(itr->account_name == account_name, "Not authorized to unstake this allocation");
      check(!itr->unstake_requested, "Unstake already requested");

      _staking.modify(itr, eosio::same_payer, [&](auto& row) {
         row.unstake_requested = true;
         row.unstake_time = eosio::current_time_point() + eosio::days(5);
      });
   }

   void stakingToken::releasetoken(name account_name, uint64_t allocation_id) {
      require_auth(account_name);

      staking_allocations _staking(get_self(), get_self().value);
      auto itr = _staking.find(allocation_id);
      check(itr != _staking.end(), "Staking allocation not found");
      check(itr->account_name == account_name, "Not authorized to finalize unstake for this allocation");
      check(itr->unstake_requested, "Unstake not requested");
      check(eosio::current_time_point() >= itr->unstake_time, "Unstaking period not yet completed");

      // Transfer tokens back to the account
      eosio::action(
         eosio::permission_level{get_self(), "active"_n},
         token_contract_name,
         "transfer"_n,
         std::make_tuple(get_self(), account_name, itr->tokens_staked, std::string("unstake tokens"))
      ).send();

      _staking.erase(itr);
   }

}