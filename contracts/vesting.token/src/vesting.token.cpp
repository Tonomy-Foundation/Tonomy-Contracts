#include <vesting.token/vesting.token.hpp>

namespace vestingtoken {

    void vestingToken::updatedate(eosio::time_point sales_date, eosio::time_point launch_date) {
        require_auth(get_self());
        sales_start_date = sales_date;
        launch_date = launch_date;
    }

    void vestingToken::assigntokens(eosio::name holder, eosio::asset amount, int category_id) {
        // Only the contract owner can call this function
        require_auth(get_self());
        
        // Check if the provided category exists in the map
        auto category_iter = vesting_categories.find(category_id);
        eosio::check(category_iter != vesting_categories.end(), "Invalid vesting category");

        // If the category exists, you can access it like this:
        vesting_category category = category_iter->second;


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
            ++num_rows;
            if (num_rows > 20) {
                eosio::check(false, "Cannot purchase tokens more than 20 times.");
            }
        }

        // Calculate the number of seconds since sales start
        const eosio::time_point_sec now = eosio::current_time_point();
        int64_t allocated_after_sales_start_seconds = now.sec_since_epoch() - sales_start_date.sec_since_epoch();

        vesting_table.emplace(get_self(), [&](auto& row) {
            row.holder = holder;
            row.total_allocated = amount;
            row.tokens_claimed = asset(0, amount.symbol);
            row.allocated = allocated_after_sales_start_seconds;
            row.vesting_category_type = category;
        });
    }
    void vestingToken::withdraw(eosio::name holder) {
        require_auth(holder);

        // Get the vesting allocations
        vesting_allocations vesting_table(get_self(), holder.value);
        const eosio::time_point_sec now = eosio::current_time_point();

        for (auto iter = vesting_table.begin(); iter != vesting_table.end(); ++iter) {
            const vested_allocation& vesting_allocation = *iter;

            vesting_category category = vesting_allocation.vesting_category_type;

            uint32_t vesting_start_since_epoch = launch_date.sec_since_epoch() + vesting_allocation.allocated + (category.start_delay_days * SECONDS_IN_DAY);
            uint32_t cliff_end_since_epoch = vesting_start_since_epoch + (category.cliff_period_days * SECONDS_IN_DAY);
            
            // Calculate the vesting end time
            uint32_t vesting_end_time_since_epoch = vesting_start_since_epoch + (category.vesting_period_days * SECONDS_IN_DAY);

            // Check if vesting period after cliff has started
            eosio::check(now.sec_since_epoch() >= cliff_end_since_epoch, "Vesting period after cliff has not started");

            // Check if vesting period after cliff has started
            eosio::check(now.sec_since_epoch() >= vesting_end_time_since_epoch, "Vesting period after cliff has not started");

            eosio::asset cliff_tokens = vesting_allocation.total_allocated * category.cliff_period_days /
                                            category.vesting_period_days;

            uint32_t seconds_elapsed = now.sec_since_epoch() - vesting_start_since_epoch;
            eosio::asset claimable = (vesting_allocation.total_allocated - cliff_tokens) * seconds_elapsed / category.vesting_period_days;

            if (!vesting_allocation.cliff_period_claimed) {
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