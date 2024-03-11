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

    typedef eosio::singleton<"startdate"_n, eosio::time_point_sec> startDate;
    struct VestingCategory {
        int cliffPeriodDays;
        int startDelayDays;
        int vestingPeriodDays;
    };

    std::map<int, VestingCategory> vestingCategories = {
        {1, {6*30, 0, 12*30}}, // private sale #1
        {2, {0, 3*30, 24*30}}  // team & Ecosystem
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
            eosio::asset totalTokens;
            eosio::asset tokensClaimed;
            eosio::time_point_sec allocated;
            VestingCategory vestingCategory;
            bool cliffPeriodClaimed;
            uint64_t primary_key() const { return holder.value; }
            EOSLIB_SERIALIZE(struct vested_allocation, (holder)(totalTokens)
            (tokensClaimed)(allocated)(vestingCategory)(cliffPeriodClaimed))
        };

        // Define the mapping of vesting schedules
        typedef eosio::multi_index<"vestschedule"_n, vested_allocation> vesting_schedules;

        /**
        * @details Updates the start date for vesting schedules to a new specified date
        *
        * @param newStartDate - The new start date for vesting schedules.
        */
        [[eosio::action]]
        void updatedate(eosio::time_point_sec newStartDate);

        /**
        * @details Assigns tokens to a holder with a specified vesting category.
        *
        * @param holder - The account name of the token holder.
        * @param amount - The amount of tokens to be assigned.
        * @param category - The vesting category for the assigned tokens.
        */
        [[eosio::action]]
        void assigntokens(eosio::name holder, eosio::asset amount, VestingCategory category);

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
