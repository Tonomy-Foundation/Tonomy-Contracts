#!/bin/bash

ARG1=$1

function start {
    # command line only args from the chain plugin:
    # disable-replay-opts
    exec nodeos \
        -e \
        -p eosio \
        --data-dir /data/data-dir \
        --config-dir /var/config \
        --disable-replay-opts
}

# Ensures gracefull shutdown of nodeos process. See:
# https://github.com/EOSIO/eos/issues/4742
# https://github.com/EOSIO/eos/issues/4462#issuecomment-412772944
function stop {
    nodeos_pid=$(pgrep -x nodeos)
    echo "Nodeos pid: ${nodeos_pid}"
    
    if [ -n "$(ps -p ${nodeos_pid} -o pid=)" ]; then
        echo "Send SIGINT"
        kill -SIGINT ${nodeos_pid}
    fi

    while [ -n "$(ps -p ${nodeos_pid} -o pid=)" ]
    do
        sleep 1
    done

    echo "Process nodeos has finished gracefully"}
}

if [ "${ARG1}" == "stop" ]
then
    stop
else
    start
fi
