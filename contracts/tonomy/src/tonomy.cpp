#include <tonomy/tonomy.hpp>
#include <eosio/symbol.hpp>
#include <eosio/transaction.hpp>
#include <vector>

#include <eosio.tonomy/eosio.tonomy.hpp>

namespace tonomysystem
{
   // contract class constructor
   tonomy::tonomy(name receiver, name code, eosio::datastream<const char *> ds) : native(receiver, code, ds),
                                                                                  // instantiate multi-index instance as data member (find it defined below)
                                                                                  _people(receiver, receiver.value)
   {
   }

   void throwError(string error_code, string message)
   {
      check(false, error_code + ": " + message);
   }

   std::map<enum_account_type, char> account_type_letters = {
       {enum_account_type::Person, 'p'},
       {enum_account_type::App, 'a'},
       {enum_account_type::Organization, 'o'},
       {enum_account_type::Gov, 'g'},
       {enum_account_type::Service, 's'}};

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

   static constexpr char *charmap = (char *)"12345abcdefghijklmnopqrstuvwxyz";

   name tidy_name(const name &account_name, const uint8_t random_number, const enum_account_type &account_type)
   {
      std::string name_string = account_name.to_string();

      // Set the first character to the account type
      name_string[0] = account_type_letters[account_type];

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

   name random_account_name(const checksum256 &hash1, const checksum256 &hash2, const enum_account_type &account_type)
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

      // TODO: go through and change any '.' character for a random character
      name res = name(name_uint64_t);
      return tidy_name(res, uint8_t(name_uint64_t), account_type);
   }

   authority create_authority_with_key(const eosio::public_key &key)
   {
      key_weight new_key = key_weight{.key = key, .weight = 1};
      authority new_authority{.threshold = 1, .keys = {new_key}, .accounts = {}, .waits = {}};

      return new_authority;
   }

   authority create_authority_with_account(const eosio::name &account)
   {
      permission_level_weight new_account = permission_level_weight{.permission = permission_level(account, "active"_n), .weight = 1};
      authority new_authority{.threshold = 1, .keys = {}, .accounts = {new_account}, .waits = {}};

      return new_authority;
   }

   // add the eosio.code permission to allow the account to call the smart contract properly
   // https://developers.eos.io/welcome/v2.1/smart-contract-guides/adding-inline-actions#step-1-adding-eosiocode-to-permissions
   permission_level create_eosio_code_permission_level(const name &account)
   {
      return permission_level(account, "eosio.code"_n);
   }

   void tonomy::newperson(
       checksum256 username_hash,
       public_key password_key,
       checksum256 password_salt)
   {
      // check the transaction is signed by the `id.tmy` account
      eosio::require_auth(get_self());

      // generate new random account name
      const name random_name = random_account_name(username_hash, password_salt, enum_account_type::Person);

      // use the password_key public key for the owner authority
      authority password_authority = create_authority_with_key(password_key);
      password_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      // If the account name exists, this will fail
      newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(get_self(), random_name, password_authority, password_authority);

      // Check the username is not already taken
      auto people_by_username_hash_itr = _people.get_index<"usernamehash"_n>();
      const auto username_itr = people_by_username_hash_itr.find(username_hash);
      if (username_itr != people_by_username_hash_itr.end())
      {
         throwError("TCON1000", "This people username is already taken");
      }

      // Store the password_salt and hashed username in table
      _people.emplace(get_self(), [&](auto &people_itr)
                      {
           people_itr.account_name = random_name;
           people_itr.status = enum_account_status::Creating_Status;
           people_itr.username_hash = username_hash;
           people_itr.password_salt = password_salt; });

      // Store the account type in the account_type table
      account_type_table account_type(get_self(), get_self().value);
      account_type.emplace(get_self(), [&](auto &row)
                           {
         row.account_name = random_name;
         row.acc_type = enum_account_type::Person;
         row.version = 1; });
   }

   void tonomy::updatekeyper(name account,
                             permission_level_name permission_level,
                             public_key key,
                             bool link_auth)
   {
      // eosio::require_auth(account); // this is not needed as tonomy::tonomy::updateauth_action checks the permission

      // update the status if needed
      auto people_itr = _people.find(account.value);
      if (people_itr != _people.end())
      {
         if (people_itr->status == enum_account_status::Creating_Status)
         {
            _people.modify(people_itr, get_self(), [&](auto &people_itr)
                           { people_itr.status = enum_account_status::Active_Status; });
            eosio::set_resource_limits(account, this->inital_ram_bytes, this->initial_cpu_weight_allocation, this->initial_net_weight_allocation);

            resource_config_table _resource_config(get_self(), get_self().value);
            auto config = _resource_config.get();
            config.total_ram_used += this->inital_ram_bytes;
            config.total_cpu_weight_allocated += this->initial_cpu_weight_allocation;
            config.total_net_weight_allocated += this->initial_net_weight_allocation;
            _resource_config.set(config, get_self());
         }
      }

      // setup the new key authoritie(s)
      authority authority = create_authority_with_key(key);
      authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      name permission;
      switch (permission_level)
      {
      case enum_permission_level_name::Pin:
         permission = "pin"_n;
         break;
      case enum_permission_level_name::Biometric:
         permission = "biometric"_n;
         break;
      case enum_permission_level_name::Local:
         permission = "local"_n;
         break;
      default:
         check(false, "Invalid permission level");
      }

      // must be signed by the account's permission_level or parent (from eosio.tonomy::updateauth())
      eosiotonomy::bios::updateauth_action updateauthaction("eosio"_n, {account, "owner"_n});
      updateauthaction.send(account, permission, "owner"_n, authority);

      if (link_auth)
      {
         // link the permission to the `loginwithapp` action
         linkauth_action linkauthaction("eosio"_n, {account, "owner"_n});
         linkauthaction.send(account, get_self(), "loginwithapp"_n, permission);
         // TODO: also needs to link to any other actions that require the permission that we know of at this stage
      }
   }

   void tonomy::updateactive(name account, authority active)
   {
      // eosio::require_auth(account); // this is not needed as tonomy::tonomy::updateauth_action checks the permission
      eosiotonomy::bios::updateauth_action updateauthaction("eosio"_n, {account, "active"_n});
      updateauthaction.send(account, "active"_n, "owner"_n, active);
   }

   void tonomy::setresparams(double ram_price, uint64_t total_ram_available, double ram_fee)
   {
      require_auth(native::governance_name); // check authorization is gov.tmy

      // check ram_price is within bounds, not negative or too high
      eosio::check(ram_price >= 0, "RAM price must be non-negative");

      // check ram_fee is non-negative
      eosio::check(ram_fee >= 0, "RAM fee must be non-negative");

      resource_config_table resource_config_singleton(get_self(), get_self().value);
      resource_config config;

      if (resource_config_singleton.exists())
      {
         // Singleton exists, get the existing config and modify the values
         config = resource_config_singleton.get();
         config.ram_price = ram_price;
         config.total_ram_available = total_ram_available;
         config.ram_fee = ram_fee;
      }
      else
      {
         // Singleton does not exist, set the values and also set other values to 0
         config = resource_config{ram_fee, ram_price, total_ram_available, 0, 0, 0};
      }

      // Save the modified or new config back to the singleton
      resource_config_singleton.set(config, get_self());
   }

}
