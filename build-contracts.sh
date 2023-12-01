#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

echo "Building smart contracts"

cd "${PARENT_PATH}/contracts/eosio.boot"
if [ -e eosio.boot.wasm ]
then
    echo "eosio.boot already built"
else
    ./build.sh
fi

cd "${PARENT_PATH}/contracts/eosio.bios"
if [ -e eosio.bios.wasm ]
then
    echo "eosio.bios already built"
else
    ./build.sh
fi

cd "${PARENT_PATH}/contracts/eosio.bios.tonomy"
if [ -e eosio.bios.tonomy.wasm ]
then
    echo "eosio.bios.tonomy already built"
else
    ./build.sh
fi

cd "${PARENT_PATH}/contracts/demo.tmy"
if [ -e demo.tmy.wasm ]
then
    echo "demo.tmy already built"
else
    ./build.sh
fi

cd "${PARENT_PATH}/contracts/id.tmy"
if [ -e id.tmy.wasm ]
then
    echo "id.tmy already built"
else
    ./build.sh
fi