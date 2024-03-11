#include <vesting.token/vesting.token.hpp>

namespace vestingtoken {
    void vestingToken::updatedate(eosio::time_point_sec newStartDate) {
        require_auth(get_self());
        startDate startDate(get_self(), get_self().value);
        startDate.set(newStartDate, get_self());
    }
    void vestingToken::assigntokens(eosio::name holder, eosio::asset amount, VestingCategory category) {
        // Only the contract owner can call this function
        require_auth(get_self());

        // Check the symbol is correct and valid
        auto sym = amount.symbol;
        eosio::check(sym == system_resource_currency, "Symbol does not match system resource currency");
        eosio::check(sym.precision() == system_resource_currency.precision(), "Symbol precision does not match");
        eosio::check(amount.amount > 0, "Amount must be greater than 0");

        // Create a new vesting schedule
        vesting_schedules vestingTable(get_self(), holder.value);
        auto iter = vestingTable.end();

        vestingTable.emplace(get_self(), [&](auto& row) {
            row.holder = holder;
            row.totalTokens = amount;
            row.tokensClaimed = asset(0, amount.symbol);
            row.allocated = eosio::current_time_point();
            row.vestingCategory = category;
        });
    }
    void vestingToken::withdraw(eosio::name holder){
        require_auth(holder);
        // Get the vesting schedule
        vesting_schedules vestingTable(get_self(), holder.value);
        for (auto iter = vestingTable.begin(); iter != vestingTable.end(); ++iter) {
            const vested_allocation& vestingSchedule = *iter;

            VestingCategory category = vestingSchedule.vestingCategory;

            eosio::time_point_sec startDelayDays = eosio::time_point_sec(category.startDelayDays * 24 * 60 * 60);
            eosio::time_point_sec startDateValue = startDate(get_self(), get_self().value).get();
            eosio::time_point_sec vesting_start = startDateValue + eosio::seconds(startDelayDays.sec_since_epoch()) + vestingSchedule.allocated;


            // Calculate the tokens that can be claimed
            if (eosio::current_time_point() > eosio::time_point(vesting_start)) {
                eosio::asset cliffTokens = vestingSchedule.totalTokens * category.cliffPeriodDays /
                                        category.vestingPeriodDays;
                
                eosio::microseconds microseconds_elapsed = eosio::current_time_point() - vesting_start;
                double days_elapsed = microseconds_elapsed.count() / 1000000.0 / 60 / 60 / 24;
                eosio::asset claimable = (vestingSchedule.totalTokens - cliffTokens) * days_elapsed / category.vestingPeriodDays;

                if (!vestingSchedule.cliffPeriodClaimed) {
                    claimable += cliffTokens;
                }

                vestingTable.modify(iter, get_self(), [&](auto& row) {
                    row.tokensClaimed += claimable;
                    row.cliffPeriodClaimed = true;
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