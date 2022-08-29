#include <eosio.bios.tonomy/eosio.bios.tonomy.hpp>

namespace eosiobiostonomy
{
   // This action can only be called by inline action from the {sender} account
   void check_sender(name sender)
   {
      check(eosio::get_sender() == sender, "You cannot call this eosio action directly, call from the " + sender.to_string() + " contract");
   }

   void bios::newaccount(name creator, name name, ignore<authority> owner, ignore<authority> active)
   {
      // require_auth(creator); // this is done implicity in apply_eosio_newaccount() which checks the "active" permission...
      check_sender("id.tonomy"_n);
   }

   void bios::updateauth(ignore<name> account,
                         ignore<name> permission,
                         ignore<name> parent,
                         ignore<authority> auth)
   {
      // check(has_auth({account, permission}) || has_auth({account, parent}), "not authorized by parent or permission"); // this is done implicity in apply_eosio_updateauth()...
      check_sender("id.tonomy"_n);
   }

   void bios::deleteauth(ignore<name> account,
                         ignore<name> permission)
   {
      // require_auth({account, permission}); // this is done implicity in apply_eosio_deleteauth()...
      check_sender("id.tonomy"_n);
   }

   void bios::linkauth(ignore<name> account,
                       ignore<name> code,
                       ignore<name> type,
                       ignore<name> requirement)
   {
      check_sender("id.tonomy"_n);
   }

   void bios::unlinkauth(ignore<name> account,
                         ignore<name> code,
                         ignore<name> type)
   {
      check_sender("id.tonomy"_n);
   }

   // TODO need to change so that other functions can only be called by tonomy logic

   void bios::setabi(name account, const std::vector<char> &abi)
   {
      abi_hash_table table(get_self(), get_self().value);
      auto itr = table.find(account.value);
      if (itr == table.end())
      {
         table.emplace(account, [&](auto &row)
                       {
         row.owner = account;
         row.hash  = eosio::sha256(const_cast<char*>(abi.data()), abi.size()); });
      }
      else
      {
         table.modify(itr, eosio::same_payer, [&](auto &row)
                      { row.hash = eosio::sha256(const_cast<char *>(abi.data()), abi.size()); });
      }
   }

   void bios::onerror(ignore<uint128_t>, ignore<std::vector<char>>)
   {
      check(false, "the onerror action cannot be called directly");
   }

   void bios::setpriv(name account, uint8_t is_priv)
   {
      require_auth(get_self());
      set_privileged(account, is_priv);
   }

   void bios::setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight)
   {
      require_auth(get_self());
      set_resource_limits(account, ram_bytes, net_weight, cpu_weight);
   }

   void bios::setprods(const std::vector<eosio::producer_authority> &schedule)
   {
      require_auth(get_self());
      set_proposed_producers(schedule);
   }

   void bios::setparams(const eosio::blockchain_parameters &params)
   {
      require_auth(get_self());
      set_blockchain_parameters(params);
   }

   void bios::reqauth(name from)
   {
      require_auth(from);
   }

   void bios::activate(const eosio::checksum256 &feature_digest)
   {
      require_auth(get_self());
      preactivate_feature(feature_digest);
   }

   void bios::reqactivated(const eosio::checksum256 &feature_digest)
   {
      check(is_feature_activated(feature_digest), "protocol feature is not activated");
   }

}
