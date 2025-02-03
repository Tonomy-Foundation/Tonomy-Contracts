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
        static constexpr eosio::symbol system_resource_currency = eosio::symbol("LEOS", 6);
        static constexpr eosio::name token_contract_name = "eosio.token"_n;
        static const uint32_t MAX_ALLOCATIONS = 100;
        eosio::microseconds LOCKUP_PERIOD = eosio::days(30);
        eosio::microseconds RELEASE_PERIOD = eosio::days(5);
        
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
          eosio::name staker; // The account name of the staker.
          eosio::asset tokens_staked; //The amount of tokens staked.
          eosio::time_point stake_time; //The time when the staking started.
          eosio::time_point unstake_time; //The time when the unstaking will occur.
          bool unstake_requested; //A flag indicating whether the tokens are currently being unstaked.
          uint64_t primary_key() const { return id; }
          EOSLIB_SERIALIZE(struct staking_allocation, (id)(staker)(tokens_staked)(stake_time)(unstake_time)(unstake_requested))
        };
        // Define the mapping of staking allocations
        typedef eosio::multi_index<"stakingalloc"_n, staking_allocation> staking_allocations;

        struct [[eosio::table]] staking_account
        {
          eosio::name staker; // The account name of the staker.
          eosio::asset total_yield; //The total amount of yield ever received
          int version; // The version of the staking allocation
          uint64_t primary_key() const { return staker.value; }
          EOSLIB_SERIALIZE(struct staking_account, (staker)(total_yield)(version))
        };
        // Define the mapping of staking accounts
        typedef eosio::multi_index<"stakingaccou"_n, staking_account> staking_accounts;

        struct [[eosio::table]] staking_settings
        {
            eosio::asset total_staked; // The total amount of tokens staked.
            eosio::asset total_releasing; // The total amount of tokens being unstaked.
            
            EOSLIB_SERIALIZE(staking_settings, (total_staked)(total_releasing))
        };

        typedef eosio::singleton<"settings"_n, staking_settings> settings_table;
        // Following line needed to correctly generate ABI. See https://github.com/EOSIO/eosio.cdt/issues/280#issuecomment-439666574
        typedef eosio::multi_index<"settings"_n, staking_settings> settings_table_dump;

        using staketokens_action = action_wrapper<"staketokens"_n, &stakingToken::staketokens>;
        using requnstake_action = action_wrapper<"requnstake"_n, &stakingToken::requnstake>;
        using releasetoken_action = action_wrapper<"releasetoken"_n, &stakingToken::releasetoken>;
    };
}
