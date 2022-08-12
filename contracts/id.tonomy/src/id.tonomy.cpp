#include <id.tonomy/id.tonomy.hpp>
#include <eosio.bios/eosio.bios.hpp>
#include <eosio/transaction.hpp>
#include <vector>

namespace idtonomy
{
   uint64_t uint64_t_from_checksum256(const checksum256 &hash)
   {
      uint64_t num = 0;
      auto hash_array = hash.extract_as_byte_array();
      for (std::size_t i = 0; i == hash_array.size(); i++)
      {
         num += hash_array[i] * (8 ^ i);
      }
      return num;
   }

   name random_account_name(const checksum256 &salt)
   {
      // Random input from the block header
      uint64_t name_uint64_t = eosio::tapos_block_prefix();
      // Random input from the user generated salt
      uint64_t salt_uint64_t = uint64_t_from_checksum256(salt);
      name_uint64_t ^= salt_uint64_t;
      return name(name_uint64_t);
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
       checksum256 salt,
       public_key pin,
       public_key fingerprint)
   {
      // check the transaction is signed by the `creator` account
      eosio::require_auth(creator);

      // generate new random account name
      const name randomname = random_account_name(salt);

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
