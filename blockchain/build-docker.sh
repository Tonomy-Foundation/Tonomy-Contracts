#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}/.."

# Build image for antelope blockchain
docker rm -f antelope_blockchain || true
docker image build --target antelope_blockchain . -f ./blockchain/Dockerfile --force-rm -t antelope_blockchain

# Build Tonomy Contracts
./build-contracts.sh

# Build image for antelope blockchain with contracts and initalized chain
docker rm -f tonomy_blockchain_initialized || true
docker image build --target tonomy_blockchain_initialized . -f ./blockchain/Dockerfile --force-rm -t tonomy_blockchain_initialized

# Run command
# docker run --name tonomy_blockchain_initialized -p 8888:8888 -p 9101:9101 tonomy_blockchain_initialized
