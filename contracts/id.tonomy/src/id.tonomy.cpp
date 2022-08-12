#include <id.tonomy/id.tonomy.hpp>
#include <eosio.bios/eosio.bios.hpp>
#include <eosio/transaction.hpp>
#include <vector>

namespace idtonomy
{
   // contract class constructor
   id::id(name receiver, name code, eosio::datastream<const char *> ds) : // contract base class contructor
                                                                          contract(receiver, code, ds),
                                                                          // instantiate multi-index instance as data member (find it defined below)
                                                                          _accounts(receiver, receiver.value)
   {
   }

   uint64_t uint64_t_from_checksum256(const checksum256 &hash)
   {
      uint64_t num = 0;
      // get an array of 32 bytes from the hash
      auto hash_array = hash.extract_as_byte_array();

      for (size_t i = 0; i < hash_array.size(); i++)
      {
         // for each byte add it to the number, right shifting the number of bits the array element is from
         num += hash_array[i] << 8 * i;
      }
      return num;
   }

   name random_account_name(const checksum256 &hash1, const checksum256 &hash2)
   {
      // Random input from the block header (8 bytes)
      uint64_t name_uint64_t = eosio::tapos_block_prefix();
      print("\n name_uint64_t: ");
      print(name_uint64_t);

      // Random input from hash1 (32 bytes)
      uint64_t hash_uint64_t = uint64_t_from_checksum256(hash1);
      print("\n hash_uint64_t: ");
      print(hash_uint64_t);

      name_uint64_t += hash_uint64_t << 8 * 8;
      print("\n name_uint64_t: ");
      print(name_uint64_t);

      // Random input from hash2 (32 bytes)
      hash_uint64_t = uint64_t_from_checksum256(hash2);
      print("\n hash_uint64_t: ");
      print(hash_uint64_t);

      name_uint64_t += hash_uint64_t << 8 * (8 + 32);
      print("\n name_uint64_t: ");
      print(name_uint64_t);

      // TODO go through and change any '.' character for a random character
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
      const name random_name = random_account_name(username_hash, salt);
      eosio::print("\n random_name: ");
      eosio::print(random_name);
      //    // use the password public key for the owner authority
      //    eosiobios::authority owner = create_authory_with_key(password);

      //    // create the account with the random account name, and the ower authority for both the owner and active permission
      //    // if the account name exists, this will fail
      //    eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      //    newaccountaction.send(creator, random_name, owner, owner);

      //    // TODO:
      //    // update key with pin
      //    // update key with fingerprint
      //    // may need to do this in separate action, or perhaps separate transaction... need to test
      //    // may need to use status to lock the account till finished craeating

      //    // Check the username is not already taken
      //    auto accounts_by_username_hash_itr = _accounts.get_index<"usernamehash"_n>();
      //    const auto username_itr = accounts_by_username_hash_itr.find(username_hash);
      //    if (username_itr != accounts_by_username_hash_itr.end())
      //    {
      //       if (username_itr->status == idtonomy::enum_account_status::Creating)
      //       {
      //          check(false, "Account keys still need to be added in a follow-up action, add the keys or this account will be deactivated");
      //       }
      //       else
      //       {
      //          check(false, "This username is already taken");
      //       }
      //    }

      //    // Store the salt and hashed username in table
      //    _accounts.emplace(get_self(), [&](auto &account_itr)
      //                      {
      //      account_itr.account_name = random_name;
      //      account_itr.type = idtonomy::enum_account_type::Person;
      //      account_itr.status = idtonomy::enum_account_status::Creating;
      //      account_itr.username_hash = username_hash;
      //      account_itr.salt = salt; });
   }
}
