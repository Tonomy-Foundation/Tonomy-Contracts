#!/bin/bash

# Get the input private key
read -s -p "Enter Private Key: " private_key

# Create a new EOSIO wallet (if not already created)
cleos wallet create -n mywallet --to-console

# Import the key into the EOSIO wallet
cleos wallet import -n mywallet --private-key $private_key

# Open terminal. Users can then do cleos commands easily with key and connected to mainnet
/bin/bash

