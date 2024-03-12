#include <vesting.token/vesting.token.hpp>

namespace vestingtoken {

    void vestingToken::updatedate(eosio::time_point_sec new_start_date) {
        require_auth(get_self());
        start_date start_date(get_self(), get_self().value);
        start_date.set(new_start_date, get_self());
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

        // Check the number of rows for the user
        uint32_t num_rows = 0;
        for (auto itr = vesting_table.begin(); itr != vesting_table.end(); ++itr) {
            if (itr->holder == holder) {
                ++num_rows;
                if (num_rows > 20) {
                    eosio::check(false, "Cannot purchase tokens more than 20 times.");
                }
            }
        }

        vesting_table.emplace(get_self(), [&](auto& row) {
            row.holder = holder;
            row.total_allocated = amount;
            row.tokens_claimed = asset(0, amount.symbol);
            row.allocated = eosio::current_time_point();
            row.vesting_category_type = category;
        });
    }
    void vestingToken::withdraw(eosio::name holder) {
        require_auth(holder);

        // Get the vesting schedule
        vesting_allocations vesting_table(get_self(), holder.value);
        eosio::check(start_date(get_self(), get_self().value).exists(), "Start date not set");

        for (auto iter = vesting_table.begin(); iter != vesting_table.end(); ++iter) {
            const vested_allocation& vesting_schedule = *iter;

            vesting_category category = vesting_schedule.vesting_category_type;

            eosio::time_point_sec start_delay_seconds = eosio::time_point_sec(category.start_delay_days * SECONDS_IN_DAY);
            eosio::time_point_sec start_date_value = start_date(get_self(), get_self().value).get();
            eosio::time_point_sec vesting_start = start_date_value + start_delay_seconds.sec_since_epoch() + vesting_schedule.allocated.sec_since_epoch();

            // Calculate the cliff end time
            eosio::time_point_sec cliff_end_time = vesting_start + eosio::time_point_sec(category.cliff_period_days * SECONDS_IN_DAY).sec_since_epoch();

            // Calculate the vesting end time
            eosio::time_point_sec vesting_end_time = vesting_start + eosio::time_point_sec(category.vesting_period_days * SECONDS_IN_DAY).sec_since_epoch();

            // Calculate the tokens that can be claimed
            const auto current_time = eosio::current_time_point().sec_since_epoch();
            eosio::time_point_sec now = eosio::time_point_sec(current_time / 1000);


            // Check if vesting period after cliff has started
            eosio::check(now >= cliff_end_time, "Vesting period after cliff has not started");

            // Check if vesting period after cliff has started
            eosio::check(now >= vesting_end_time, "Vesting period after cliff has not started");

            if (now > vesting_start) {
                eosio::asset cliff_tokens = vesting_schedule.total_allocated * category.cliff_period_days /
                                            category.vesting_period_days;

                eosio::time_point_sec seconds_elapsed = now - vesting_start.sec_since_epoch();

                double seconds_elapsed_double = static_cast<double>(seconds_elapsed.sec_since_epoch());
                eosio::asset claimable = (vesting_schedule.total_allocated - cliff_tokens) * seconds_elapsed_double / category.vesting_period_days;

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