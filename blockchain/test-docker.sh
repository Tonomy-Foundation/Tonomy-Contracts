#!/bin/bash

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

docker exec -it eosiodocker nodeos-stop.sh
docker stop eosiodocker
docker rm eosiodocker

# Make sure you give the context as the root of the repo
docker build ./.. -f ./Dockerfile --force-rm -t eosiobuild

# sudo rm ${PARENT_PATH}/tmp -rf
# docker run -v ${PARENT_PATH}/tmp:/data --name eosiodocker -d eosiobuild
docker run --name eosiodocker eosiobuild
# docker exec -it eosiodocker cleos --help