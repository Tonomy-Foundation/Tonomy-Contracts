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
      stats statstable(get_self(), sym.code().raw());
      const auto &st = get_stats(statstable, sym);

      check_quantity(quantity, memo, st);

      check(to == st.issuer, "tokens can only be issued to issuer account");
      require_auth(st.issuer);

      add_supply(statstable, st, quantity);
      add_balance(st.issuer, quantity, get_self());
   }

   void token::retire(const asset &quantity, const string &memo)
   {
      auto sym = quantity.symbol;
      stats statstable(get_self(), sym.code().raw());
      const auto &st = get_stats(statstable, sym);

      check_quantity(quantity, memo, st);

      require_auth(st.issuer);

      sub_supply(statstable, st, quantity);
      sub_balance(st.issuer, quantity);
   }

   void token::bridgeissue(const name& to, const asset& quantity, const string& memo)
   {
      require_auth(get_self());
      check(is_account(to), "to account does not exist");

      auto sym = quantity.symbol;
      stats statstable(get_self(), sym.code().raw());
      const auto& st = get_stats(statstable, sym);

      check_quantity(quantity, memo, st);

      add_supply(statstable, st, quantity);

      add_balance(to, quantity, get_self());
      require_recipient(to);
   }

   void token::bridgeretire(const name& from, const asset& quantity, const string& memo)
   {
      require_auth(get_self());
      check(is_account(from), "from account does not exist");

      auto sym = quantity.symbol;
      stats statstable(get_self(), sym.code().raw());
      const auto& st = get_stats(statstable, sym);

      check_quantity(quantity, memo, st);

      sub_supply(statstable, st, quantity);

      sub_balance(from, quantity);
      require_recipient(from);
   }


   void token::transfer(const name &from,
                        const name &to,
                        const asset &quantity,
                        const string &memo)
   {
      check(from != to, "cannot transfer to self");
      require_auth(from);
      check(is_account(to), "to account does not exist");

      require_recipient(from);
      require_recipient(to);

      auto sym = quantity.symbol;
      stats statstable(get_self(), sym.code().raw());
      const auto &st = get_stats(statstable, sym);

      check_quantity(quantity, memo, st);

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

   void token::setstats()
   {
      // Admin only function
      require_auth(get_self());

      stats statstable2(get_self(), SYSTEM_RESOURCE_CURRENCY.code().raw());
      auto itr = statstable2.find(SYSTEM_RESOURCE_CURRENCY.code().raw());
      check(itr != statstable2.end(), "stats table not found for system currency");
      check(itr->supply.amount == 0, "supply is not 0, so should not be updated");

      statstable2.modify(itr, same_payer, [&](auto &s)
      {
         s.supply      = itr->max_supply;
         s.max_supply  = itr->max_supply;
         s.issuer      = itr->issuer;
      });
   }

   const token::currency_stats& token::get_stats(stats& statstable, const symbol& sym) {
      auto existing = statstable.find(sym.code().raw());
      check(existing != statstable.end(), "token with symbol does not exist");
      return *existing;
   }

   void token::check_quantity(const asset& quantity, const string& memo, const currency_stats& st) {
      auto sym = quantity.symbol;
      check(sym.is_valid(), "invalid symbol name");
      check(memo.size() <= 256, "memo has more than 256 bytes");
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.amount > 0, "must be positive quantity");
      check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   }

   void token::add_supply(stats& statstable, const currency_stats& st, const asset& quantity) {
      check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

      statstable.modify(st, same_payer, [&](auto &s)
                        { s.supply += quantity; });
   }

   void token::sub_supply(stats& statstable, const currency_stats& st, const asset& quantity) {
      check(st.supply.amount >= quantity.amount, "quantity exceeds available supply");
      
      statstable.modify(st, same_payer, [&](auto &s)
                        { s.supply -= quantity; });
   }
} /// namespace eosio