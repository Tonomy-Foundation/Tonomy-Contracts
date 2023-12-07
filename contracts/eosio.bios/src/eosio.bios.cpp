#include <eosio.bios/eosio.bios.hpp>

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

void bios::setpriv( name account, uint8_t is_priv ) {
   require_auth( get_self() );
   set_privileged( account, is_priv );
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

   // Set the system resource currency to oncoin.tmy
   eosio::symbol system_resource_currency = eosio::symbol("ONO", 4);
   set_resource_currency(system_resource_currency);

   // Set the resource token to oncoin.tmy
   eosio::name resource_token_contract = eosio::name("onocoin.tmy");
   set_resource_token(resource_token_contract);
}

void bios::reqauth( name from ) {
   require_auth( from );
}

void bios::activate( const eosio::checksum256& feature_digest ) {
   require_auth( get_self() );
   preactivate_feature( feature_digest );
}

void bios::reqactivated( const eosio::checksum256& feature_digest ) {
   check( is_feature_activated( feature_digest ), "protocol feature is not activated" );
}

void bios::setramprice(double new_price) {
   require_auth("gov.tmy"_n); // check authorization is gov.tmy

   // check new_price is within bounds, not negative or too high
   eosio::check(new_price >= 0, "New price must be non-negative");

   resource_config_table resource_configs(get_self(), get_self().value);
   auto itr = resource_configs.find(get_self().value);

   if (itr == resource_configs.end()) {
      // If record not found, create a new one
      resource_configs.emplace(get_self(), [&](auto& row) {
         row.ram_price = new_price;
         row.total_ram_available = 0;
         row.total_ram_used = 0;
      });
    } else {
      // Modify existing record
      resource_configs.modify(itr, get_self(), [&](auto& row) {
         row.ram_price = new_price;
      });
    }
}

}


void bios::buyram(eosio::name dao_owner, eosio::name app, eosio::asset quant) {
    require_auth(app); // Check that the app has the necessary authorization

    // Check the account type of the app
    auto itr = _accounts.find(app.value);
    eosio::check(itr != _accounts.end(), "Could not find account");
    eosio::check(itr->type == enum_account_type::App, "Only apps can buy and sell RAM");

    // Check that the RAM is being purchased with the correct token
    eosio::check(quant.symbol == core_symbol(), "must buy ram with core token");

    // Check that the amount of tokens being used for the purchase is positive
    eosio::check(quant.amount > 0, "Amount must be positive");

    // Get the RAM price
    auto resource_itr = _resource_config.find(get_self().value);
    double ram_price = resource_itr->ram_price;
    uint64_t ram_purchase = ram_price * quant.amount;
    eosio::check(resource_itr->total_ram_available >= resource_itr->total_ram_used + ram_purchase, "Not enough RAM available");

    // Allocate the RAM
    int64_t myRAM, myCPU, myNET;
    eosio::get_resource_limits(app, &myRAM, &myCPU, &myNET);
    eosio::set_resource_limits(app, myRAM + ram_purchase, myCPU, myNET);

    // Update total ram used and available
    _resource_config.modify(resource_itr, get_self(), [&](auto& row) {
        row.total_ram_used += ram_purchase;
    });

    // Transfer token and buy ram
    eosio::action(
        eosio::permission_level{app, "active"_n},
        "onocoin.tmy"_n,
        "transfer"_n,
        std::make_tuple(app, "gov.tmy"_n, quant, std::string("buy ram"))
    ).send();
}