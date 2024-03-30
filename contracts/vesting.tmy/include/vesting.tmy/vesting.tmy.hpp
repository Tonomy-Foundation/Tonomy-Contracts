// vesting.tmy.hpp

#pragma once

#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

namespace vestingtoken
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

    struct vesting_category
    {
        microseconds cliff_period;
        microseconds start_delay;
        microseconds vesting_period;
    };

    static const std::map<int, vesting_category> vesting_categories = {
        {1, {days(6 * 30), days(0 * 30), days(2 * 365)}}, // Seed Private Sale,
        // {2, {6 * 30 * MICROSECONDS_IN_DAY, 6 * 30 * MICROSECONDS_IN_DAY, 2 * 365 * MICROSECONDS_IN_DAY}},  // Strategic Partnerships Private Sale,
        // {3, {0 * 30 * MICROSECONDS_IN_DAY, 0 * 30 * MICROSECONDS_IN_DAY, 0 * 30 * MICROSECONDS_IN_DAY}},   // Public Sale (DO NOT USED YET),
        // {4, {0 * 30 * MICROSECONDS_IN_DAY, 1 * 365 * MICROSECONDS_IN_DAY, 5 * 365 * MICROSECONDS_IN_DAY}}, // Team and Advisors, Ecosystem
        // {5, {0 * 30 * MICROSECONDS_IN_DAY, 0 * 30 * MICROSECONDS_IN_DAY, 1 * 365 * MICROSECONDS_IN_DAY}},  // Legal and Compliance
        // {6, {0 * 30 * MICROSECONDS_IN_DAY, 0 * 30 * MICROSECONDS_IN_DAY, 2 * 365 * MICROSECONDS_IN_DAY}},  // Reserves, Partnerships, Liquidly Allocation
        // {7, {0 * 30 * MICROSECONDS_IN_DAY, 0 * 30 * MICROSECONDS_IN_DAY, 5 * 365 * MICROSECONDS_IN_DAY}},  // Community and Marketing, Platform Dev, Infra Rewards

        {999, {eosio::seconds(10), eosio::seconds(10), eosio::seconds(20)}}, // TESTING ONLY
    };

    class [[eosio::contract("vesting.tmy")]] vestingToken : public eosio::contract
    {
    public:
        using contract::contract;
        static constexpr eosio::symbol system_resource_currency = eosio::symbol("LEOS", 6);
        static constexpr eosio::name token_contract_name = "eosio.token"_n;

        struct [[eosio::table]] vesting_settings
        {
            eosio::time_point sales_start_date;
            eosio::time_point launch_date;

            EOSLIB_SERIALIZE(vesting_settings, (sales_start_date)(launch_date))
        };

        typedef eosio::singleton<"settings"_n, vesting_settings> settings_table;
        // Following line needed to correctly generate ABI. See https://github.com/EOSIO/eosio.cdt/issues/280#issuecomment-439666574
        typedef eosio::multi_index<"settings"_n, vesting_settings> settings_table_dump;

        // Define the structure of a vesting schedule
        struct [[eosio::table]] vested_allocation
        {
            eosio::name holder;
            eosio::asset tokens_allocated;
            eosio::asset tokens_claimed;
            microseconds time_since_sale_start;
            int vesting_category_type;
            bool cliff_period_claimed;
            uint64_t primary_key() const
            {
                return time_since_sale_start.count();
            }
            EOSLIB_SERIALIZE(struct vested_allocation, (holder)(tokens_allocated)(tokens_claimed)(time_since_sale_start)(vesting_category_type)(cliff_period_claimed))
        };

        // Define the mapping of vesting schedules
        typedef eosio::multi_index<"allocation"_n, vested_allocation> vesting_allocations;

        /**
         * @details Updates the start date for vesting schedules to a new specified date
         *
         * @param sales_start_date {string} - The new start date for vesting schedules.
         * @param launch_date {string} - The new start date for vesting schedules.
         * @details
         * Before any allocations can be executed, the start date should be set using this action.
         * If the launch date is not known when the sale starts, set it to a long time in the future.
         *
         * Example of the string format expected: "2024-04-01T24:00:00"
         */
        [[eosio::action]] void updatedate(string sales_start_date, string launch_date);

        /**
         * @details Assigns tokens to a holder with a specified vesting category.
         *
         * @param sender {name} - The account name of the sender who is assigning the tokens.
         * @param holder {name} - The account name of the token holder.
         * @param amount {asset} - The amount of tokens to be assigned.
         * @param category {integer} - The vesting category for the assigned tokens.
         */
        [[eosio::action]] void assigntokens(eosio::name sender, eosio::name holder, eosio::asset amount, int category);

        /**
         * @details Allows a holder to withdraw vested tokens if the vesting conditions are met.
         *
         * @param holder {name} - The account name of the token holder.
         */
        [[eosio::action]] void withdraw(eosio::name holder);

        using updatedate_action = action_wrapper<"updatedate"_n, &vestingToken::updatedate>;
        using assigntokens_action = action_wrapper<"assigntokens"_n, &vestingToken::assigntokens>;
        using withdraw_action = action_wrapper<"withdraw"_n, &vestingToken::withdraw>;
    };
}
