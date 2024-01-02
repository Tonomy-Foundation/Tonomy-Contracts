#include <eosio.bios/eosio.bios.hpp>
#include <eosio/symbol.hpp>
#include <id.tmy/id.tmy.hpp>

namespace eosiobios {
void bios::setabi( name account, const std::vector<char>& abi ) {
   abi_hash_table table(get_self(), get_self().value);
   auto itr = table.find( account.value );
   if( itr == table.end() ) {
      table.emplace( account, [&]( auto& row ) {
         row.owner = account;
         row.hash  = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
      });
   } else {
      table.modify( itr, eosio::same_payer, [&]( auto& row ) {
         row.hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
      });
   }
}

void bios::onerror( ignore<uint128_t>, ignore<std::vector<char>> ) {
   check( false, "the onerror action cannot be called directly" );
}

void bios::setalimits( name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
   require_auth( get_self() );
   set_resource_limits( account, ram_bytes, net_weight, cpu_weight );
}

void bios::setprods( const std::vector<eosio::producer_authority>& schedule ) {
   require_auth( get_self() );
   set_proposed_producers( schedule );
}

void bios::setparams( const eosio::blockchain_parameters& params ) {
   require_auth( get_self() );
   set_blockchain_parameters( params );
}


void bios::reqauth( name from ) {
   require_auth( from );
}

void bios::setpriv( name account, uint8_t is_priv ) {
   require_auth( get_self() );
   set_privileged( account, is_priv );
}

void bios::activate( const eosio::checksum256& feature_digest ) {
   require_auth( get_self() );
   preactivate_feature( feature_digest );
}

void bios::reqactivated( const eosio::checksum256& feature_digest ) {
   check( is_feature_activated( feature_digest ), "protocol feature is not activated" );
}


void bios::setresparams(double ram_price, uint64_t total_ram_available, double ram_fee) {
   require_auth(gov_name); // check authorization is gov.tmy

   // check ram_price is within bounds, not negative or too high
   eosio::check(ram_price >= 0, "RAM price must be non-negative");

   // check ram_fee is non-negative
   eosio::check(ram_fee >= 0, "RAM fee must be non-negative");

   resource_config_table resource_config_singleton(get_self(), get_self().value);
   resource_config config;

   if (resource_config_singleton.exists()) {
      // Singleton exists, get the existing config and modify the values
      config = resource_config_singleton.get();
      config.ram_price = ram_price;
      config.total_ram_available = total_ram_available;
      config.ram_fee = ram_fee;
   } else {
      // Singleton does not exist, set the values and also set other values to 0
      config = resource_config{ram_price, ram_fee, total_ram_available, 0, 0, 0};
   }

   // Save the modified or new config back to the singleton
   resource_config_singleton.set(config, get_self());
}

void bios::buyram(const name& dao_owner, const name& app, const asset& quant) {
    require_auth(app); // Check that the app has the necessary authorization

   // Access the account table from id.tmy.hpp
    idtmy::id::account_type_table account_type(idtmy_name, idtmy_name.value);
   // Check the account type of the app
   auto itr = account_type.find(app.value);
   eosio::check(itr != account_type.end(), "Could not find account");
   eosio::check(itr->acc_type == idtmy::enum_account_type::App, "Only apps can buy and sell RAM");

   // Check that the RAM is being purchased with the correct token
   eosio::check(quant.symbol == bios::system_resource_currency, "must buy ram with core token");

   // Check that the amount of tokens being used for the purchase is positive
   eosio::check(quant.amount > 0, "Amount must be positive");
  
    // Get the RAM price
   resource_config_table resource_config_singleton(get_self(), get_self().value);
    auto config = resource_config_singleton.get();

    // Read values from the table
    double ram_price = config.ram_price;
    double ram_fee = (1.0 + config.ram_fee);
    uint64_t ram_purchase = (ram_price / ram_fee) * quant.amount;

    eosio::check(config.total_ram_available >= config.total_ram_used + ram_purchase, "Not enough RAM available");

    // modify the values and save them back to the table,
    config.total_ram_used += ram_purchase;
    resource_config_singleton.set(config, get_self());
    
    //Allocate the RAM
    int64_t myRAM, myNET, myCPU;
    eosio::get_resource_limits( app, myRAM, myNET, myCPU );
    eosio::set_resource_limits(app, myRAM + ram_purchase, myNET, myNET);


   //Transfer token and buy ram
     eosio::action(permission_level{get_self(), "active"_n},
               "onocoin.tmy"_n,
               "transfer"_n,
        std::make_tuple(dao_owner, gov_name, quant, std::string("buy ram")))
            .send();
}

void bios::sellram(eosio::name dao_owner, eosio::name app, eosio::asset quant) {
    require_auth(app); // Check that the app has the necessary authorization

   // Access the account table from id.tmy.hpp
    idtmy::id::account_type_table account_type("id.tmy"_n, "id.tmy"_n.value);
    // Check the account type of the app
    auto itr = account_type.find(app.value);
    eosio::check(itr != account_type.end(), "Could not find account");
    eosio::check(itr->acc_type == idtmy::enum_account_type::App, "Only apps can buy and sell RAM");

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
    uint64_t ram_sold  = ram_price * ram_fee * quant.amount;

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
                  "onocoin.tmy"_n,
                  "transfer"_n,
                  std::make_tuple(gov_name, dao_owner, eosio::asset(ram_sold  , bios::system_resource_currency), std::string("sell ram")))
        .send();
}

}