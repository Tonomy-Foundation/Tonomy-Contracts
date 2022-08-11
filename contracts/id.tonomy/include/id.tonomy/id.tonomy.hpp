#pragma once

#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>

namespace idtonomy
{

   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::public_key;

   /**
    * @defgroup idtonomy id.tonomy
    * @ingroup tonomycontracts
    *
    * id.tonomy controls the identity management system for Tonomy ecosystems. It allows people and organizations
    * to create and manage their account, resources and smart contracts
    *
    * @{
    */
   class [[eosio::contract("id.tonomy")]] id : public eosio::contract
   {
   public:
      using contract::contract;
      /**
       * New account action
       *
       * @details Creates a new account.
       *
       * @param creator - the creator of the account
       */
      [[eosio::action]] void newperson(name creator);

      using newperson_action = action_wrapper<"newperson"_n, &id::newperson>;
   };
   /** @}*/ // end of @defgroup idtonomy id.tonomy
} /// namespace idtonomy
