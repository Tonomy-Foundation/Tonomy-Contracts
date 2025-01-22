// staking.tmy.hpp

#pragma once

#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

namespace stakingtoken
{
    using eosio::action_wrapper;
    using eosio::asset;
    using eosio::check;
    using eosio::days;
    using eosio::microseconds;
    using eosio::name;
    using eosio::singleton;
    using eosio::time_point;
    using std::string;

    class [[eosio::contract("staking.tmy")]] stakingToken : public eosio::contract
    {
    public:
        using contract::contract;
        static constexpr eosio::name token_contract_name = "eosio.token"_n;
        
        /**
        * Stake tokens for 30 days
        *
        * @param account_name - the account name of the staker
        * @param quantity - the amount of tokens to stake
        */
        [[eosio::action]] void staketokens(name account_name, asset quantity);

        /**
        * Request unstaking, starts a 5-day unstaking period
        *
        * @param account_name - the account name of the staker
        * @param allocation_id - the ID of the staking allocation
        */
        [[eosio::action]] void requnstake(name account_name, uint64_t allocation_id);

        /**
        * Finalize the unstaking process after 5 days
        *
        * @param account_name - the account name of the staker
        * @param allocation_id - the ID of the staking allocation
        */
        [[eosio::action]] void releasetoken(name account_name, uint64_t allocation_id);

          // Define the structure of a staking allocation
        struct [[eosio::table]] staking_allocation
        {
          uint64_t id;
          eosio::name account_name; // The account name of the staker.
          eosio::asset tokens_staked; //The amount of tokens staked.
          eosio::time_point stake_time; //The time when the staking started.
          eosio::time_point unstake_time; //The time when the unstaking will occur.
          bool unstake_requested; //A flag indicating whether the tokens are currently being unstaked.
          uint64_t primary_key() const { return id; }
          EOSLIB_SERIALIZE(struct staking_allocation, (id)(account_name)(tokens_staked)(stake_time)(unstake_time)(unstake_requested))
        };

        // Define the mapping of staking allocations
        typedef eosio::multi_index<"stakingalloc"_n, staking_allocation> staking_allocations;

        using staketokens_action = action_wrapper<"staketokens"_n, &stakingToken::staketokens>;
        using requnstake_action = action_wrapper<"requnstake"_n, &stakingToken::requnstake>;
        using releasetoken_action = action_wrapper<"releasetoken"_n, &stakingToken::releasetoken>;
    };
}
