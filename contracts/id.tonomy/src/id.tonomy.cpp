#include <id.tonomy/id.tonomy.hpp>
#include <eosio.bios/eosio.bios.hpp>
#include <vector>

namespace idtonomy
{
   name random_account_name()
   {
      // inspired by https://github.com/bada-studio/knights_contract/blob/master/knights/contract/system_control.hpp#L162

      size_t ts eosio::read_transaction(
          char *ptr,
          eosio::transaction_size())

          int x = eosio::tapos_block_prefix();
   }

   eosiobios::authority create_authory_with_key(const eosio::public_key &key)
   {
      eosiobios::authority new_authority{}; // zero initialized
      new_authority.threshold = 1;
      eosiobios::key_weight key_weight1;
      key_weight1.weight = 1;
      key_weight1.key = key;
      new_authority.keys.push_back(key_weight1);
      return new_authority;
   }

   void id::newperson(
       name creator,
       checksum256 username_hash,
       public_key password,
       std::string salt,
       public_key pin,
       public_key fingerprint)
   {
      // check the transaction is signed by the `creator` account
      eosio::require_auth(creator);

      // generate new random account name
      const name randomname = random_account_name();

      // use the password public key for the owner authority
      eosiobios::authority owner = create_authory_with_key(password);

      // create the account with the random account name, and the ower authority for both the owner and active permission
      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(creator, randomname, owner, owner);

      // TODO:
      // update key with pin
      // update key with fingerprint
      // Store the salt and hashed username in table, with type = Person

      check(false, "Check false");
   }

}
