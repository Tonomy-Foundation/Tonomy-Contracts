#include <tonomy/tonomy.hpp>
#include <eosio/symbol.hpp>
#include <eosio/transaction.hpp>
#include <vector>

namespace tonomysystem
{

   void require_governance_active()
   {
      eosio::require_auth({native::governance_name, "active"_n});
   }

   void require_governance_owner()
   {
      eosio::require_auth({native::governance_name, "owner"_n});
   }

   void special_governance_check(name account)
   {
      if (account == "eosio"_n || account == "tonomy"_n)
      {
         require_governance_owner();
      }
      else
      {
         require_governance_active();
      }
   }

   void native::newaccount(name creator, name name, authority owner, authority active)
   {
      require_governance_active();
      native::newaccount_action action("eosio"_n, {creator, "active"_n});
      action.send(creator, name, owner, active);
   }

   void native::updateauth(name account,
                           name permission,
                           name parent,
                           authority auth)
   {
      // gov only, till we implement this
      // upgrades to the tonomy contract itself requires governance approval (owner)
      special_governance_check(account);
      native::updateauth_action action("eosio"_n, {account, permission});
      action.send(account, permission, parent, auth);
   }

   void native::deleteauth(name account,
                           name permission)
   {
      require_governance_active();
      native::deleteauth_action action("eosio"_n, {account, permission});
      action.send(account, permission);
   }

   void native::linkauth(name account,
                         name code,
                         name type,
                         name requirement)
   {
      // TODO: this should be governance authorized only. Create a separate action in tonomy.cpp for People accounts to link their SSO account
      native::linkauth_action action("eosio"_n, {account, "active"_n});
      action.send(account, code, type, requirement);
   }

   void native::unlinkauth(name account,
                           name code,
                           name type)
   {
      require_governance_active();
      native::unlinkauth_action action("eosio"_n, {account, "active"_n});
      action.send(account, code, type);
   }

   void native::canceldelay(permission_level canceling_auth, checksum256 trx_id)
   {
      require_governance_active();
      native::canceldelay_action action("eosio"_n, {governance_name, "active"_n});
      action.send(canceling_auth, trx_id);
   }

   void native::setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code)
   {
      // gov only, till we implement this
      // upgrades to the tonomy contract itself requires governance approval (owner)
      special_governance_check(account);

      native::setcode_action action("eosio"_n, {account, "active"_n});
      action.send(account, vmtype, vmversion, code);
   }

   void native::setabi(name account, const std::vector<char> &abi)
   {
      require_governance_active();
      native::setabi_action action("eosio"_n, {account, "active"_n});
      action.send(account, abi);
   }

   void native::setpriv(name account, uint8_t is_priv)
   {
      // TODO disable proxying: this contract is priviledged and can execute the required API calls directly
      require_governance_owner();
      native::setpriv_action action("eosio"_n, {governance_name, "active"_n});
      action.send(account, is_priv);
   }

   void native::setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight)
   {
      // TODO delete function. This is handled by our own resource management
      require_governance_owner();
      native::setalimits_action action("eosio"_n, {governance_name, "active"_n});
      action.send(account, ram_bytes, net_weight, cpu_weight);
   }

   void native::setprods(const std::vector<eosio::producer_authority> &schedule)
   {
      require_governance_owner();
      native::setprods_action action("eosio"_n, {governance_name, "active"_n});
      action.send(schedule);
   }

   void native::setparams(const eosio::blockchain_parameters &params)
   {
      // TODO disable proxying: this contract is priviledged and can execute the required API calls directly
      require_governance_owner();
      native::setparams_action action("eosio"_n, {governance_name, "active"_n});
      action.send(params);
   }

   void native::reqauth(name from)
   {
      // TODO delete as not needed. Check in Telegram first
      require_governance_owner();
      native::reqauth_action action("eosio"_n, {from, "active"_n});
      action.send(from);
   }

   void native::activate(const eosio::checksum256 &feature_digest)
   {
      require_governance_owner();
      native::activate_action action("eosio"_n, {governance_name, "active"_n});
      action.send(feature_digest);
   }

   void native::reqactivated(const eosio::checksum256 &feature_digest)
   {
      require_governance_owner();
      native::reqactivated_action action("eosio"_n, {governance_name, "active"_n});
      action.send(feature_digest);
   }

   void native::onerror(uint128_t sender_id, std::vector<char> sent_trx)
   {
      // TODO delete: this is not needed in this contract. It is not supposed to be called in the eosio contract
      require_governance_active();
      native::onerror_action action("eosio"_n, {governance_name, "active"_n});
      action.send(sender_id, sent_trx);
   }

}
