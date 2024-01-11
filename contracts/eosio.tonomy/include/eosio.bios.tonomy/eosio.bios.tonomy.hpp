#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

namespace eosiobiostonomy
{
   using eosio::action_wrapper;
   using eosio::asset;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::print;
   using eosio::public_key;
   using eosio::singleton;
   using std::string;

   struct permission_level_weight
   {
      permission_level permission;
      uint16_t weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
   };

   struct key_weight
   {
      eosio::public_key key;
      uint16_t weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(key_weight, (key)(weight))
   };

   struct wait_weight
   {
      uint32_t wait_sec;
      uint16_t weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
   };

   struct authority
   {
      uint32_t threshold = 0;
      std::vector<key_weight> keys;
      std::vector<permission_level_weight> accounts;
      std::vector<wait_weight> waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
   };

   struct block_header
   {
      uint32_t timestamp;
      name producer;
      uint16_t confirmed = 0;
      checksum256 previous;
      checksum256 transaction_mroot;
      checksum256 action_mroot;
      uint32_t schedule_version = 0;
      std::optional<eosio::producer_schedule> new_producers;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(block_header, (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(action_mroot)(schedule_version)(new_producers))
   };

   // Create an enum type and an eosio type for enums
   // https://eosio.stackexchange.com/questions/4950/store-enum-value-in-table
   enum enum_account_type
   {
      Person,
      Organization,
      App,
      Gov,
      Service
   };
   typedef uint8_t account_type;

   enum enum_account_status
   {
      Creating_Status,
      Active_Status,
      Deactivated_Status,
      Upgrading_Status
   };
   typedef uint8_t account_status;

   std::map<enum_account_type, char> account_type_letters = {
       {enum_account_type::Person, 'p'},
       {enum_account_type::App, 'a'},
       {enum_account_type::Organization, 'o'},
       {enum_account_type::Gov, 'g'},
       {enum_account_type::Service, 's'}};

   enum enum_permission_level_name
   {
      Owner,
      Active,
      Password,
      Pin,
      Biometric,
      Local
   };
   typedef uint8_t permission_level_name;

   /**
    * The `eosio.tonomy` is the first sample of system contract provided by `block.one` through the EOSIO platform. It is a minimalist system contract because it only supplies the actions that are absolutely critical to bootstrap a chain and nothing more. This allows for a chain agnostic approach to bootstrapping a chain.
    *
    * Just like in the `eosio.system` sample contract implementation, there are a few actions which are not implemented at the contract level (`newaccount`, `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, `canceldelay`, `onerror`, `setabi`, `setcode`), they are just declared in the contract so they will show in the contract's ABI and users will be able to push those actions to the chain via the account holding the `eosio.system` contract, but the implementation is at the EOSIO core level. They are referred to as EOSIO native actions.
    */
   class [[eosio::contract("eosio.tonomy")]] bios : public eosio::contract
   {
   private:
      uint64_t initial_cpu_weight_allocation = 1000;
      uint64_t initial_net_weight_allocation = 1000;

   public:
      using contract::contract;

      /**
       * Constructor for the contract, which initializes the _accounts table
       */
      bios(name receiver, name code, eosio::datastream<const char *> ds);

      static constexpr eosio::symbol system_resource_currency = eosio::symbol("ONO", 4);
      static constexpr eosio::name gov_name = "gov.tmy"_n;
      /**
       * New account action, called after a new account is created. This code enforces resource-limits rules
       * for new accounts as well as new account naming conventions.
       *
       * 1. accounts cannot contain '.' symbols which forces all acccounts to be 12
       * characters long without '.' until a future account auction process is implemented
       * which prevents name squatting.
       *
       * 2. new accounts must stake a minimal number of tokens (as set in system parameters)
       * therefore, this method will execute an inline buyram from receiver for newacnt in
       * an amount equal to the current new account creation fee.
       */
      [[eosio::action]] void newaccount(name creator,
                                        name name,
                                        ignore<authority> owner,
                                        ignore<authority> active);
      /**
       * Update authorization action updates pemission for an account.
       *
       * @param account - the account for which the permission is updated,
       * @param pemission - the permission name which is updated,
       * @param parem - the parent of the permission which is updated,
       * @param aut - the json describing the permission authorization.
       */
      [[eosio::action]] void updateauth(ignore<name> account,
                                        ignore<name> permission,
                                        ignore<name> parent,
                                        ignore<authority> auth);

      /**
       * Delete authorization action deletes the authorization for an account's permission.
       *
       * @param account - the account for which the permission authorization is deleted,
       * @param permission - the permission name been deleted.
       */
      [[eosio::action]] void deleteauth(ignore<name> account,
                                        ignore<name> permission);

      /**
       * Link authorization action assigns a specific action from a contract to a permission you have created. Five system
       * actions can not be linked `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, and `canceldelay`.
       * This is useful because when doing authorization checks, the EOSIO based blockchain starts with the
       * action needed to be authorized (and the contract belonging to), and looks up which permission
       * is needed to pass authorization validation. If a link is set, that permission is used for authoraization
       * validation otherwise then active is the default, with the exception of `eosio.any`.
       * `eosio.any` is an implicit permission which exists on every account; you can link actions to `eosio.any`
       * and that will make it so linked actions are accessible to any permissions defined for the account.
       *
       * @param account - the permission's owner to be linked and the payer of the RAM needed to store this link,
       * @param code - the owner of the action to be linked,
       * @param type - the action to be linked,
       * @param requirement - the permission to be linked.
       */
      [[eosio::action]] void linkauth(ignore<name> account,
                                      ignore<name> code,
                                      ignore<name> type,
                                      ignore<name> requirement);

      /**
       * Unlink authorization action it's doing the reverse of linkauth action, by unlinking the given action.
       *
       * @param account - the owner of the permission to be unlinked and the receiver of the freed RAM,
       * @param code - the owner of the action to be unlinked,
       * @param type - the action to be unlinked.
       */
      [[eosio::action]] void unlinkauth(ignore<name> account,
                                        ignore<name> code,
                                        ignore<name> type);

      /**
       * Cancel delay action cancels a deferred transaction.
       *
       * @param canceling_auth - the permission that authorizes this action,
       * @param trx_id - the deferred transaction id to be cancelled.
       */
      [[eosio::action]] void canceldelay(ignore<permission_level> canceling_auth, ignore<checksum256> trx_id)
      {
         require_auth(gov_name);
      }

      /**
       * Set code action sets the contract code for an account.
       *
       * @param account - the account for which to set the contract code.
       * @param vmtype - reserved, set it to zero.
       * @param vmversion - reserved, set it to zero.
       * @param code - the code content to be set, in the form of a blob binary..
       */
      [[eosio::action]] void setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code)
      {
         require_auth(gov_name);
      }

      /**
       * Set abi action sets the abi for contract identified by `account` name. Creates an entry in the abi_hash_table
       * index, with `account` name as key, if it is not already present and sets its value with the abi hash.
       * Otherwise it is updating the current abi hash value for the existing `account` key.
       *
       * @param account - the name of the account to set the abi for
       * @param abi     - the abi hash represented as a vector of characters
       */
      [[eosio::action]] void setabi(name account, const std::vector<char> &abi);

      /**
       * On error action, notification of this action is delivered to the sender of a deferred transaction
       * when an objective error occurs while executing the deferred transaction.
       * This action is not meant to be called directly.
       *
       * @param sender_id - the id for the deferred transaction chosen by the sender,
       * @param sent_trx - the deferred transaction that failed.
       */
      [[eosio::action]] void onerror(ignore<uint128_t> sender_id, ignore<std::vector<char>> sent_trx);

      /**
       * Set privilege action allows to set privilege status for an account (turn it on/off).
       * @param account - the account to set the privileged status for.
       * @param is_priv - 0 for false, > 0 for true.
       */
      [[eosio::action]] void setpriv(name account, uint8_t is_priv);

      /**
       * Sets the resource limits of an account
       *
       * @param account - name of the account whose resource limit to be set
       * @param ram_bytes - ram limit in absolute bytes
       * @param net_weight - fractionally proportionate net limit of available resources based on (weight / total_weight_of_all_accounts)
       * @param cpu_weight - fractionally proportionate cpu limit of available resources based on (weight / total_weight_of_all_accounts)
       */
      [[eosio::action]] void setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);

      /**
       * Set producers action, sets a new list of active producers, by proposing a schedule change, once the block that
       * contains the proposal becomes irreversible, the schedule is promoted to "pending"
       * automatically. Once the block that promotes the schedule is irreversible, the schedule will
       * become "active".
       *
       * @param schedule - New list of active producers to set
       */
      [[eosio::action]] void setprods(const std::vector<eosio::producer_authority> &schedule);

      /**
       * Set params action, sets the blockchain parameters. By tuning these parameters, various degrees of customization can be achieved.
       *
       * @param params - New blockchain parameters to set
       */
      [[eosio::action]] void setparams(const eosio::blockchain_parameters &params);

      /**
       * Require authorization action, checks if the account name `from` passed in as param has authorization to access
       * current action, that is, if it is listed in the action’s allowed permissions vector.
       *
       * @param from - the account name to authorize
       */
      [[eosio::action]] void reqauth(name from);

      /**
       * Activate action, activates a protocol feature
       *
       * @param feature_digest - hash of the protocol feature to activate.
       */
      [[eosio::action]] void activate(const eosio::checksum256 &feature_digest);

      /**
       * The setresparams action sets the price for RAM, the total RAM available, and the RAM fee.
       * It checks for an entry in the resource_config_table singleton, with `get_self()` as key.
       * If the entry does not exist, it creates a new entry with the provided RAM price, total RAM available, RAM fee, and sets other parameters to 0.
       * If the entry exists, it updates the RAM price, total RAM available, and RAM fee values for the existing `get_self()` key.
       *
       * @param ram_price - the new price of RAM
       * @param total_ram_available - the new total RAM available
       * @param ram_fee - the new RAM fee
       */
      [[eosio::action]] void setresparams(double ram_price, uint64_t total_ram_available, double ram_fee);

      /**
       * Buy RAM action allows an app to purchase RAM.
       * It checks the account type of the app, ensures the RAM is being purchased with the correct token,
       * and that the amount of tokens being used for the purchase is positive.
       * It then calculates the amount of RAM to purchase based on the current RAM price,
       * checks if there is enough available RAM, and allocates the purchased RAM to the app.
       * Finally, it updates the total RAM used and available in the system, and
       * transfers the tokens used for the purchase.
       *
       * @param dao_owner - the name of the DAO owner account
       * @param app - the name of the app account purchasing the RAM
       * @param quant - the amount and symbol of the tokens used for the purchase
       */
      [[eosio::action]] void buyram(const name &dao_owner, const name &app, const asset &quant);

      /**
       * Sell RAM action allows an app to sell RAM.
       * It checks the account type of the app, ensures the RAM is being sold for the correct token,
       * and that the amount of RAM being sold is positive.
       * It then calculates the amount of tokens to return based on the current RAM price,
       * checks if there is enough RAM being used by the app, and deallocates the sold RAM from the app.
       * Finally, it updates the total RAM used in the system, and
       * transfers the tokens from the sale.
       *
       * @param dao_owner - the name of the DAO owner account
       * @param app - the name of the app account selling the RAM
       * @param quant - the amount and symbol of the tokens used to sell
       */
      [[eosio::action]] void sellram(eosio::name dao_owner, eosio::name app, eosio::asset quant);

      /**
       * Require activated action, asserts that a protocol feature has been activated
       *
       * @param feature_digest - hash of the protocol feature to check for activation.
       */
      [[eosio::action]] void reqactivated(const eosio::checksum256 &feature_digest);

      /**
       * Create a new account for a person
       *
       * @details Creates a new account for a person.
       *
       * @param username_hash - hash of the username of the account
       * @param password_key - public key generated from the account's password
       * @param password_salt - salt used to generate the password_key with the password
       */
      [[eosio::action]] void newperson(
          checksum256 username_hash,
          public_key password_key,
          checksum256 password_salt);

      /**
       * Sets the account type for a given account
       *
       * @param account_name - name of the account
       * @param acc_type - account type to be set
       */
      [[eosio::action]] void setacctype(name account_name, account_type acc_type);

      /**
       * Create a new account for an app and registers it's details
       *
       * @details Creates a new account for an app and registers it's details.
       *
       * @param name - name of the app
       * @param description - description of the app
       * @param username_hash - hash of the username
       * @param logo_url - url to the logo of the app
       * @param origin - domain associated with the app
       * @param password_key - public key generated from the account's password
       */
      [[eosio::action]] void newapp(
          string app_name,
          string description,
          checksum256 username_hash,
          string logo_url,
          string origin,
          public_key key);

      /**
       * Adds a new key to a person's account to log into an app with
       *
       * @param account - account of the person
       * @param app - account of the app to authorize the key to
       * @param key - public key to authorize
       */
      [[eosio::action]] void loginwithapp(
          name account,
          name app,
          name parent,
          public_key key);

      /**
       * Update a key of a person
       *
       * @param account - name of the account to update
       * @param permission - permission level of the key to update
       * @param key - public key to update
       */
      [[eosio::action]] void updatekeyper(name account,
                                          permission_level_name permission,
                                          public_key key,
                                          bool link_auth = false);

      struct [[eosio::table]] account_type_struct
      {
         name account_name;
         account_type acc_type;
         uint16_t version; // used for upgrading the account structure

         uint64_t primary_key() const { return account_name.value; }
         EOSLIB_SERIALIZE(struct account_type_struct, (account_name)(acc_type)(version))
      };

      typedef eosio::multi_index<"acctypes"_n, account_type_struct> account_type_table;

      TABLE person
      {
         name account_name;
         account_status status;
         checksum256 username_hash;
         checksum256 password_salt;

         // primary key automatically added by EOSIO method
         uint64_t primary_key() const { return account_name.value; }
         // also index by username hash
         checksum256 index_by_username_hash() const { return username_hash; }
      };

      // Create a multi-index-table with two indexes
      typedef eosio::multi_index<"people"_n, person,
                                 eosio::indexed_by<"usernamehash"_n, eosio::const_mem_fun<person, checksum256, &person::index_by_username_hash>>>
          people_table;

      // Create an instance of the table that can is initalized in the constructor
      people_table _people;

      TABLE app
      {
         name account_name;
         string app_name;
         checksum256 username_hash;
         string description;
         string logo_url;
         string origin;

         // primary key automatically added by EOSIO method
         uint64_t primary_key() const { return account_name.value; }
         // also index by username hash
         checksum256 index_by_username_hash() const { return username_hash; }
         checksum256 index_by_origin_hash() const { return eosio::sha256(origin.c_str(), std::strlen(origin.c_str())); }
      };

      // Create a multi-index-table with two indexes
      typedef eosio::multi_index<"apps"_n, app,
                                 eosio::indexed_by<"usernamehash"_n,
                                                   eosio::const_mem_fun<app, checksum256, &app::index_by_username_hash>>,
                                 eosio::indexed_by<"originhash"_n,
                                                   eosio::const_mem_fun<app, checksum256, &app::index_by_origin_hash>>>
          apps_table;

      // Create an instance of the table that can is initalized in the constructor
      apps_table _apps;

      /**
       * Returns the account name of the app that corresponds to the origin
       *
       * @param {string} origin - the origin of the app
       * @example "https://www.tonomy.com"
       * @param {name} [contract_name] - the name of the contract to query
       * @returns {name} - the account name of the app
       */
      static const name get_app_permission_by_origin(string origin, name contract_name = "id.tmy"_n)
      {
         apps_table id_apps = apps_table(contract_name, contract_name.value);
         auto apps_by_origin_hash_itr = id_apps.get_index<"originhash"_n>();

         eosio::checksum256 origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
         const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
         check(origin_itr == apps_by_origin_hash_itr.end(), "No app with this origin found");

         return origin_itr->account_name;
      }

      /**
       * Returns the account name of the app that corresponds to the origin
       *
       * @param {string} username - the username of the app
       * @example "demo.app.tonomy.id"
       * @param {name} [contract_name] - the name of the contract to query
       * @returns {name} - the account name of the app
       */
      static const name get_app_permission_by_username(string username, name contract_name = "id.tmy"_n)
      {
         apps_table id_apps = apps_table(contract_name, contract_name.value);
         auto apps_by_username_hash_itr = id_apps.get_index<"usernamehash"_n>();

         eosio::checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
         const auto username_itr = apps_by_username_hash_itr.find(username_hash);
         check(username_itr == apps_by_username_hash_itr.end(), "No app with this username found");

         return username_itr->account_name;
      }

      struct [[eosio::table]] abi_hash
      {
         name owner;
         checksum256 hash;
         uint64_t primary_key() const { return owner.value; }

         EOSLIB_SERIALIZE(abi_hash, (owner)(hash))
      };

      typedef eosio::multi_index<"abihash"_n, abi_hash> abi_hash_table;

      struct [[eosio::table]] resource_config
      {
         double ram_fee;                      // RAM fee fraction (0.01 = 1% fee)
         double ram_price;                    // RAM price in tokens/byte
         uint64_t total_ram_available;        // Total available RAM in bytes
         uint64_t total_ram_used;             // Total RAM used in bytes
         uint64_t total_cpu_weight_allocated; // Total allocated CPU weight
         uint64_t total_net_weight_allocated; // Total allocated NET weight

         EOSLIB_SERIALIZE(resource_config, (ram_price)(total_ram_available)(total_ram_used)(total_cpu_weight_allocated)(total_net_weight_allocated)(ram_fee))
      };
      typedef eosio::singleton<"resconfig"_n, resource_config> resource_config_table;

      using newaccount_action = action_wrapper<"newaccount"_n, &bios::newaccount>;
      using updateauth_action = action_wrapper<"updateauth"_n, &bios::updateauth>;
      using deleteauth_action = action_wrapper<"deleteauth"_n, &bios::deleteauth>;
      using linkauth_action = action_wrapper<"linkauth"_n, &bios::linkauth>;
      using unlinkauth_action = action_wrapper<"unlinkauth"_n, &bios::unlinkauth>;
      using canceldelay_action = action_wrapper<"canceldelay"_n, &bios::canceldelay>;
      using setcode_action = action_wrapper<"setcode"_n, &bios::setcode>;
      using setabi_action = action_wrapper<"setabi"_n, &bios::setabi>;
      using setpriv_action = action_wrapper<"setpriv"_n, &bios::setpriv>;
      using setalimits_action = action_wrapper<"setalimits"_n, &bios::setalimits>;
      using setprods_action = action_wrapper<"setprods"_n, &bios::setprods>;
      using setparams_action = action_wrapper<"setparams"_n, &bios::setparams>;
      using reqauth_action = action_wrapper<"reqauth"_n, &bios::reqauth>;
      using activate_action = action_wrapper<"activate"_n, &bios::activate>;
      using reqactivated_action = action_wrapper<"reqactivated"_n, &bios::reqactivated>;
      using setresourceparams_action = action_wrapper<"setresparams"_n, &bios::setresparams>;
      using buyram_action = action_wrapper<"buyram"_n, &bios::buyram>;

      using newperson_action = action_wrapper<"newperson"_n, &bios::newperson>;
      using updatekeyper_action = action_wrapper<"updatekeyper"_n, &bios::updatekeyper>;
   };
}

void throwError(std::string error_code, std::string message)
{
   eosio::check(false, error_code + ": " + message);
}

/*
List of which files have which error codes, to avoid duplicates:export function throwError(string error_code, string message) {

TCON10## = id.tmy.cpp
*/
