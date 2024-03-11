#include <vesting.token/vesting.token.hpp>

namespace vestingtoken {

    void vestingToken::updatedate(eosio::time_point_sec newStartDate) {
        require_auth(get_self());
        startDate startDate(get_self(), get_self().value);
        startDate.set(newStartDate, get_self());
    }

    void vestingToken::assigntokens(eosio::name holder, eosio::asset amount, vesting_category category) {
        // Only the contract owner can call this function
        require_auth(get_self());

        // Check the symbol is correct and valid
        auto sym = amount.symbol;
        eosio::check(sym == system_resource_currency, "Symbol does not match system resource currency");
        eosio::check(sym.precision() == system_resource_currency.precision(), "Symbol precision does not match");
        eosio::check(amount.amount > 0, "Amount must be greater than 0");

        // Create a new vesting schedule
        vesting_allocations vesting_table(get_self(), holder.value);
        auto iter = vesting_table.end();

        vesting_table.emplace(get_self(), [&](auto& row) {
            row.holder = holder;
            row.total_allocated = amount;
            row.tokens_claimed = asset(0, amount.symbol);
            row.allocated = eosio::current_time_point();
            row.vesting_category_type = category;
        });
    }

    void vestingToken::withdraw(eosio::name holder){
        require_auth(holder);
        // Get the vesting schedule
        vesting_allocations vesting_table(get_self(), holder.value);
        for (auto iter = vesting_table.begin(); iter != vesting_table.end(); ++iter) {
            const vested_allocation& vesting_schedule = *iter;

            vesting_category category = vesting_schedule.vesting_category_type;

            eosio::time_point_sec start_delay_seconds = eosio::time_point_sec(category.start_delay_days * 24 * 60 * 60);
            eosio::time_point_sec start_date_value = startDate(get_self(), get_self().value).get();
            eosio::time_point_sec vesting_start = start_date_value + eosio::seconds(start_delay_seconds.sec_since_epoch()) + vesting_schedule.allocated;
            
            // Calculate the tokens that can be claimed
            if (eosio::current_time_point() > eosio::time_point(vesting_start)) {
                eosio::asset cliff_tokens = vesting_schedule.total_allocated * category.cliff_period_days /
                                        category.vesting_period_days;
                
                eosio::microseconds microseconds_elapsed = eosio::current_time_point() - vesting_start;
                double days_elapsed = microseconds_elapsed.count() / 1000000.0 / 60 / 60 / 24;
                eosio::asset claimable = (vesting_schedule.total_allocated - cliff_tokens) * days_elapsed / category.vesting_period_days;

                if (!vesting_schedule.cliff_period_claimed) {
                    claimable += cliff_tokens;
                }

                vesting_table.modify(iter, get_self(), [&](auto& row) {
                    row.tokens_claimed += claimable;
                    row.cliff_period_claimed = true;
                });

                // Transfer the tokens to the holder
                eosio::action({get_self(), "active"_n},
                token_contract_name,
                "transfer"_n,
                std::make_tuple(get_self(), holder, claimable, std::string("Unlocked vested coins")))
                .send();
            }
        }
    }
}