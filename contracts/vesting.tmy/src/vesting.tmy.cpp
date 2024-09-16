#include <vesting.tmy/vesting.tmy.hpp>

namespace vestingtoken
{
    void check_asset(const eosio::asset &asset)
    {
        auto sym = asset.symbol;
        eosio::check(sym.is_valid(), "invalid amount symbol");
        eosio::check(sym == vestingToken::system_resource_currency, "Symbol does not match system resource currency");
        eosio::check(sym.precision() == vestingToken::system_resource_currency.precision(), "Symbol precision does not match");
        eosio::check(asset.amount > 0, "Amount must be greater than 0");
    }

    void vestingToken::setsettings(string sales_date_str, string launch_date_str)
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
        // Check if the provided category exists in the map
        eosio::check(vesting_categories.contains(category_id), "Invalid vesting category");
        // Check if the category is not in the list of depreciated categories
        eosio::check(!depreciated_categories.contains(category_id), "Category is depreciated");

        // Check the symbol is correct and valid
        check_asset(amount);

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
            if (num_rows >= MAX_ALLOCATIONS)
            {
                eosio::check(false, "Too many purchases received on this account.");
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
            row.id = vesting_table.available_primary_key();
            row.holder = holder;
            row.tokens_allocated = amount;
            row.tokens_claimed = eosio::asset(0, amount.symbol);
            row.time_since_sale_start = time_since_sale_start;
            row.vesting_category_type = category_id; });

        eosio::require_recipient(holder);

        eosio::action({sender, "active"_n},
                      token_contract_name,
                      "transfer"_n,
                      std::make_tuple(sender, get_self(), amount, std::string("Allocated vested funds")))
            .send(); // This will also run eosio::require_auth(sender)
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
        eosio::check(now >= launch_date, "Launch date not yet reached");

        int64_t total_claimable = 0;
        for (auto iter = vesting_table.begin(); iter != vesting_table.end(); ++iter)
        {
            const vested_allocation &vesting_allocation = *iter;

            vesting_category category = vesting_categories.at(vesting_allocation.vesting_category_type);

            time_point vesting_start = launch_date + category.start_delay;
            time_point cliff_finished = vesting_start + category.cliff_period;

            // Calculate the vesting end time
            time_point vesting_end = vesting_start + category.vesting_period;

            // Check if vesting period after cliff has started
            if (now >= cliff_finished)
            {
                // Calculate the total claimable amount
                int64_t claimable = 0;
                if (now >= vesting_end)
                {
                    claimable = vesting_allocation.tokens_allocated.amount;
                }
                else
                {
                    double vesting_finished = static_cast<double>((now - vesting_start).count()) / category.vesting_period.count();
                    claimable = vesting_allocation.tokens_allocated.amount * std::min((vesting_finished + category.tge_unlock), 1.0);
                }

                total_claimable += claimable - vesting_allocation.tokens_claimed.amount;

                // Update the tokens_claimed field
                eosio::asset tokens_claimed = eosio::asset(claimable, vesting_allocation.tokens_claimed.symbol);
                vesting_table.modify(iter, get_self(), [&](auto &row)
                                     { row.tokens_claimed = tokens_claimed; });
            }
        }

        if (total_claimable > 0)
        {
            // Transfer the tokens to the holder
            eosio::asset total_tokens_claimed = eosio::asset(total_claimable, system_resource_currency);
            eosio::action({get_self(), "active"_n},
                          token_contract_name,
                          "transfer"_n,
                          std::make_tuple(get_self(), holder, total_tokens_claimed, std::string("Unlocked vested coins")))
                .send();
        }
    }

    // Migrates an allocation to a new amount and category
    void vestingToken::migratealloc(eosio::name sender, name holder, uint64_t allocation_id, eosio::asset old_amount, eosio::asset new_amount, int old_category_id, int new_category_id)
    {
        require_auth(get_self());

        // Check if the provided category exists in the map
        eosio::check(vesting_categories.contains(new_category_id), "Invalid vesting category");

        // Check the symbol is correct and valid
        check_asset(new_amount);

        // Get the vesting allocations
        vesting_allocations vesting_table(get_self(), holder.value);
        auto iter = vesting_table.find(allocation_id);
        eosio::check(iter != vesting_table.end(), "Allocation not found");

        // Check the new category is not in the list of depreciated categories
        eosio::check(depreciated_categories.at(new_category_id), "Category is depreciated");

        // Check the old amount and category match the existing allocation
        eosio::check(iter->tokens_allocated == old_amount, "Old amount does not match existing allocation");
        eosio::check(iter->vesting_category_type == old_category_id, "Old category does not match existing allocation");

        // Calculate the change in the allocation amount
        int64_t amount_change = new_amount.amount - iter->tokens_allocated.amount;

        // Modify the table row data, and update the table
        vesting_table.modify(iter, get_self(), [&](auto &row)
                             {
            row.tokens_allocated = new_amount;
            row.vesting_category_type = new_category_id; });

        // Notify the holder
        eosio::require_recipient(holder);

        // If new tokens were allocated, then send them to the contract
        if (amount_change > 0)
        {
            eosio::action({sender, "active"_n},
                          token_contract_name,
                          "transfer"_n,
                          std::make_tuple(sender, get_self(), amount_change, std::string("Allocated vested funds")))
                .send();
        }
        // If tokens were removed, send them back to the sender
        else if (amount_change < 0)
        {
            eosio::action({get_self(), "active"_n},
                          token_contract_name,
                          "transfer"_n,
                          std::make_tuple(get_self(), sender, -amount_change, std::string("Refunded vested funds")))
                .send();
        }
    }
}