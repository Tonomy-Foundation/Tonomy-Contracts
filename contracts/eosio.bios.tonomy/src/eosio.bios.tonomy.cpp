#include <eosio.bios.tonomy/eosio.bios.tonomy.hpp>
#include <eosio/symbol.hpp>
#include <eosio/transaction.hpp>
#include <vector>

namespace eosiobiostonomy
{
   // contract class constructor
   bios::bios(name receiver, name code, eosio::datastream<const char *> ds) : // contract base class contructor
                                                                              contract(receiver, code, ds),
                                                                              // instantiate multi-index instance as data member (find it defined below)
                                                                              _people(receiver, receiver.value),
                                                                              _apps(receiver, receiver.value)
   {
   }
   // This action can only be called by inline action from the {sender} account
   void check_sender(name sender)
   {
      check(eosio::get_sender() == sender, "You cannot call this eosio action directly, call from the " + sender.to_string() + " contract");
   }

   void bios::newaccount(name creator, name name, ignore<authority> owner, ignore<authority> active)
   {
      check_sender(get_self());
   }

   void bios::updateauth(ignore<name> account,
                         ignore<name> permission,
                         ignore<name> parent,
                         ignore<authority> auth)
   {
      check_sender(get_self());
   }

   void bios::deleteauth(ignore<name> account,
                         ignore<name> permission)
   {
      check_sender(get_self());
   }

   void bios::linkauth(ignore<name> account,
                       ignore<name> code,
                       ignore<name> type,
                       ignore<name> requirement)
   {
      check_sender(get_self());
   }

   void bios::unlinkauth(ignore<name> account,
                         ignore<name> code,
                         ignore<name> type)
   {
      check_sender(get_self());
   }

   // TODO need to change so that other functions can only be called by tonomy logic

   void bios::setabi(name account, const std::vector<char> &abi)
   {
      require_auth(gov_name);
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
      require_auth(gov_name);
      check(is_feature_activated(feature_digest), "protocol feature is not activated");
   }

   void bios::setresparams(double ram_price, uint64_t total_ram_available, double ram_fee)
   {
      require_auth(gov_name); // check authorization is gov.tmy

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
         config = resource_config{ram_price, ram_fee, total_ram_available, 0, 0, 0};
      }

      // Save the modified or new config back to the singleton
      resource_config_singleton.set(config, get_self());
   }

   void bios::buyram(const name &dao_owner, const name &app, const asset &quant)
   {
      require_auth(app); // Check that the app has the necessary authorization

      // Access the account table from id.tmy.hpp
      eosiobiostonomy::bios::account_type_table account_type(get_self(), get_self().value);
      // Check the account type of the app
      auto itr = account_type.find(app.value);
      eosio::check(itr != account_type.end(), "Could not find account");
      eosio::check(itr->acc_type == eosiobiostonomy::enum_account_type::App, "Only apps can buy and sell RAM");

      // Check that the RAM is being purchased with the correct token
      eosio::check(quant.symbol == bios::system_resource_currency, "must buy ram with core token");

      // Check that the amount of tokens being used for the purchase is positive
      eosio::check(quant.amount > 0, "Amount must be positive");

      // Get the RAM price
      // resource_config_table resource_config_singleton(get_self(), get_self().value);
      //  auto config = resource_config_singleton.get();
      resource_config_table config_table(get_self(), get_self().value);

      // Retrieve the resource_config object from the table
      resource_config config;
      if (config_table.exists())
      {
         config = config_table.get();
      }
      else
      {
         eosio::check(false, "Resource config does not exist");
      }

      // Check if the values are retrieved successfully
      eosio::check(config.ram_price != 0, "Failed to retrieve ram_price from resource config");
      eosio::check(config.ram_fee != 0, "Failed to retrieve ram_fee from resource config");

      // Read values from the table
      double ram_price = config.ram_price;
      double ram_fee = (1.0 + config.ram_fee);
      uint64_t ram_purchase = (ram_price / ram_fee) * quant.amount;

      eosio::check(config.total_ram_available >= config.total_ram_used + ram_purchase, "Not enough RAM available");

      // modify the values and save them back to the table,
      config.total_ram_used += ram_purchase;
      config_table.set(config, get_self());

      // Allocate the RAM
      int64_t myRAM, myNET, myCPU;
      eosio::get_resource_limits(app, myRAM, myNET, myCPU);
      eosio::set_resource_limits(app, myRAM + ram_purchase, myNET, myNET);

      eosio::action(permission_level{dao_owner, "active"_n},
                    "eosio.token"_n,
                    "transfer"_n,
                    std::make_tuple(dao_owner, gov_name, quant, std::string("buy ram")))
          .send();
   }

   void bios::sellram(eosio::name dao_owner, eosio::name app, eosio::asset quant)
   {
      require_auth(app); // Check that the app has the necessary authorization

      // Access the account table from id.tmy.hpp
      eosiobiostonomy::bios::account_type_table account_type(get_self(), get_self().value);
      // Check the account type of the app
      auto itr = account_type.find(app.value);
      eosio::check(itr != account_type.end(), "Could not find account");
      eosio::check(itr->acc_type == eosiobiostonomy::enum_account_type::App, "Only apps can buy and sell RAM");

      // Check that the RAM is being purchased with the correct token
      eosio::check(quant.symbol == bios::system_resource_currency, "must buy ram with core token");

      // Check that the amount of bytes being sold is positive
      eosio::check(quant.amount > 0, "Amount must be positive");

      // Get the RAM price
      resource_config_table resource_config_singleton(get_self(), get_self().value);
      auto config = resource_config_singleton.get();

      // Read values from the table
      double ram_price = config.ram_price;
      double ram_fee = (1.0 + config.ram_fee);
      uint64_t ram_sold = ram_price * ram_fee * quant.amount;

      eosio::check(config.total_ram_used >= quant.amount, "Not enough RAM to sell");
      // Modify the values and save them back to the table
      config.total_ram_used -= quant.amount;
      resource_config_singleton.set(config, get_self());

      // Deallocate the RAM
      int64_t myRAM, myNET, myCPU;
      eosio::get_resource_limits(app, myRAM, myNET, myCPU);
      eosio::set_resource_limits(app, myRAM - quant.amount, myNET, myNET);

      // Transfer token and sell RAM
      eosio::action(permission_level{get_self(), "active"_n},
                    "eosio.token"_n,
                    "transfer"_n,
                    std::make_tuple(gov_name, dao_owner, eosio::asset(ram_sold, bios::system_resource_currency), std::string("sell ram")))
          .send();
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

   eosiobiostonomy::authority create_authority_with_key(const eosio::public_key &key)
   {
      eosiobiostonomy::key_weight new_key = eosiobiostonomy::key_weight{.key = key, .weight = 1};
      eosiobiostonomy::authority new_authority{.threshold = 1, .keys = {new_key}, .accounts = {}, .waits = {}};

      return new_authority;
   }

   // add the eosio.code permission to allow the account to call the smart contract properly
   // https://developers.eos.io/welcome/v2.1/smart-contract-guides/adding-inline-actions#step-1-adding-eosiocode-to-permissions
   eosiobiostonomy::permission_level create_eosio_code_permission_level(const name &account)
   {
      return eosiobiostonomy::permission_level(account, "eosio.code"_n);
   }

   void bios::newperson(
       checksum256 username_hash,
       public_key password_key,
       checksum256 password_salt)
   {
      // check the transaction is signed by the `id.tmy` account
      eosio::require_auth(get_self());

      // generate new random account name
      const name random_name = random_account_name(username_hash, password_salt, enum_account_type::Person);

      // use the password_key public key for the owner authority
      eosiobiostonomy::authority password_authority = create_authority_with_key(password_key);
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

      // Set the resource limits for the new account
      // uncomment this in task TODOS #77

      resource_config_table _resource_config(get_self(), get_self().value);
      auto config = _resource_config.get_or_create(get_self(), eosiobiostonomy::bios::resource_config());
      config.total_cpu_weight_allocated += this->initial_cpu_weight_allocation;
      config.total_net_weight_allocated += this->initial_net_weight_allocation;
      _resource_config.set(config, get_self());

      // Store the password_salt and hashed username in table
      _people.emplace(get_self(), [&](auto &people_itr)
                      {
           people_itr.account_name = random_name;
           people_itr.status = eosiobiostonomy::enum_account_status::Creating_Status;
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

   void bios::newapp(
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
      eosiobiostonomy::authority key_authority = create_authority_with_key(key);
      key_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      // If the account name exists, this will fail
      newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
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

      // Set the resource limits for the new app
      // uncomment this in task TODOS #77
      //  eosiobiostonomy::bios::resource_config_table _resource_config("eosio.bios"_n, "eosio.bios"_n.value);
      //  auto config = _resource_config.get_or_create(get_self(), eosiobiostonomy::bios::resource_config());
      //  config.total_cpu_weight_allocated = this->initial_cpu_weight_allocation;
      //  config.total_net_weight_allocated = this->initial_net_weight_allocation;
      //  _resource_config.set(config, get_self());

      // Store the password_salt and hashed username in table
      _apps.emplace(get_self(), [&](auto &app_itr)
                    {
                           app_itr.account_name = random_name;
                           app_itr.app_name = app_name;
                           app_itr.description = description;
                           app_itr.logo_url = logo_url;
                           app_itr.origin = origin;
                           app_itr.username_hash = username_hash; });

      // Store the account type in the account_type table
      account_type_table account_type(get_self(), get_self().value);
      account_type.emplace(get_self(), [&](auto &row)
                           {
         row.account_name = random_name;
         row.acc_type = enum_account_type::App;
         row.version = 1; });
   }

   void bios::setacctype(name account_name, account_type acc_type)
   {
      eosio::require_auth(get_self()); // signed by active@id.tmy permission

      account_type_table account_type(get_self(), get_self().value);

      auto itr = account_type.find(account_name.value);
      if (itr != account_type.end())
      {
         account_type.modify(itr, get_self(), [&](auto &row)
                             {
               row.acc_type = acc_type;
               row.version = 1; });
      }
      else
      {
         account_type.emplace(get_self(), [&](auto &row)
                              {
               row.account_name = account_name;
               row.acc_type = acc_type;
               row.version = 1; });
      }
   }

   void bios::updatekeyper(name account,
                           permission_level_name permission_level,
                           public_key key,
                           bool link_auth)
   {
      // eosio::require_auth(account); // this is not needed as eosiobiostonomy::bios::updateauth_action checks the permission

      // update the status if needed
      auto people_itr = _people.find(account.value);
      if (people_itr != _people.end())
      {
         if (people_itr->status == eosiobiostonomy::enum_account_status::Creating_Status)
         {
            _people.modify(people_itr, get_self(), [&](auto &people_itr)
                           { people_itr.status = eosiobiostonomy::enum_account_status::Active_Status; });
            eosio::set_resource_limits(account, 6000, this->initial_cpu_weight_allocation, this->initial_net_weight_allocation);
         }
      }

      // setup the new key authoritie(s)
      eosiobiostonomy::authority authority = create_authority_with_key(key);
      authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

      name permission;
      switch (permission_level)
      {
      case eosiobiostonomy::enum_permission_level_name::Pin:
         permission = "pin"_n;
         break;
      case eosiobiostonomy::enum_permission_level_name::Biometric:
         permission = "biometric"_n;
         break;
      case eosiobiostonomy::enum_permission_level_name::Local:
         permission = "local"_n;
         break;
      default:
         check(false, "Invalid permission level");
      }

      // must be signed by the account's permission_level or parent (from eosio.bios::updateauth())
      eosiobiostonomy::bios::updateauth_action updateauthaction("eosio"_n, {account, "owner"_n});
      updateauthaction.send(account, permission, "owner"_n, authority);

      if (link_auth)
      {
         // link the permission to the `loginwithapp` action
         linkauth_action linkauthaction("eosio"_n, {account, "owner"_n});
         linkauthaction.send(account, get_self(), "loginwithapp"_n, permission);
         // TODO also needs to link to any other actions that require the permission that we know of at this stage
      }
   }

   void bios::loginwithapp(
       name account,
       name app,
       name parent,
       public_key key)
   {
      // eosio::require_auth(account); // this is not needed as eosiobiostonomy::bios::updateauth_action checks the permission

      // check the app exists and is registered with status
      auto app_itr = _apps.find(app.value);
      check(app_itr != _apps.end(), "App does not exist");

      // TODO uncomment when apps have status
      // check(app_itr->status == eosiobiostonomy::enum_account_status::Active_Status, "App is not active");

      // TODO check parent is only from allowed parents : "local", "pin", "biometric", "active"

      // TODO instead of "app" as the permission, use sha256(parent, app, name of key(TODO provide as argument with default = "main"))

      // setup the new key authoritie(s)
      eosiobiostonomy::authority authority = create_authority_with_key(key);

      eosiobiostonomy::bios::updateauth_action updateauthaction("eosio"_n, {account, parent});
      updateauthaction.send(account, app, parent, authority);
      // must be signed by the account's permission_level or parent (from eosio.bios::updateauth())
   }

}
