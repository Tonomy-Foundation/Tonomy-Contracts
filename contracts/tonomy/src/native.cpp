#include <tonomy/tonomy.hpp>
#include <eosio/symbol.hpp>
#include <eosio/transaction.hpp>
#include <vector>

namespace tonomysystem
{

   // This action can only be called by inline action from the {sender} account
   void check_sender(name sender)
   {
      check(eosio::get_sender() == sender, "You cannot call this eosio action directly, call from the " + sender.to_string() + " contract");
   }

   void native::newaccount(name creator, name name, authority owner, authority active)
   {
      require_auth(governance_name);
      native::newaccount_action action("eosio"_n, {get_self(), "active"_n});
      action.send(creator, name, owner, active);
   }

   void native::updateauth(name account,
                           name permission,
                           name parent,
                           authority auth)
   {
      require_auth(governance_name);
      native::updateauth_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, permission, parent, auth);
   }

   void native::deleteauth(name account,
                           name permission)
   {
      require_auth(governance_name);
      native::deleteauth_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, permission);
   }

   void native::linkauth(name account,
                         name code,
                         name type,
                         name requirement)
   {
      require_auth(governance_name);
      native::linkauth_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, code, type, requirement);
   }

   void native::unlinkauth(name account,
                           name code,
                           name type)
   {
      require_auth(governance_name);
      native::unlinkauth_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, code, type);
   }

   void native::canceldelay(permission_level canceling_auth, checksum256 trx_id)
   {
      require_auth(governance_name);
      native::canceldelay_action action("eosio"_n, {get_self(), "active"_n});
      action.send(canceling_auth, trx_id);
   }

   void native::setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code)
   {
      require_auth(governance_name);
      native::setcode_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, vmtype, vmversion, code);
   }

   void native::setabi(name account, const std::vector<char> &abi)
   {
      require_auth(governance_name);
      native::setabi_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, abi);
   }

   void native::setpriv(name account, uint8_t is_priv)
   {
      require_auth(governance_name);
      native::setpriv_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, is_priv);
   }

   void native::setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight)
   {
      require_auth(governance_name);
      native::setalimits_action action("eosio"_n, {get_self(), "active"_n});
      action.send(account, ram_bytes, net_weight, cpu_weight);
   }

   void native::setprods(const std::vector<eosio::producer_authority> &schedule)
   {
      require_auth(governance_name);
      native::setprods_action action("eosio"_n, {get_self(), "active"_n});
      action.send(schedule);
   }

   void native::setparams(const eosio::blockchain_parameters &params)
   {
      require_auth(governance_name);
      native::setparams_action action("eosio"_n, {get_self(), "active"_n});
      action.send(params);
   }

   void native::reqauth(name from)
   {
      require_auth(governance_name);
      native::reqauth_action action("eosio"_n, {get_self(), "active"_n});
      action.send(from);
   }

   void native::activate(const eosio::checksum256 &feature_digest)
   {
      require_auth(governance_name);
      native::activate_action action("eosio"_n, {get_self(), "active"_n});
      action.send(feature_digest);
   }

   void native::reqactivated(const eosio::checksum256 &feature_digest)
   {
      require_auth(governance_name);
      native::reqactivated_action action("eosio"_n, {get_self(), "active"_n});
      action.send(feature_digest);
   }

   void native::onerror(uint128_t sender_id, std::vector<char> sent_trx)
   {
      require_auth(governance_name);
      native::onerror_action action("eosio"_n, {get_self(), "active"_n});
      action.send(sender_id, sent_trx);
   }

}
