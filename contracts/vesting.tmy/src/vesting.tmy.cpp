#include <vesting.tmy/vesting.tmy.hpp>

namespace vestingtoken
{

    void vestingToken::updatedate(string sales_date_str, string launch_date_str)
    {
        require_auth(get_self());

        vesting_settings settings;
        settings.sales_start_date = eosio::time_point::from_iso_string(sales_date_str);
        settings.launch_date = eosio::time_point::from_iso_string(launch_date_str);

        settings_table settings_table_instance(get_self(), get_self().value);
        settings_table_instance.set(settings, get_self());
    }

    void vestingToken::assigntokens(eosio::name sender, eosio::name holder, eosio::asset amount, int category_id)
    {
        // Only the contract owner can call this function
        require_auth(get_self());

        // Check if the provided category exists in the map
        eosio::check(vesting_categories.contains(category_id), "Invalid vesting category");

        // Check the symbol is correct and valid
        auto sym = amount.symbol;
        eosio::check(sym.is_valid(), "invalid amount symbol");
        eosio::check(sym == system_resource_currency, "Symbol does not match system resource currency");
        eosio::check(sym.precision() == system_resource_currency.precision(), "Symbol precision does not match");
        eosio::check(amount.amount > 0, "Amount must be greater than 0");

        // Create a new vesting schedule
        vesting_allocations vesting_table(get_self(), holder.value);

        // Check the number of rows for the user
        uint32_t num_rows = 0;
        for (auto itr = vesting_table.begin(); itr != vesting_table.end(); ++itr)
        {
            ++num_rows;
            // Prevent unbounded array iteration DoS. If too many rows are added to the table, the user
            // may no longer be able to withdraw from the account.
            // For more information, see https://swcregistry.io/docs/SWC-128/
            if (num_rows > 150)
            {
                eosio::check(false, "Cannot purchase tokens more than 150 times.");
            }
        }

        // Calculate the number of seconds since sales start
        time_point now = eosio::current_time_point();

        settings_table settings_table_instance(get_self(), get_self().value);
        vesting_settings settings = settings_table_instance.get();

        eosio::check(now >= settings.sales_start_date, "Sale has not yet started");

        microseconds time_since_sale_start = now - settings.sales_start_date;

        vesting_table.emplace(get_self(), [&](auto &row)
                              {
            row.holder = holder;
            row.tokens_allocated = amount;
            row.tokens_claimed = eosio::asset(0, amount.symbol);
            row.time_since_sale_start = time_since_sale_start;
            row.vesting_category_type = category_id;
            row.cliff_period_claimed = false; });

        eosio::require_recipient(holder);

        eosio::action({get_self(), "active"_n},
                      token_contract_name,
                      "transfer"_n,
                      std::make_tuple(sender, get_self(), amount, std::string("Allocated vested funds")))
            .send();
    }

    void vestingToken::withdraw(eosio::name holder)
    {
        require_auth(holder);

        // Get the vesting allocations
        vesting_allocations vesting_table(get_self(), holder.value);
        time_point now = eosio::current_time_point();

        settings_table settings_table_instance(get_self(), get_self().value);
        vesting_settings settings = settings_table_instance.get();

        time_point launch_date = settings.launch_date;

        for (auto iter = vesting_table.begin(); iter != vesting_table.end(); ++iter)
        {
            const vested_allocation &vesting_allocation = *iter;

            vesting_category category = vesting_categories.at(vesting_allocation.vesting_category_type);

            time_point vesting_start = launch_date + vesting_allocation.time_since_sale_start + category.start_delay;
            time_point cliff_end = vesting_start + category.cliff_period;

            // Calculate the vesting end time
            time_point vesting_end = vesting_start + category.vesting_period;

            // Check if vesting period after cliff has started
            eosio::check(now >= cliff_end, "Vesting period after cliff has not started");

            // Print for debugging
            eosio::print("{\"sales_start_date\":\"", settings.sales_start_date.to_string(), "\"");
            eosio::print(",\"launch_date\":\"", launch_date.to_string(), "\"");
            eosio::print(",\"now\":\"", now.to_string(), "\"");
            eosio::print(",\"vesting_start\":\"", vesting_start.to_string(), "\"");
            eosio::print(",\"cliff_end\":\"", cliff_end.to_string(), "\"");
            eosio::print(",\"vesting_end\":\"", vesting_end.to_string(), "\"");
            eosio::print(",\"vesting_finished\":", static_cast<double>((now - vesting_start).count()) / category.vesting_period.count());
            eosio::print("}");

            // Calculate the total claimable amount
            int64_t claimable = 0;
            if (now >= vesting_end)
            {
                claimable = vesting_allocation.tokens_allocated.amount;
            }
            else
            {
                double vesting_finished = static_cast<double>((now - vesting_start).count()) / category.vesting_period.count();
                claimable = vesting_allocation.tokens_allocated.amount * vesting_finished;
            }

            claimable -= vesting_allocation.tokens_claimed.amount;
            eosio::asset tokens_claimed = eosio::asset(claimable, vesting_allocation.tokens_claimed.symbol);

            // Update the tokens_claimed field
            vesting_table.modify(iter, get_self(), [&](auto &row)
                                 {
                row.tokens_claimed += tokens_claimed;
                row.cliff_period_claimed = true; });

            // Transfer the tokens to the holder
            eosio::action({get_self(), "active"_n},
                          token_contract_name,
                          "transfer"_n,
                          std::make_tuple(get_self(), holder, tokens_claimed, std::string("Unlocked vested coins")))
                .send();
        }
    }
}