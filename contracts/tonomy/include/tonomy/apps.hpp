#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <cstring>
#include <vector>
#include "native.hpp"

namespace tonomysystem
{
   using eosio::action_wrapper;
   using eosio::asset;
   using eosio::check;
   using eosio::checksum256;

   // Constants
   static constexpr eosio::name app_controller_account = "gov.tmy"_n;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::print;
   using eosio::public_key;
   using eosio::singleton;
   using std::string;

   using eosiotonomy::authority;

   /**
    * The `eosio.tonomy` is the first sample of system contract provided by `block.one` through the EOSIO platform. It is a minimalist system contract because it only supplies the actions that are absolutely critical to bootstrap a chain and nothing more. This allows for a chain agnostic approach to bootstrapping a chain.
    *
    * Just like in the `eosio.system` sample contract implementation, there are a few actions which are not implemented at the contract level (`newaccount`, `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, `canceldelay`, `onerror`, `setabi`, `setcode`), they are just declared in the contract so they will show in the contract's ABI and users will be able to push those actions to the chain via the account holding the `eosio.system` contract, but the implementation is at the EOSIO core level. They are referred to as EOSIO native actions.
    */
   class [[eosio::contract("tonomy")]] apps : public native
   {
   public:
      static constexpr eosio::name app_controller_account = "gov.tmy"_n;

      uint64_t initial_cpu_weight_allocation = 1000;
      uint64_t initial_net_weight_allocation = 1000;

      /**
       * Constructor for the contract, which initializes the _accounts table
       */
      apps(name receiver, name code, eosio::datastream<const char *> ds);

        /**
         * Create a new app account with random name and register its details (sets plan = basic)
         *
         * @param creator - the account name of the creator (used for auth and active permission)
         * @param json_data - JSON with display details: app_name, description, logo_url, background_color, accent_color
         * @param username - raw username string (e.g., "coolapp" or "@coolapp"); must be unique
         * @param origin - domain or origin associated with the app; must be unique
         */
        [[eosio::action]] void appcreate(
           name creator,
           string json_data,
           string username,
           string origin);

        /**
         * Update app data
         *
         * @param account_name - the app account name
         * @param json_data - updated JSON data (full)
         * @param username - new raw username; must be unique if changed
         */
        [[eosio::action]] void appupdate(
           name account_name,
           string json_data,
           string username);

        /**
         * Update the app subscription plan
         *
         * @param account_name - the app account name
         * @param plan - subscription plan enum: 0 = basic, 1 = pro
         */
        [[eosio::action]] void appupdplan(
           name account_name,
           uint8_t plan);
    
      /**
       * Adds a new key to a person's account to log into an app with
       *
       * @param account - account of the person
       * @param app - account of the app to authorize the key to
       * @param parent - parent permission of the new permission
       * @param key - public key to authorize
       */
      [[eosio::action]] void loginwithapp(
          name account,
          name app,
          name parent,
          public_key key);


        /**
         * Deploy initial smart contract code, ABI, and metadata for an app (sets version = 1)
         *
         * @param account_name - the app account name
         * @param vmtype - WebAssembly VM type (as in `setcode`)
         * @param vmversion - WebAssembly VM version (as in `setcode`)
         * @param code - WASM code bytes for the contract
         * @param abi - ABI bytes for the contract
         * @param source_code_url - optional URL to the contract source code (empty for none)
         */
        [[eosio::action]] void scdeploy(
           name account_name,
           uint8_t vmtype,
           uint8_t vmversion,
           const std::vector<char> &code,
           const std::vector<char> &abi,
           string source_code_url);

        /**
         * Update smart contract code, ABI, and/or metadata for an app (increments version)
         *
         * @param account_name - the app account name
         * @param vmtype - WebAssembly VM type (as in `setcode`)
         * @param vmversion - WebAssembly VM version (as in `setcode`)
         * @param code - WASM code bytes for the contract (empty vector to skip)
         * @param abi - ABI bytes for the contract (empty vector to skip)
         * @param source_code_url - optional new source code URL (empty to leave unchanged)
         */
        [[eosio::action]] void scupdate(
           name account_name,
           uint8_t vmtype,
           uint8_t vmversion,
           const std::vector<char> &code,
           const std::vector<char> &abi,
           string source_code_url);

        /**
         * Buy RAM for the app's smart contract using core tokens
         *
         * @param account_name - the app account name
         * @param quant - amount of core tokens to spend for RAM
         */
        [[eosio::action]] void scbuyram(
           const name &account_name,
           const asset &quant);

        /**
         * Sell RAM from the app's smart contract and return core tokens
         *
         * @param account_name - the app account name
         * @param quant - amount of core tokens to sell for RAM reduction
         */
        [[eosio::action]] void scsellram(
           const name &account_name,
           const asset &quant);

        /**
         * Add a new key to the app account's active permission
         *
         * @param account_name - the app account name
         * @param key - the new public key to add
         */
        [[eosio::action]] void appaddkey(
           name account_name,
           public_key key);

        /**
         * Remove a key from the app account's active permission
         *
         * @param account_name - the app account name
         * @param key - the public key to remove
         */
        [[eosio::action]] void appremkey(
           name account_name,
           public_key key);

      struct [[eosio::table]] appv2 
      {
         name account_name;
         string json_data; // JSON string containing app details
         // {
         //    app_name: string;
         //    description: string;
         //    logo_url: string;
         //    background_color: string; // hex string starting with #
         //    accent_color: string; // hex string starting with #
         // };
         uint16_t version; // Version number to track schema changes
         checksum256 username_hash;
         string origin;

         uint64_t primary_key() const { return account_name.value; }
         checksum256 index_by_username_hash() const { return username_hash; }
         checksum256 index_by_origin_hash() const { return eosio::sha256(origin.c_str(), std::strlen(origin.c_str())); }
      };

       // Create a multi-index-table with two indexes
       typedef eosio::multi_index<"appsv2"_n, appv2,
       eosio::indexed_by<"usernamehash"_n,
                         eosio::const_mem_fun<appv2, checksum256, &appv2::index_by_username_hash>>,
       eosio::indexed_by<"originhash"_n,
                         eosio::const_mem_fun<appv2, checksum256, &appv2::index_by_origin_hash>>>
      appsv2_table;

      // Create an instance of the table that can is initalized in the constructor
      appsv2_table _appsv2;

      // ----------------------------------------------------------------------
      // Apps V3
      // ----------------------------------------------------------------------
      // Plan enum for subscription tiers
      enum plan_t : uint8_t {
         plan_basic = 0,
         plan_pro = 1
      };

      struct [[eosio::table]] appv3
      {
         name account_name;           // app account name
         string json_data;            // JSON string containing app details:
         // {
         //    app_name: string;
         //    description: string;
         //    logo_url: string;
         //    background_color: string; // hex string starting with #
         //    accent_color: string; // hex string starting with #
         // };
         uint16_t version;            // schema/data version
         string username;             // raw username string (e.g., "coolapp")
         string origin;               // app domain
         uint8_t plan;                // subscription plan (0 = basic, 1 = pro)

         uint64_t primary_key() const { return account_name.value; }
         checksum256 index_by_username_hash() const { return eosio::sha256(username.c_str(), std::strlen(username.c_str())); }
         checksum256 index_by_origin_hash() const { return eosio::sha256(origin.c_str(), std::strlen(origin.c_str())); }
      };

      // Multi-index for appsv3 with indexes on username (via hash) and origin
      typedef eosio::multi_index<"appsv3"_n, appv3,
         eosio::indexed_by<"usernamehash"_n,
            eosio::const_mem_fun<appv3, checksum256, &appv3::index_by_username_hash>>,
         eosio::indexed_by<"originhash"_n,
            eosio::const_mem_fun<appv3, checksum256, &appv3::index_by_origin_hash>>
      > appsv3_table;

      // Instance to be initialized in the constructor
      appsv3_table _appsv3;

      // ----------------------------------------------------------------------
      // Smart Contract info per app
      // ----------------------------------------------------------------------
      struct [[eosio::table]] smartcontract
      {
         name account_name;           // app account
         uint16_t version;            // deployment/version number
         string source_code_url;      // optional: empty string if not set

         uint64_t primary_key() const { return account_name.value; }
      };

      typedef eosio::multi_index<"appscntrct"_n, smartcontract> smartcontract_table;

      smartcontract_table _smartcontracts;

      /**
       * Returns the account name of the app that corresponds to the origin
       *
       * @param {string} origin - the origin of the app
       * @example "https://www.tonomy.com"
       * @param {name} [contract_name] - the name of the contract to query
       * @returns {name} - the account name of the app
       */
      static const name get_app_permission_by_origin(string origin, name contract_name = "id.tmy"_n);

      /**
       * Returns the account name of the app that corresponds to the username
       *
       * @param {string} username - the username of the app
       * @example "demo.app.tonomy.id" or "@coolapp"
       * @param {name} [contract_name] - the name of the contract to query
       * @returns {name} - the account name of the app
       */
      static const name get_app_permission_by_username(string username, name contract_name = "tonomy"_n);

      using loginwithapp_action = action_wrapper<"loginwithapp"_n, &apps::loginwithapp>;

        // Action wrappers
        using appcreate_action = action_wrapper<"appcreate"_n, &apps::appcreate>;
        using appupdate_action = action_wrapper<"appupdate"_n, &apps::appupdate>;
        using appupdplan_action = action_wrapper<"appupdplan"_n, &apps::appupdplan>;
        using scdeploy_action = action_wrapper<"scdeploy"_n, &apps::scdeploy>;
        using scupdate_action = action_wrapper<"scupdate"_n, &apps::scupdate>;
        using scbuyram_action = action_wrapper<"scbuyram"_n, &apps::scbuyram>;
        using scsellram_action = action_wrapper<"scsellram"_n, &apps::scsellram>;
        using appaddkey_action = action_wrapper<"appaddkey"_n, &apps::appaddkey>;
        using appremkey_action = action_wrapper<"appremkey"_n, &apps::appremkey>;

        /**
         * Admin: create or set an app record
         *
         * @param json_data - JSON with display details
         * @param username - raw username (unique)
         * @param origin - domain (unique)
         */
        [[eosio::action]] void admncrtapp(
           string json_data,
           string username,
           string origin);

        /**
         * Admin: update an app record
         *
         * @param account_name - the app account name
         * @param json_data - JSON with display details
         * @param username - raw username (unique)
         * @param origin - domain (unique)
         * @param plan - subscription plan enum: 0 = basic, 1 = pro
         */
        [[eosio::action]] void admnupdapp(
           name account_name,
           string json_data,
           string username,
           string origin,
           uint8_t plan);

        /**
         * Admin: delete an app record
         *
         * @param account_name - the app account name
         */
        [[eosio::action]] void admndelapp(
           name account_name);

        /**
         * Admin: migrate a single app from V2
         *
         * @param account_name - the app account name to migrate
         * @param username - raw username string (e.g., "coolapp" or "@coolapp")
         * @param plan - subscription plan enum: 0 = basic, 1 = pro (default to basic)
         * @param key - public key to initialize/attach to the app during migration
         */
        [[eosio::action]] void admnmigapp(
           name account_name,
           string username,
           uint8_t plan,
           public_key key);

        /**
         * Admin: migrate smart contract metadata for an app
         * Note: RAM info is fetched from get_resource_limits for the account
         *
         * @param account_name - the app account name
         * @param source_code_url - optional URL to the contract source code
         */
        [[eosio::action]] void admnmigsc(
           name account_name,
           string source_code_url);

        using admncrtapp_action = action_wrapper<"admncrtapp"_n, &apps::admncrtapp>;
        using admnupdapp_action = action_wrapper<"admnupdapp"_n, &apps::admnupdapp>;
        using admndelapp_action = action_wrapper<"admndelapp"_n, &apps::admndelapp>;
        using admnmigapp_action = action_wrapper<"admnmigapp"_n, &apps::admnmigapp>;
        using admnmigsc_action = action_wrapper<"admnmigsc"_n, &apps::admnmigsc>;
   
      private:
      /**
       * Check if the app username is already taken
       *
       * @param username_hash - hash of the username of the account
       */    
      void check_app_username(const checksum256 &username_hash);
    
      /**
       * Check if the app origin is already taken
       *
       * @param origin - domain associated with the app
       */
      void check_app_origin(const string &origin);

      /**
       * Check if the raw username is already taken in appsv3
       *
       * @param username - raw username string (may include leading '@')
       */
      void check_app_username_v3(const string &username)
      {
         // Compute hash index from raw username for efficient lookup
         checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
         auto idx = _appsv3.get_index<"usernamehash"_n>();
         auto itr = idx.find(username_hash);
         check(itr == idx.end(), "Username already taken");
      }
   };
}