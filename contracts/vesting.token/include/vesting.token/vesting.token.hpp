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
    struct vesting_category  {
        uint32_t  cliff_period_days;
        uint32_t  start_delay_days;
        uint32_t  vesting_period_days;
    };

    static const std::map<int, vesting_category> vesting_categories = {
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
            eosio::asset total_allocated;
            eosio::asset tokens_claimed;
            eosio::time_point_sec allocated;
            vesting_category vesting_category_type;
            bool cliff_period_claimed;
            uint64_t primary_key() const {
                return allocated.sec_since_epoch();
            }
            EOSLIB_SERIALIZE(struct vested_allocation, (holder)(total_allocated)
            (tokens_claimed)(allocated)(vesting_category_type)(cliff_period_claimed))
        };

        // Define the mapping of vesting schedules
        typedef eosio::multi_index<"allocation"_n, vested_allocation> vesting_allocations;

        /**
        * @details Updates the start date for vesting schedules to a new specified date
        *
        * @param new_starte_date - The new start date for vesting schedules.
        */
        [[eosio::action]]
        void updatedate(eosio::time_point_sec new_starte_date);

        /**
        * @details Assigns tokens to a holder with a specified vesting category.
        *
        * @param holder - The account name of the token holder.
        * @param amount - The amount of tokens to be assigned.
        * @param category - The vesting category for the assigned tokens.
        */
        [[eosio::action]]
        void assigntokens(eosio::name holder, eosio::asset amount, vesting_category category);

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
