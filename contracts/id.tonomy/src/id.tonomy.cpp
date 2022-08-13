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
      std::array<uint8_t, 32> hash_array = hash.extract_as_byte_array();

      // iterate over the first 8 bytes (only need 8 bytes for uint64_t)
      for (size_t i = 0; i < 8; i++)
      {
         // for each byte add it to the number, right shifting the number of bits the array element is from
         num += hash_array[i] << 8 * i;
      }
      return num;
   }

   static const char *charmap = "12345abcdefghijklmnopqrstuvwxyz";

   name tidy_name(const name &account_name, const uint8_t random_number)
   {
      std::string name_string = account_name.to_string();

      // Remove any . character and replace with random character
      for (int i = 0; i < name_string.length(); i++)
      {
         if (name_string[i] == '.')
         {
            // TODO: if this is the last character then it must not be greater then 'f'
            name_string[i] = charmap[(random_number * i) % 31];
         }
      }

      // remove last character (only 12 characters allowed)
      name_string.erase(name_string.end() - 1);
      return name(name_string);
   }

   name random_account_name(const checksum256 &hash1, const checksum256 &hash2)
   {
      // Put random input from the block header (32 bits) in the first and last 32 bits
      uint64_t tapos = eosio::tapos_block_prefix();
      uint64_t name_uint64_t = tapos;
      name_uint64_t ^= tapos << 32;

      // Put the random input from hash1 (256 bits aka 32 bytes) in first 32 bits
      uint64_t hash_uint64_t = uint64_t_from_checksum256(hash1);
      name_uint64_t ^= hash_uint64_t;

      // Put the random input from hash2 (32 bytes) in the last 32 bytes
      hash_uint64_t = uint64_t_from_checksum256(hash2);
      name_uint64_t ^= hash_uint64_t << 32;

      // TODO go through and change any '.' character for a random character
      name res = name(name_uint64_t);
      return tidy_name(res, uint8_t(name_uint64_t));
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

      // use the password public key for the owner authority
      eosiobios::authority owner = create_authory_with_key(password);

      // create the account with the random account name, and the ower authority for both the owner and active permission
      // if the account name exists, this will fail
      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(creator, random_name, owner, owner);

      eosiobios::authority owner = create_authory_with_key(pin);
      eosiobios::bios::updateauth_action updateauthaction("eosio"_n, {get_self(), "active"_n});
      newacupdateauthactioncountaction.send(random_name, "pin"_n, "owner"_n, owner);
      //    // TODO:
      //    // update key with pin
      //    // update key with fingerprint
      //    // may need to do this in separate action, or perhaps separate transaction... need to test
      //    // may need to use status to lock the account till finished craeating

      //    // Check the username is not already taken
      auto accounts_by_username_hash_itr = _accounts.get_index<"usernamehash"_n>();
      const auto username_itr = accounts_by_username_hash_itr.find(username_hash);
      if (username_itr != accounts_by_username_hash_itr.end())
      {
         check(false, "This username is already taken");
      }

      //    // Store the salt and hashed username in table
      _accounts.emplace(get_self(), [&](auto &account_itr)
                        {
           account_itr.account_name = random_name;
           account_itr.type = idtonomy::enum_account_type::Person;
           account_itr.status = idtonomy::enum_account_status::Creating;
           account_itr.username_hash = username_hash;
           account_itr.salt = salt; });
   }
}
