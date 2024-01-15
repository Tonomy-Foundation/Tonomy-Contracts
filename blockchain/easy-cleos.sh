#!/bin/bash

if [ "$1" == "create-key" ]; then
    echo "Running create-key..."
elif [ "$1" == "transact" ]; then
    echo "Running transact with key: $2..."
    # Add your transact logic here
else
    echo "Invalid command. Usage: $0 [create-key|transact]"
    exit 1
fi