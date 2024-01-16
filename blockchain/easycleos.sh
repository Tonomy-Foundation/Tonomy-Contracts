#!/bin/bash

if [ "$1" == "create-key" ]; then
    echo "Running create-key..."
    bash ./create-key.sh
elif [ "$1" == "transact" ]; then
    echo "Running transact with key: $2..."
    bash ./transact.sh 
else
    echo "Invalid command. Usage: $0 [create-key|transact]"
    exit 1
fi