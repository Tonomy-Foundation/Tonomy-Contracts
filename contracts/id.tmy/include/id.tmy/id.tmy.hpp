#pragma once

#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>

namespace idtmy
{
   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::name;
   using eosio::print;
   using eosio::public_key;
   using std::string;

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

   enum enum_permission_level
   {
      Owner,
      Active,
      Password,
      Pin,
      Biometric,
      Local
   };
   typedef uint8_t permission_level;

   /**
    * @defgroup id.tmy id.tmy
    * @ingroup tonomycontracts
    *
    * id.tmy controls the identity management system for Tonomy ecosystems. It allows people and organizations
    * to create and manage their account, resources and smart contracts
    *
    * @{
    */
   class [[eosio::contract("id.tmy")]] id : public eosio::contract
   {
   private:
      uint64_t initial_cpu_weight_allocation = 1000;
      uint64_t initial_net_weight_allocation = 1000;

   public:
      using contract::contract;


      /**
       * Constructor for the contract, which initializes the _accounts table
       */
      id(name receiver, name code, eosio::datastream<const char *> ds);

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
                                          permission_level permission,
                                          public_key key,
                                          bool link_auth = false);

      /**
       * calls eosio::linkauth()
       *
       * @param account - the permission's owner to be linked and the payer of the RAM needed to store this link,
       * @param code - the owner of the action to be linked,
       * @param type - the action to be linked,
       * @param requirement - the permission to be linked.
       */
      [[eosio::action]] void linkauth(name account,
                                      name code,
                                      name type,
                                      name requirement);

      using newperson_action = action_wrapper<"newperson"_n, &id::newperson>;
      using updatekeyper_action = action_wrapper<"updatekeyper"_n, &id::updatekeyper>;
      using linkauth_action = action_wrapper<"linkauth"_n, &id::linkauth>;

      struct [[eosio::table]] account_type_struct {
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
   };
} /// namespace idtmy
