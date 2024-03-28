// vestng.token.hpp

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
    using eosio::name;
    using eosio::singleton;
    using std::string;

    struct vesting_settings
    {
        eosio::time_point_sec sales_start_date;
        eosio::time_point_sec launch_date;

        EOSLIB_SERIALIZE(vesting_settings, (sales_start_date)(launch_date))
    };

    typedef eosio::singleton<"settings"_n, vesting_settings> settings;

    static const uint32_t SECONDS_IN_DAY = 24 * 60 * 60;

    struct vesting_category
    {
        uint32_t cliff_period_seconds;
        uint32_t start_delay_seconds;
        uint32_t vesting_period_seconds;
    };

    static const std::map<int, vesting_category> vesting_categories = {
        {1, {6 * 30 * SECONDS_IN_DAY, 0 * 30 * SECONDS_IN_DAY, 2 * 365 * SECONDS_IN_DAY}},  // Seed Private Sale,
        {2, {6 * 30 * SECONDS_IN_DAY, 6 * 30 * SECONDS_IN_DAY, 2 * 365 * SECONDS_IN_DAY}},  // Strategic Partnerships Private Sale,
        {3, {0 * 30 * SECONDS_IN_DAY, 0 * 30 * SECONDS_IN_DAY, 0 * 30 * SECONDS_IN_DAY}},   // Public Sale (DO NOT USED YET),
        {4, {0 * 30 * SECONDS_IN_DAY, 1 * 365 * SECONDS_IN_DAY, 5 * 365 * SECONDS_IN_DAY}}, // Team, Ecosystem
        {5, {0 * 30 * SECONDS_IN_DAY, 0 * 30 * SECONDS_IN_DAY, 1 * 365 * SECONDS_IN_DAY}},  // Legal and Compliance
        {6, {0 * 30 * SECONDS_IN_DAY, 0 * 30 * SECONDS_IN_DAY, 2 * 365 * SECONDS_IN_DAY}},  // Reserves, Partnerships, Liquidly Allocation
        {7, {0 * 30 * SECONDS_IN_DAY, 0 * 30 * SECONDS_IN_DAY, 5 * 365 * SECONDS_IN_DAY}},  // Community and Marketing, Platform Dev, Infra Rewards

        {999, {10, 10, 20}}, // TESTING ONLY
    };

    class [[eosio::contract("vestng.token")]] vestingToken : public eosio::contract
    {
    public:
        using contract::contract;
        static constexpr eosio::symbol system_resource_currency = eosio::symbol("LEOS", 6);
        static constexpr eosio::name token_contract_name = "eosio.token"_n;

        // Define the structure of a vesting schedule
        struct [[eosio::table]] vested_allocation
        {
            eosio::name holder;
            eosio::asset total_allocated;
            eosio::asset tokens_claimed;
            uint32_t seconds_since_sales_start;
            vesting_category vesting_category_type;
            bool cliff_period_claimed;
            uint64_t primary_key() const
            {
                return seconds_since_sales_start;
            }
            EOSLIB_SERIALIZE(struct vested_allocation, (holder)(total_allocated)(tokens_claimed)(seconds_since_sales_start)(vesting_category_type)(cliff_period_claimed))
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
