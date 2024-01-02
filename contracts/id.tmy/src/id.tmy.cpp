#include <id.tmy/id.tmy.hpp>
#include <eosio.bios/eosio.bios.hpp>
#include <eosio/transaction.hpp>
#include <vector>
#include "../../errors.cpp"

namespace idtmy
{
   // contract class constructor
   id::id(name receiver, name code, eosio::datastream<const char *> ds) : // contract base class contructor
                                                                          contract(receiver, code, ds),
                                                                          // instantiate multi-index instance as data member (find it defined below)
                                                                          _people(receiver, receiver.value),
                                                                          _apps(receiver, receiver.value)
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

      // TODO go through and change any '.' character for a random character
      name res = name(name_uint64_t);
      return tidy_name(res, uint8_t(name_uint64_t), account_type);
   }

   eosiobios::authority create_authority_with_key(const eosio::public_key &key)
   {
      eosiobios::key_weight new_key = eosiobios::key_weight{.key = key, .weight = 1};
      eosiobios::authority new_authority{.threshold = 1, .keys = {new_key}, .accounts = {}, .waits = {}};

      return new_authority;
   }

   // add the eosio.code permission to allow the account to call the smart contract properly
   // https://developers.eos.io/welcome/v2.1/smart-contract-guides/adding-inline-actions#step-1-adding-eosiocode-to-permissions
   eosiobios::permission_level create_eosio_code_permission_level(const name &account)
   {
      return eosiobios::permission_level(account, "eosio.code"_n);
   }

   void id::newperson(
       checksum256 username_hash,
       public_key password_key,
       checksum256 password_salt)
   {
      // check the transaction is signed by the `id.tmy` account
      eosio::require_auth(get_self());

      // generate new random account name
      const name random_name = random_account_name(username_hash, password_salt, enum_account_type::Person);

      // use the password_key public key for the owner authority
      eosiobios::authority password_authority = create_authority_with_key(password_key);
      password_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      // If the account name exists, this will fail
      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(get_self(), random_name, password_authority, password_authority);

      // Check the username is not already taken
      auto people_by_username_hash_itr = _people.get_index<"usernamehash"_n>();
      const auto username_itr = people_by_username_hash_itr.find(username_hash);
      if (username_itr != people_by_username_hash_itr.end())
      {
         throwError("TCON1000", "This people username is already taken");
      }
      // If the account name exists, set resources and return
      auto existing_person = _people.find(random_name.value);
      if (existing_person != _people.end())
      {
         eosio::set_resource_limits(random_name, 0, this->initial_cpu_weight_allocation, this->initial_net_weight_allocation);
         return;
      }

      //Set the resource limits for the new account
      //uncomment this in task TODOS #77 

      // eosiobios::bios::resource_config_table _resource_config("eosio.bios"_n, "eosio.bios"_n.value);
      // auto config = _resource_config.get_or_create(get_self(), eosiobios::bios::resource_config());
      // config.total_cpu_weight_allocated = this->initial_cpu_weight_allocation;
      // config.total_net_weight_allocated = this->initial_net_weight_allocation;
      // _resource_config.set(config, get_self());

      // Store the password_salt and hashed username in table
      _people.emplace(get_self(), [&](auto &people_itr)
                      {
           people_itr.account_name = random_name;
           people_itr.status = idtmy::enum_account_status::Creating_Status;
           people_itr.username_hash = username_hash;
           people_itr.password_salt = password_salt;
            });

      // Store the account type in the account_type table
      account_type_table account_type(get_self(), get_self().value);
      account_type.emplace(get_self(), [&](auto& row) {
         row.account_name = random_name;
         row.acc_type = enum_account_type::Person;
         row.version = 1;
      });

   }
   
   void id::newapp(
       string app_name,
       string description,
       checksum256 username_hash,
       string logo_url,
       string origin,
       public_key key)
   {
      // TODO in the future only an organization type can create an app
      // check the transaction is signed by the `id.tmy` account
      eosio::require_auth(get_self());

      checksum256 description_hash = eosio::sha256(description.c_str(), description.length());

      // generate new random account name
      const eosio::name random_name = random_account_name(username_hash, description_hash, enum_account_type::App);

      // use the password_key public key for the owner authority
      eosiobios::authority key_authority = create_authority_with_key(key);
      key_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      // If the account name exists, this will fail
      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(get_self(), random_name, key_authority, key_authority);

      // Check the username is not already taken
      auto apps_by_username_hash_itr = _apps.get_index<"usernamehash"_n>();
      const auto username_itr = apps_by_username_hash_itr.find(username_hash);
      if (username_itr != apps_by_username_hash_itr.end())
      {
         throwError("TCON1001", "This app username is already taken");
      }

      // Check the origin is not already taken
      auto origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
      auto apps_by_origin_hash_itr = _apps.get_index<"originhash"_n>();
      const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
      if (origin_itr != apps_by_origin_hash_itr.end())
      {
         throwError("TCON1002", "This app origin is already taken");
      }

      // If the account name exists, set resources and return
      auto existing_person = _people.find(random_name.value);
      if (existing_person != _people.end())
      {
         eosio::set_resource_limits(random_name, 0, this->initial_cpu_weight_allocation, this->initial_net_weight_allocation);
         return;
      }
      //Set the resource limits for the new app
      //uncomment this in task TODOS #77 
      // eosiobios::bios::resource_config_table _resource_config("eosio.bios"_n, "eosio.bios"_n.value);
      // auto config = _resource_config.get_or_create(get_self(), eosiobios::bios::resource_config());
      // config.total_cpu_weight_allocated = this->initial_cpu_weight_allocation;
      // config.total_net_weight_allocated = this->initial_net_weight_allocation;
      // _resource_config.set(config, get_self());

      // Store the password_salt and hashed username in table
      _apps.emplace(get_self(), [&](auto &app_itr)
                    {
                           app_itr.account_name = random_name;
                           app_itr.app_name = app_name;
                           app_itr.description = description;
                           app_itr.logo_url = logo_url;
                           app_itr.origin = origin;
                           app_itr.username_hash = username_hash;
                            });

      // Store the account type in the account_type table
      account_type_table account_type(get_self(), get_self().value);
      account_type.emplace(get_self(), [&](auto& row) {
         row.account_name = random_name;
         row.acc_type = enum_account_type::App;
         row.version = 1;
      });
      
     
   }

   void id::setacctype(name account_name, account_type acc_type) {
         account_type_table account_type(get_self(), get_self().value);
         auto itr = account_type.find(account_name.value);
         if (itr != account_type.end()) {
            account_type.modify(itr, get_self(), [&](auto& row) {
               row.acc_type = acc_type;
               row.version = 1;
            });
         } else {
            account_type.emplace(get_self(), [&](auto& row) {
               row.account_name = account_name;
               row.acc_type = acc_type;
               row.version = 1;
            });
         }
      }

   void id::updatekeyper(name account,
                         permission_level permission_level,
                         public_key key,
                         bool link_auth)
   {
      // update the status if needed
      auto people_itr = _people.find(account.value);
      if (people_itr != _people.end())
      {
         if (people_itr->status == idtmy::enum_account_status::Creating_Status)
         {
            _people.modify(people_itr, get_self(), [&](auto &people_itr)
                           { people_itr.status = idtmy::enum_account_status::Active_Status; });
         }
      }

      // setup the new key authoritie(s)
      eosiobios::authority authority = create_authority_with_key(key);
      authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      name permission;
      switch (permission_level)
      {
      case idtmy::enum_permission_level::Pin:
         permission = "pin"_n;
         break;
      case idtmy::enum_permission_level::Biometric:
         permission = "biometric"_n;
         break;
      case idtmy::enum_permission_level::Local:
         permission = "local"_n;
         break;
      default:
         check(false, "Invalid permission level");
      }

      // must be signed by the account's permission_level or parent (from eosio.bios::updateauth())
      eosiobios::bios::updateauth_action updateauthaction("eosio"_n, {account, "owner"_n});
      updateauthaction.send(account, permission, "owner"_n, authority);

      if (link_auth)
      {
         // link the permission to the `loginwithapp` action
         eosiobios::bios::linkauth_action linkauthaction("eosio"_n, {account, "owner"_n});
         linkauthaction.send(account, get_self(), "loginwithapp"_n, permission);
         // TODO also needs to link to any other actions that require the permission that we know of at this stage
      }
   }

   void id::linkauth(name account,
                     name code,
                     name type,
                     name req)
   {
      // TODO check that "code" account is a registered, active app

      eosiobios::bios::linkauth_action linkauthaction("eosio"_n, {account, "active"_n});
      linkauthaction.send(account, code, type, req);
   }

   void id::loginwithapp(
       name account,
       name app,
       name parent,
       public_key key)
   {
      // check the app exists and is registered with status
      auto app_itr = _apps.find(app.value);
      check(app_itr != _apps.end(), "App does not exist");

      // TODO uncomment when apps have status
      // check(app_itr->status == idtmy::enum_account_status::Active_Status, "App is not active");

      // TODO check parent is only from allowed parents : "local", "pin", "biometric", "active"

      // TODO instead of "app" as the permission, use sha256(parent, app, name of key(TODO provide as argument with default = "main"))

      // setup the new key authoritie(s)
      eosiobios::authority authority = create_authority_with_key(key);

      eosiobios::bios::updateauth_action updateauthaction("eosio"_n, {account, parent});
      updateauthaction.send(account, app, parent, authority);
      // must be signed by the account's permission_level or parent (from eosio.bios::updateauth())
   }
}