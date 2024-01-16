#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}/.."

docker rm -f antelope_blockchain || true
docker image build --target antelope_blockchain . -f ./blockchain/Dockerfile --force-rm -t antelope_blockchain

# Build Tonomy Contracts
./build-contracts.sh

# Create docker image
docker rm -f tonomy_blockchain_initialized || true 
docker build -t easycleos --target tonomy_easycleos . -f ./blockchain/Dockerfile

docker image build --target tonomy_blockchain_initialized . -f ./blockchain/Dockerfile --force-rm -t tonomy_blockchain_initialized
# docker run -it easycleos ./easycleos.sh create-key
# docker run -it easycleos ./easycleos.sh transact KEY_ehatehatha
