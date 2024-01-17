#!/bin/bash

# Get the input private key 
read -s -p "Enter Private Key: " private_key

# Import the key into the EOSIO wallet
cleos wallet import --private-key $private_key

# Get the wallet alias
read -s -p ""Enter blockchain API url (e.g. https://blockchain-api.pangea.web4.world): " alias_url

# create an alias so that it connects to pangea mainnet
alias cleos='cleos -u' $alias_url

# open terminal. user can then do cleos commands easily with key and connected to mainnet
/bin/bash