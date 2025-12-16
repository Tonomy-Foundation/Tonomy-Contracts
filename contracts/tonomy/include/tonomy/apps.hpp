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
#include "native.hpp"

namespace tonomysystem
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
      * Manually sets the details of an app (admin only)
      *
      * @param account_name - name of the account
      * @param json_data - JSON string containing app details (name,description, logo_url, background_color, accent_color)
      * @param username_hash - hash of the username
      * @param origin - domain associated with the app
      */
      [[eosio::action]] void adminsetapp(
            name account_name,
            string json_data,
            checksum256 username_hash,
            string origin);
      
      /**
       * Removes an app (admin only)
       * @param account_name - name of the account
       */
      [[eosio::action]] void deleteapp(name account_name);

      /**
       * Create a new account for an app and registers its details
       *
       * @param json_data - JSON string containing app details (name,description, logo_url, background_color, accent_color)
       * @param username_hash - Hash of the username
       * @param origin - Domain associated with the app
       * @param key - Public key generated from the account's password
       */
      [[eosio::action]] void newapp(
          string json_data,
          checksum256 username_hash,
          string origin,
          public_key key);
    
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
       * Delete all the old apps
       */
       [[eosio::action]] void eraseoldapps();

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

      struct [[eosio::table]] app
      {
         name account_name;
         string app_name;
         checksum256 username_hash;
         string description;
         string logo_url;
         string origin;

         uint64_t primary_key() const { return account_name.value; }
         checksum256 index_by_username_hash() const { return username_hash; }
         checksum256 index_by_origin_hash() const { return eosio::sha256(origin.c_str(), std::strlen(origin.c_str())); }
      };

      typedef eosio::multi_index<"apps"_n, app,
                                 eosio::indexed_by<"usernamehash"_n,
                                                   eosio::const_mem_fun<app, checksum256, &app::index_by_username_hash>>,
                                 eosio::indexed_by<"originhash"_n,
                                                   eosio::const_mem_fun<app, checksum256, &app::index_by_origin_hash>>>
          apps_table;

      apps_table _apps;

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
         uint32_t ram_purchased_mb;   // purchased RAM in MB
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
      static const name get_app_permission_by_username(string username, name contract_name = "tonomy"_n)
      {
         apps_table id_apps = apps_table(contract_name, contract_name.value);
         auto apps_by_username_hash_itr = id_apps.get_index<"usernamehash"_n>();

         eosio::checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
         const auto username_itr = apps_by_username_hash_itr.find(username_hash);
         check(username_itr == apps_by_username_hash_itr.end(), "No app with this username found");

         return username_itr->account_name;
      }

      using newapp_action = action_wrapper<"newapp"_n, &apps::newapp>;
      using loginwithapp_action = action_wrapper<"loginwithapp"_n, &apps::loginwithapp>;
      using adminsetapp_action = action_wrapper<"adminsetapp"_n, &apps::adminsetapp>;
      using eraseoldapps_action = action_wrapper<"eraseoldapps"_n, &apps::eraseoldapps>;
      using buyram_action = action_wrapper<"buyram"_n, &apps::buyram>;
      using sellram_action = action_wrapper<"sellram"_n, &apps::sellram>;
   
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