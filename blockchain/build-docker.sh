#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}/.."

ARCH=$(uname -m)  # Get system architecture

if [[ "$ARCH" == "x86_64" || "$ARCH" == "i386" ]]; then
    DOCKERFILE_NAME="./blockchain/Dockerfile"
elif [ "$ARCH" = "aarch64" ]; then
    DOCKERFILE_NAME="./blockchain/Dockerfile_arm"
else
    echo "Unsupported architecture: $ARCH"
    exit 1
fi

docker rm -f antelope_blockchain || true
docker image build --target antelope_blockchain . -f "${DOCKERFILE_NAME}" --force-rm -t antelope_blockchain

# Build Tonomy Contracts
./build-contracts.sh

# Create docker image
docker rm -f tonomy_blockchain_initialized || true 

docker image build --target tonomy_blockchain_initialized . -f "${DOCKERFILE_NAME}" --force-rm -t tonomy_blockchain_initialized
docker image build --target tonomy_blockchain_easycleos . -f "${DOCKERFILE_NAME}" --force-rm -t tonomy_blockchain_easycleos
