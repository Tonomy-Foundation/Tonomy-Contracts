#include <staking.tmy/staking.tmy.hpp>

namespace stakingtoken
{
   void check_asset(const asset &asset)
   {
      auto sym = asset.symbol;
      eosio::check(sym.is_valid(), "invalid amount symbol");
      eosio::check(sym == stakingToken::SYSTEM_RESOURCE_CURRENCY, "Symbol does not match system resource currency");
      eosio::check(sym.precision() == stakingToken::SYSTEM_RESOURCE_CURRENCY.precision(), "Symbol precision does not match");
      eosio::check(asset.amount > 0, "Amount must be greater than 0");
   }

   void stakingToken::check_minimum_asset_prevent_dos(const asset &compare_to)
   {
      eosio::check(compare_to.amount >= MINIMUM_TRANSFER.amount, "Amount must be greater than or equal to " + MINIMUM_TRANSFER.to_string());
   }

   void stakingToken::setsettings(asset yearly_stake_pool)
   {
      require_auth(get_self());
      check_asset(yearly_stake_pool);

      settings_table settings_table_instance(get_self(), get_self().value);
      staking_settings settings = settings_table_instance.get_or_default({
            asset(0, SYSTEM_RESOURCE_CURRENCY), // current_yield_pool
            asset(0, SYSTEM_RESOURCE_CURRENCY), // yearly_stake_pool
            asset(0, SYSTEM_RESOURCE_CURRENCY), // total_staked
            asset(0, SYSTEM_RESOURCE_CURRENCY)  // total_releasing
      });
      settings.yearly_stake_pool = yearly_stake_pool;
      settings_table_instance.set(settings, get_self());
   }

   void stakingToken::addyield(name sender, asset quantity)
   {
      check_asset(quantity);
      check_minimum_asset_prevent_dos(quantity);

      settings_table settings_table_instance(get_self(), get_self().value);
      staking_settings settings = settings_table_instance.get();
      settings.current_yield_pool += quantity;
      settings_table_instance.set(settings, get_self());

      // Transfer tokens to the contract
      eosio::action(
          {sender, "active"_n},
          TOKEN_CONTRACT,
          "transfer"_n,
          std::make_tuple(sender, get_self(), quantity, std::string("add yield")))
          .send(); // This will also run eosio::require_auth(sender)
   }

   void stakingToken::staketokens(name staker, asset quantity)
   {
      // eosio::require_auth(staker); // this is not needed as eosio.token::transfer checks the permission

      // check that the staker is a person account
      eosio::check(staker.value >= LOWEST_PERSON_NAME && staker.value <= HIGHEST_PERSON_NAME, "Invalid staker account");

      check_asset(quantity);
      check_minimum_asset_prevent_dos(quantity);

      stakingToken::staking_allocations staking_allocations_table(get_self(), staker.value);

      // Prevent unbounded array iteration DoS. If too many rows are added to the table, the user
      // may no longer be able to withdraw from the account.
      // For more information, see https://swcregistry.io/docs/SWC-128/
      std::ptrdiff_t allocations_count = std::distance(staking_allocations_table.begin(),staking_allocations_table.end());
      eosio::check(allocations_count < MAX_ALLOCATIONS, "Too many stakes received on this account");

      time_point now = eosio::current_time_point();

      // Add the staking allocation
      staking_allocations_table.emplace(get_self(), [&](auto &row)
                       {
         row.id = staking_allocations_table.available_primary_key();
         row.staker = staker;
         row.initial_stake = quantity;
         row.tokens_staked = quantity;
         row.stake_time = now;
         // row.unstake_time = unset as does not mean anything. this could be any value
         row.unstake_requested = false; });

      // Add the user to the accounts table if they are not already there
      stakingToken::staking_accounts staking_accounts_table(get_self(), get_self().value);

      auto itr = staking_accounts_table.find(staker.value);
      if (itr == staking_accounts_table.end())
      {
         staking_accounts_table.emplace(get_self(), [&](auto &row)
         {
            row.staker = staker;
            row.total_yield = asset(0, SYSTEM_RESOURCE_CURRENCY);
            row.last_payout = now;
            row.payments = 0;
            row.version = 1;
         });
      }

      // Update the total staked amount
      stakingToken::settings_table settings_table_instance(get_self(), get_self().value);
      staking_settings settings = settings_table_instance.get();
      settings.total_staked += quantity;
      settings_table_instance.set(settings, get_self());

      // Transfer tokens to the contract
      eosio::action(
          {staker, "active"_n},
          TOKEN_CONTRACT,
          "transfer"_n,
          std::make_tuple(staker, get_self(), quantity, std::string("stake tokens")))
          .send(); // This will also run eosio::require_auth(staker)
   }

   void stakingToken::requnstake(name staker, uint64_t allocation_id)
   {
      require_auth(staker);

      const time_point now = eosio::current_time_point();

      staking_allocations staking_allocations_table(get_self(), staker.value);
      auto itr = staking_allocations_table.find(allocation_id);
      check(itr != staking_allocations_table.end(), "Staking allocation not found");
      check(!itr->unstake_requested, "Unstake already requested");
      check(itr->stake_time + LOCKUP_PERIOD <= now, "Tokens are still locked up");

      staking_allocations_table.modify(itr, eosio::same_payer, [&](auto &row)
      {
         row.unstake_requested = true;
         row.unstake_time = now;
      });

      // Update the settings total staked and releasing amounts
      settings_table settings_table_instance(get_self(), get_self().value);
      staking_settings settings = settings_table_instance.get();
      settings.total_staked -= itr->tokens_staked;
      settings.total_releasing += itr->tokens_staked;
      settings_table_instance.set(settings, get_self());
   }

   void stakingToken::releasetoken(name staker, uint64_t allocation_id)
   {
      require_auth(staker);

      staking_allocations staking_allocations_table(get_self(), staker.value);
      auto itr = staking_allocations_table.find(allocation_id);
      check(itr != staking_allocations_table.end(), "Staking allocation not found");
      check(itr->staker == staker, "Not authorized to finalize unstake for this allocation");
      check(itr->unstake_requested, "Unstake not requested");
      check(itr->unstake_time + RELEASE_PERIOD <= eosio::current_time_point(), "Release period not yet completed");

      // Update the settings total staked and releasing amounts
      settings_table settings_table_instance(get_self(), get_self().value);
      staking_settings settings = settings_table_instance.get();
      settings.total_releasing -= itr->tokens_staked;
      settings_table_instance.set(settings, get_self());

      staking_allocations_table.erase(itr);

      // Transfer tokens back to the account
      eosio::action(
          {get_self(), "active"_n},
          TOKEN_CONTRACT,
          "transfer"_n,
          std::make_tuple(get_self(), staker, itr->tokens_staked, std::string("unstake tokens")))
          .send(); // This will also run eosio::require_auth(get_self())
   }

   void stakingToken::cron()
   {
      // If the sender is not the system contract, require auth
      // This is so that it can be run from the eosio::onblock() action as a cron
      // or alternatively be called manually if needed
      if (eosio::get_sender() != SYSTEM_CONTRACT) {
         require_auth(get_self());
      }

      staking_accounts staking_accounts_table(get_self(), get_self().value);
      
      // this is going to be called every hour of the day
      // for each hour, we will iterate through 1/24th of the stakers using the staker.value
      // this is to avoid hitting the CPU limit

      // Determine the interval of the cron cycle that we are up to
      const time_point now = eosio::current_time_point();
      uint8_t current_cron_interval = (now.time_since_epoch().count() % STAKING_CYCLE_MICROSECONDS) / CRON_PERIOD_MICROSECONDS;

      // Calculate the range of account names for this interval
      uint64_t range_size = (HIGHEST_PERSON_NAME - LOWEST_PERSON_NAME) / cron_intervals;
      uint64_t lower_bound = LOWEST_PERSON_NAME + (current_cron_interval * range_size);
      uint64_t upper_bound = (current_cron_interval == (cron_intervals-1)) ? HIGHEST_PERSON_NAME + 1 : (lower_bound + range_size);

      // Find the first account in the range
      auto itr = staking_accounts_table.lower_bound(lower_bound);

      uint64_t count = 0;
      while (itr != staking_accounts_table.end() && itr->staker.value < upper_bound)
      {
         // check did not call create_account_yield() since before the last cron period
         // If so then this batch of accounts has already been processed in this interval, so exit
         if (itr->last_payout + microseconds(1.01 * CRON_PERIOD_MICROSECONDS) > now) return;

         create_account_yield(itr->staker);
         count++;
         itr++;
      }

      eosio::print("Processed ", count, " staking accounts in batch ", current_cron_interval);
   }

   void stakingToken::create_account_yield(name staker)
   {
      stakingToken::staking_allocations staking_allocations_table(get_self(), staker.value);
      stakingToken::staking_accounts staking_accounts_table(get_self(), get_self().value);
      settings_table settings_table_instance(get_self(), get_self().value);
      staking_settings settings = settings_table_instance.get();

      auto accounts_itr = staking_accounts_table.find(staker.value);
      
      time_point now = eosio::current_time_point();
      microseconds since_last_payout = now - accounts_itr->last_payout;

      // Calculate the yield rate for the interval
      double apy = std::min(static_cast<double>(settings.yearly_stake_pool.amount) / static_cast<double>(settings.total_staked.amount), MAX_APY);
      double interval_percentage_yield = std::pow(1 + apy, static_cast<double>(since_last_payout.count()) / MICROSECONDS_PER_YEAR) - 1;
      asset total_yield = asset(0, SYSTEM_RESOURCE_CURRENCY);

      // Iterate through allocations and add yield (if not unstaking)
      for (auto itr = staking_allocations_table.begin(); itr != staking_allocations_table.end(); itr++)
      {
         if (!itr->unstake_requested)
         {
            asset yield = asset(static_cast<int64_t>(itr->tokens_staked.amount * interval_percentage_yield), SYSTEM_RESOURCE_CURRENCY);

            staking_allocations_table.modify(itr, eosio::same_payer, [&](auto &row)
            {
               row.tokens_staked += yield;
            });

            total_yield += yield;
         }
      }

      if (total_yield.amount != 0)
      {
         staking_accounts_table.modify(accounts_itr, eosio::same_payer, [&](auto &row)
         {
            row.total_yield += total_yield;
            row.last_payout = now;
            row.payments = accounts_itr->payments + 1;
         });

         settings.total_staked += total_yield;
         settings.current_yield_pool -= total_yield;
         settings_table_instance.set(settings, get_self());   
      }
   }

   #ifdef BUILD_TEST
   void stakingToken::resetall()
   {
      require_auth(get_self());

      staking_accounts staking_accounts_table(get_self(), get_self().value);
      auto account_itr = staking_accounts_table.begin();
      while (account_itr != staking_accounts_table.end())
      {
         staking_allocations staking_allocations_table(get_self(), account_itr->staker.value);
         auto allocation_itr = staking_allocations_table.begin();
         while (allocation_itr != staking_allocations_table.end())
         {
            allocation_itr = staking_allocations_table.erase(allocation_itr);
         }

         account_itr = staking_accounts_table.erase(account_itr);
      }

      settings_table settings_table_instance(get_self(), get_self().value);

      // send all tokens back to infra.tmy
      staking_settings settings = settings_table_instance.get();
      eosio::action(
          {get_self(), "active"_n},
          TOKEN_CONTRACT,
          "transfer"_n,
          std::make_tuple(get_self(), "infra.tmy"_n, settings.total_staked + settings.total_releasing + settings.current_yield_pool, std::string("reset all")))
          .send(); // This will also run eosio::require_auth(get_self())
          
      settings_table_instance.remove();
   }
   #endif
}