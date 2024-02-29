#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

echo "Deleting built smart contracts"

CONTRACTS=(
    "demo.tmy"
    "eosio.bios"
    "eosio.boot"
    "eosio.msig"
    "eosio.token"
    "eosio.tonomy"
    "tonomy"
    "vesting.token"
)
for CONTRACT in "${CONTRACTS[@]}"
do
    echo "Deleting smart contract ${CONTRACT} .wasm and .abi files"
    rm -f "${PARENT_PATH}/contracts/${CONTRACT}/${CONTRACT}.wasm"
    rm -f "${PARENT_PATH}/contracts/${CONTRACT}/${CONTRACT}.abi"
done