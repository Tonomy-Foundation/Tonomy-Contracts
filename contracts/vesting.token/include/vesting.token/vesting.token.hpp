// vesting.token.hpp

#pragma once

#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

namespace vestingtoken {
    using eosio::action_wrapper;
    using eosio::asset;
    using eosio::name;
    using std::string;
    using eosio::check;

    struct launch_and_sales_dates {
        eosio::time_point_sec sales_start_date;
        eosio::time_point_sec launch_date;

        EOSLIB_SERIALIZE(launch_and_sales_dates, (sales_start_date)(launch_date))
    };

    typedef eosio::singleton<"launchsales"_n, launch_and_sales_dates> launch_sales_dates;

    static const uint32_t SECONDS_IN_DAY = 24*60*60;

    struct vesting_category  {
        uint32_t  cliff_period_seconds;
        uint32_t  start_delay_seconds;
        uint32_t  vesting_period_seconds;
    };

    static const std::map<int, vesting_category> vesting_categories = {
        {1, {6*30*SECONDS_IN_DAY, 0, 12*30*SECONDS_IN_DAY}}, // private sale #1
        {2, {0, 3*30*SECONDS_IN_DAY, 24*30*SECONDS_IN_DAY}}  // team & Ecosystem
        // Add other categories as needed
    };

    class [[eosio::contract("vesting.token")]] vestingToken : public eosio::contract {
    public:
        using contract::contract;
        static constexpr eosio::symbol system_resource_currency = eosio::symbol("LEOS", 6);
        static constexpr eosio::name token_contract_name = "eosio.token"_n;

        // Define the structure of a vesting schedule
        struct [[eosio::table]] vested_allocation {
            eosio::name holder;
            eosio::asset total_allocated;
            eosio::asset tokens_claimed;
            uint32_t seconds_since_sales_start;
            vesting_category vesting_category_type;
            bool cliff_period_claimed;
            uint64_t primary_key() const {
                return seconds_since_sales_start;
            }
            EOSLIB_SERIALIZE(struct vested_allocation, (holder)(total_allocated)
            (tokens_claimed)(seconds_since_sales_start)(vesting_category_type)(cliff_period_claimed))
        };

        // Define the mapping of vesting schedules
        typedef eosio::multi_index<"allocation"_n, vested_allocation> vesting_allocations;

        /**
        * @details Updates the start date for vesting schedules to a new specified date
        *
        * @param sales_start_date - The new start date for vesting schedules.
        * @param launch_date - The new start date for vesting schedules.
        */
        [[eosio::action]]
        void updatedate(string sales_start_date, string launch_date);

        /**
        * @details Assigns tokens to a holder with a specified vesting category.
        *
        * @param holder - The account name of the token holder.
        * @param amount - The amount of tokens to be assigned.
        * @param category - The vesting category for the assigned tokens.
        */
        [[eosio::action]]
        void assigntokens(eosio::name holder, eosio::asset amount, int category);

        /**
        * @details Allows a holder to withdraw vested tokens if the vesting conditions are met.
        *
        * @param holder - The account name of the token holder.
        */
        [[eosio::action]]
        void withdraw(eosio::name holder);

        using updatedate_action = action_wrapper<"updatedate"_n, &vestingToken::updatedate>;
        using assigntokens_action = action_wrapper<"assigntokens"_n, &vestingToken::assigntokens>;
        using withdraw_action = action_wrapper<"withdraw"_n, &vestingToken::withdraw>;
    };
}
