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
      // if the account name exists, this will fail
      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(creator, randomname, owner, owner);

      // TODO:
      // update key with pin
      // update key with fingerprint
      // may need to use status to lock the account till finished craeating

      // Check the username is not already taken
      auto accounts_by_username_hash_itr = _accounts.get_index<"usernamehash"_n>();
      const auto username_itr = accounts_by_username_hash_itr.find(username_hash);
      if (username_itr != accounts_by_username_hash_itr.end())
      {
         check(false, "This username is already taken");
      }

      // Store the salt and hashed username in table, with type = Person
      _accounts.emplace(get_self(), [&](auto &account_itr)
                        {
        account_itr.account_name = randomname;
        account_itr.type = idtonomy::AccountType::Person;
        account_itr.status = idtonomy::AccountStatus::Creating;
        account_itr.username_hash = username_hash;
        account_itr.salt = salt; });
   }
}
