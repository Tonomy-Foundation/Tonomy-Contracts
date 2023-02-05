#!/bin/bash

# command line only args from the chain plugin:
# disable-replay-opts
nodeos\
    -e\
    -p eosio\
    --data-dir /data/data-dir\
    --config-dir /var/config\
    --disable-replay-opts

# nohup sh -c nodeos -e -p eosio &
# nodeos -e -p eosio
# nohup echo "what do you want" > ./log.log &
# sleep 1
# cat ./log.log
# echo "Running nodeos.sh"

# echo "what do you want now?" > /var/config/log.log &