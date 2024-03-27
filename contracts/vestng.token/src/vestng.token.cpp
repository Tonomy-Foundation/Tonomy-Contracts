#include <vestng.token/vestng.token.hpp>

namespace vestingtoken
{

    void vestingToken::updatedate(string sales_date_str, string launch_date_str)
    {
        require_auth(get_self());

        vestingtoken::vesting_settings dates;
        dates.sales_start_date = eosio::time_point_sec::from_iso_string(sales_date_str);
        dates.launch_date = eosio::time_point_sec::from_iso_string(launch_date_str);

        vestingtoken::settings launch_sales_dates_singleton(get_self(), get_self().value);
        launch_sales_dates_singleton.set(dates, get_self());
    }

    void vestingToken::assigntokens(eosio::name sender, eosio::name holder, eosio::asset amount, int category_id)
    {
        // Only the contract owner can call this function
        require_auth(get_self());

        // Check if the provided category exists in the map
        auto category_iter = vesting_categories.find(category_id);
        eosio::check(category_iter != vesting_categories.end(), "Invalid vesting category");

        // If the category exists, you can access it like this:
        vesting_category category = category_iter->second;

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
            if (num_rows > 20)
            {
                eosio::check(false, "Cannot purchase tokens more than 20 times.");
            }
        }

        // Calculate the number of seconds since sales start
        uint32_t now_since_epoch = eosio::current_time_point().sec_since_epoch();
        vestingtoken::settings launch_sales_dates_singleton(get_self(), get_self().value);

        vestingtoken::vesting_settings dates = launch_sales_dates_singleton.get();
        uint32_t sales_start_date_since_epoch = dates.sales_start_date.sec_since_epoch();

        check(now_since_epoch >= sales_start_date_since_epoch, "Sale has not yet started");

        uint32_t allocated_after_sales_start_seconds = 0;
        allocated_after_sales_start_seconds = now_since_epoch - sales_start_date_since_epoch;

        vesting_table.emplace(get_self(), [&](auto &row)
                              {
            row.holder = holder;
            row.total_allocated = amount;
            row.tokens_claimed = asset(0, amount.symbol);
            row.seconds_since_sales_start = allocated_after_sales_start_seconds;
            row.vesting_category_type = category;
            row.cliff_period_claimed = false; });

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
        uint32_t now_since_epoch = eosio::time_point_sec(eosio::current_time_point()).sec_since_epoch();

        vestingtoken::settings launch_sales_dates_singleton(get_self(), get_self().value);
        vestingtoken::vesting_settings dates = launch_sales_dates_singleton.get();
        uint32_t launch_date_since_epoch = dates.launch_date.sec_since_epoch();

        for (auto iter = vesting_table.begin(); iter != vesting_table.end(); ++iter)
        {
            const vested_allocation &vesting_allocation = *iter;

            vesting_category category = vesting_allocation.vesting_category_type;

            uint32_t vesting_start_since_epoch = launch_date_since_epoch + vesting_allocation.seconds_since_sales_start + category.start_delay_seconds;
            uint32_t cliff_end_since_epoch = vesting_start_since_epoch + category.cliff_period_seconds;

            // Calculate the vesting end time
            uint32_t vesting_end_time_since_epoch = vesting_start_since_epoch + category.vesting_period_seconds;

            // Check if vesting period after cliff has started
            eosio::check(now_since_epoch >= cliff_end_since_epoch, "Vesting period after cliff has not started");

            // Calculate the total claimable amount
            int64_t claimable = 0;
            if (now_since_epoch >= vesting_end_time_since_epoch)
            {
                claimable = vesting_allocation.total_allocated.amount;
            }
            else
            {
                claimable = vesting_allocation.total_allocated.amount * (now_since_epoch - vesting_start_since_epoch) / category.vesting_period_seconds;
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