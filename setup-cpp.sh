#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

echo "Installing c++"
sudo apt install c++ -y

echo "Cloning eosio.cdt"
git clone git@github.com:EOSIO/eosio.cdt.git
cd eosio.cdt
git checkout v1.8.1
