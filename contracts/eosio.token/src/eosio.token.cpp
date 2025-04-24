#include <eosio.token/eosio.token.hpp>

namespace eosio
{

   void token::create(const name &issuer,
                      const asset &maximum_supply)
   {
      require_auth(get_self());

      auto sym = maximum_supply.symbol;
      check(maximum_supply.is_valid(), "invalid supply");
      check(maximum_supply.amount > 0, "max-supply must be positive");

      stats statstable(get_self(), sym.code().raw());
      auto existing = statstable.find(sym.code().raw());
      check(existing == statstable.end(), "token with symbol already exists");

      statstable.emplace(get_self(), [&](auto &s)
                         {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer; });
   }

   void token::issue(const name &to, const asset &quantity, const string &memo)
   {
      auto sym = quantity.symbol;

      check(sym.is_valid(), "invalid symbol name");
      check(memo.size() <= 256, "memo has more than 256 bytes");

      stats statstable(get_self(), sym.code().raw());
      auto existing = statstable.find(sym.code().raw());
      check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
      const auto &st = *existing;
      check(to == st.issuer, "tokens can only be issued to issuer account");

      require_auth(st.issuer);
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.amount > 0, "must issue positive quantity");

      check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
      check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

      statstable.modify(st, same_payer, [&](auto &s)
                        { s.supply += quantity; });

      add_balance(st.issuer, quantity, get_self());
   }

   void token::retire(const asset &quantity, const string &memo)
   {
      auto sym = quantity.symbol;
      check(sym.is_valid(), "invalid symbol name");
      check(memo.size() <= 256, "memo has more than 256 bytes");

      stats statstable(get_self(), sym.code().raw());
      auto existing = statstable.find(sym.code().raw());
      check(existing != statstable.end(), "token with symbol does not exist");
      const auto &st = *existing;

      require_auth(st.issuer);
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.amount > 0, "must retire positive quantity");

      check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

      statstable.modify(st, same_payer, [&](auto &s)
                        { s.supply -= quantity; });

      sub_balance(st.issuer, quantity);
   }

   void token::transfer(const name &from,
                        const name &to,
                        const asset &quantity,
                        const string &memo)
   {
      check(from != to, "cannot transfer to self");
      require_auth(from);
      check(is_account(to), "to account does not exist");

      auto sym = quantity.symbol.code();
      stats statstable(get_self(), sym.raw());
      const auto &st = statstable.get(sym.raw());

      require_recipient(from);
      require_recipient(to);

      check(quantity.is_valid(), "invalid quantity");
      check(quantity.amount > 0, "must transfer positive quantity");
      check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
      check(memo.size() <= 256, "memo has more than 256 bytes");

      auto payer = get_self();

      sub_balance(from, quantity);
      add_balance(to, quantity, payer);
   }

   void token::sub_balance(const name &owner, const asset &value)
   {
      accounts from_acnts(get_self(), owner.value);

      const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
      check(from.balance.amount >= value.amount, "overdrawn balance");

      from_acnts.modify(from, get_self(), [&](auto &a)
                        { a.balance -= value; });
   }

   void token::add_balance(const name &owner, const asset &value, const name &ram_payer)
   {
      accounts to_acnts(get_self(), owner.value);
      auto to = to_acnts.find(value.symbol.code().raw());
      if (to == to_acnts.end())
      {
         to_acnts.emplace(ram_payer, [&](auto &a)
                          { a.balance = value; });
      }
      else
      {
         to_acnts.modify(to, same_payer, [&](auto &a)
                         { a.balance += value; });
      }
   }

   void token::open(const name &owner, const symbol &symbol, const name &ram_payer)
   {
      require_auth(ram_payer);

      check(is_account(owner), "owner account does not exist");

      auto sym_code_raw = symbol.code().raw();
      stats statstable(get_self(), sym_code_raw);
      const auto &st = statstable.get(sym_code_raw, "symbol does not exist");
      check(st.supply.symbol == symbol, "symbol precision mismatch");

      accounts acnts(get_self(), owner.value);
      auto it = acnts.find(sym_code_raw);
      if (it == acnts.end())
      {
         acnts.emplace(ram_payer, [&](auto &a)
                       { a.balance = asset{0, symbol}; });
      }
   }

   void token::close(const name &owner, const symbol &symbol)
   {
      require_auth(owner);
      accounts acnts(get_self(), owner.value);
      auto it = acnts.find(symbol.code().raw());
      check(it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect.");
      check(it->balance.amount == 0, "Cannot close because the balance is not zero.");
      acnts.erase(it);
   }

   void token::migratestats()
   {
      // Admin only function
      require_auth(get_self());

      asset max_supply;
      name issuer;

      // Look at the table using old symbol as scope
      stats statstable(get_self(), SYSTEM_RESOURCE_CURRENCY_OLD.code().raw());
      // Check if old token found
      auto existing_itr = statstable.find(SYSTEM_RESOURCE_CURRENCY_OLD.code().raw());
      if(existing_itr != statstable.end()) {
         max_supply = asset(existing_itr->max_supply.amount, SYSTEM_RESOURCE_CURRENCY);
         issuer = existing_itr->issuer;
         statstable.erase(existing_itr);
      }
      // Check if new token found
      auto new_itr = statstable.find(SYSTEM_RESOURCE_CURRENCY.code().raw());
      if(new_itr != statstable.end()) {
         max_supply = asset(new_itr->max_supply.amount, SYSTEM_RESOURCE_CURRENCY);
         issuer = new_itr->issuer;
         statstable.erase(new_itr);
      }

      // New symbol as scope
      stats statstable2(get_self(), SYSTEM_RESOURCE_CURRENCY.code().raw());
      statstable2.emplace(get_self(), [&](auto &s)
      {
         s.supply.symbol = SYSTEM_RESOURCE_CURRENCY;
         s.max_supply    = max_supply;
         s.issuer        = issuer;
      });

   }

   void token::migrateacc(const name &account)
   {
      // Admin only function
      require_auth(get_self());

      accounts account_balance(get_self(), account.value);

      auto balance_itr = account_balance.find(SYSTEM_RESOURCE_CURRENCY_OLD.code().raw());
      
      if (balance_itr != account_balance.end()) {
         account_balance.emplace(get_self(), [&](auto &a) {
            a.balance = asset(balance_itr->balance.amount, SYSTEM_RESOURCE_CURRENCY);
         });
         account_balance.erase(balance_itr);
      }
   }
} /// namespace eosio