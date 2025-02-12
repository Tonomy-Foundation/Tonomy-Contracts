FROM ubuntu:20.04 as antelope_blockchain

RUN apt update
RUN apt install -y wget curl

RUN wget https://github.com/AntelopeIO/leap/releases/download/v3.2.3/leap_3.2.3-ubuntu20.04_amd64.deb
RUN apt install -y ./leap_3.2.3-ubuntu20.04_amd64.deb
# TODO: update to use stable 4.x release
# RUN wget https://github.com/AntelopeIO/leap/releases/download/v4.0.0-rc3/leap_4.0.0-rc3-ubuntu20.04_amd64.deb
# RUN apt install -y ./leap_4.0.0-rc3-ubuntu20.04_amd64.deb

RUN wget https://github.com/AntelopeIO/cdt/releases/download/v3.1.0/cdt_3.1.0_amd64.deb
RUN apt install ./cdt_3.1.0_amd64.deb

CMD nodeos

FROM antelope_blockchain as tonomy_blockchain_base

RUN mkdir /var/config
COPY . /var/contracts
WORKDIR /var/contracts/blockchain

RUN cp ./config.ini /var/config/config.ini

CMD ./nodeos.sh

### Intialize the blockchain
FROM tonomy_blockchain_base as tonomy_blockchain_initialized

## Run bootstrap script
RUN bash -c "./nodeos.sh &" && sleep 10 && ./initialize-blockchain.sh && ./nodeos.sh stop

### Install nodejs
FROM tonomy_blockchain_initialized as tonomy_blockchain_nodejs

RUN wget nodejs.org/dist/v22.3.0/node-v22.3.0-linux-x64.tar.gz
RUN tar -C /usr/local --strip-components 1 -xzf node-v22.3.0-linux-x64.tar.gz

### Easy cleos script
FROM tonomy_blockchain_initialized as tonomy_blockchain_easycleos

RUN mkdir /var/easycleos

# Copy the script to the container
RUN cp ./create-key.sh /var/easycleos/create-key.sh
RUN cp ./transact.sh /var/easycleos/transact.sh
