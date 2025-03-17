// staking.tmy.hpp

#pragma once

#include <math.h>
#include <cmath>
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
        static constexpr eosio::symbol SYSTEM_RESOURCE_CURRENCY = eosio::symbol("LEOS", 6);
        static constexpr eosio::name TOKEN_CONTRACT = "eosio.token"_n;
        static constexpr eosio::name SYSTEM_CONTRACT = "eosio"_n;
        #ifdef BUILD_TEST
          static const uint8_t MAX_ALLOCATIONS = 5;
          // Lockup period is how long the tokens are locked up for before they can be unstaked
          eosio::microseconds LOCKUP_PERIOD = eosio::seconds(10);
          // Release period is how long the unstaking process takes before the tokens are released
          eosio::microseconds RELEASE_PERIOD = eosio::seconds(5);
          // Cron period is how often the cron job is called. This should be the same as the period in eosio.tonomy.hpp
          const int64_t CRON_PERIOD_MICROSECONDS = eosio::seconds(10).count(); // should correspond the the same in eosio.tonomy.hpp
          // Staking cycle is how often the staking yield is distributed per account.
          const int64_t STAKING_CYCLE_MICROSECONDS = eosio::seconds(60).count();
          // Minimum transfer amount for DOS protection
          const asset MINIMUM_TRANSFER = asset(1 * std::pow(10, SYSTEM_RESOURCE_CURRENCY.precision()), SYSTEM_RESOURCE_CURRENCY); // 1 LEOS
        #else
          static const uint8_t MAX_ALLOCATIONS = 20;
          // Lockup period is how long the tokens are locked up for before they can be unstaked
          eosio::microseconds LOCKUP_PERIOD = eosio::days(14);
          // Release period is how long the unstaking process takes before the tokens are released
          eosio::microseconds RELEASE_PERIOD = eosio::days(5);
          // Cron period is how often the cron job is called. This should be the same as the period in eosio.tonomy.hpp
          const int64_t CRON_PERIOD_MICROSECONDS = eosio::hours(1).count();
          // Staking cycle is how often the staking yield is distributed per account.
          const int64_t STAKING_CYCLE_MICROSECONDS = eosio::hours(24).count();
          // Minimum transfer amount for DOS protection
          const asset MINIMUM_TRANSFER = asset(1000 * std::pow(10, SYSTEM_RESOURCE_CURRENCY.precision()), SYSTEM_RESOURCE_CURRENCY); // 1000 LEOS
        #endif
        // TODO: make sure that this does not need to be rounded otherwise will create a bug. if the cycle is 17 seconds and the period is 3 seconds there. 3 does not fit into 17 exactly
        const uint8_t cron_intervals = STAKING_CYCLE_MICROSECONDS / CRON_PERIOD_MICROSECONDS;
        // Annual Percentage Yield for staking
        static constexpr double MAX_APY = 1.0; // 100% APY
        const double MICROSECONDS_PER_YEAR = 365.25 * 24 * 60 * 60 * 1000000;
        static constexpr uint64_t LOWEST_PERSON_NAME  = ("p1111111111"_n).value;
        static constexpr uint64_t HIGHEST_PERSON_NAME  = ("pzzzzzzzzzz"_n).value;    

        /**
         * Sets the settings
         */
        [[eosio::action]] void setsettings(asset yearly_stake_pool);

        /**
         * Adds new tokens available for yield
         */
        [[eosio::action]] void addyield(name sender, asset quantity);

        /**
        * Stake tokens for 14 days
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

        /**
         * Cron job to be called every hour to distribute yield to stakers
         */
        [[eosio::action]] void cron();

        #ifdef BUILD_TEST
        /**
         * Resets all the contract data
         * For testing purposes only
         */
        [[eosio::action]] void resetall();
        #endif
        
        struct [[eosio::table]] staking_settings
      {
          eosio::asset current_yield_pool; // The amount of tokens available for staking yield each month.
          eosio::asset yearly_stake_pool; // The amount of tokens that should be available for staking yield each month.
          eosio::asset total_staked; // The total amount of tokens staked.
          eosio::asset total_releasing; // The total amount of tokens being unstaked.
          
          EOSLIB_SERIALIZE(staking_settings, (current_yield_pool)(yearly_stake_pool)(total_staked)(total_releasing))
      };

      typedef eosio::singleton<"settings"_n, staking_settings> settings_table;
      // Following line needed to correctly generate ABI. See https://github.com/EOSIO/eosio.cdt/issues/280#issuecomment-439666574
      typedef eosio::multi_index<"settings"_n, staking_settings> settings_table_dump;

     // Define the structure of a staking allocation
     struct [[eosio::table]] staking_allocation
     {
       uint64_t id;
       eosio::asset initial_stake; // The amount of tokens initially staked.
       eosio::asset tokens_staked; //The amount of tokens staked.
       eosio::time_point stake_time; //The time when the staking started.
       eosio::time_point unstake_time; //The time when the unstaking will occur.
       bool unstake_requested; //A flag indicating whether the tokens are currently being unstaked.
       uint64_t primary_key() const { return id; }
       EOSLIB_SERIALIZE(struct staking_allocation, (id)(initial_stake)(tokens_staked)(stake_time)(unstake_time)(unstake_requested))
     };
     // Define the mapping of staking allocations
     typedef eosio::multi_index<"stakingalloc"_n, staking_allocation> staking_allocations;

        struct [[eosio::table]] staking_account
        {
          eosio::name staker; // The account name of the staker.
          eosio::asset total_yield; //The total amount of yield ever received
          eosio::time_point last_payout; //The time of the last yield payout
          uint32_t payments; // The number of payments made to the account. TODO: this field is not strictly needed so could be optimized out of code. It is pretty handy to understand the cron job though...
          int version; // The version of the staking allocation
          uint64_t primary_key() const { return staker.value; }
          EOSLIB_SERIALIZE(struct staking_account, (staker)(total_yield)(last_payout)(payments)(version))
        };
        // Define the mapping of staking accounts
        typedef eosio::multi_index<"stakingaccou"_n, staking_account> staking_accounts;

      

        using staketokens_action = action_wrapper<"staketokens"_n, &stakingToken::staketokens>;
        using requnstake_action = action_wrapper<"requnstake"_n, &stakingToken::requnstake>;
        using releasetoken_action = action_wrapper<"releasetoken"_n, &stakingToken::releasetoken>;

      private:
        /**
         * Add yield to an account
         */
        void create_account_yield(time_point now, const name &staker, double apy, staking_settings &settings, staking_accounts::const_iterator accounts_itr);
      
        /**
         * Check minimum amount needed to prevent DOSing the action
         */
        void check_minimum_asset_prevent_dos(const asset &compare_to);

        /**
          *  Releases staked tokens back to the staker.
        */
        void _releasetoken(const name &staker, staking_settings &settings, staking_allocations &staking_allocations_table, staking_allocations::const_iterator allocation);
    };
}