#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}/.."

# Build Tonomy Contracts
./build-contracts.sh

# Create docker image
docker rm -f tonomy_blockchain_initialized || true
docker image build --target tonomy_blockchain_initialized . -f ./blockchain/Dockerfile --force-rm -t tonomy_blockchain_initialized
