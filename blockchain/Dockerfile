FROM eosio/eosio:v2.0.13 as tonomy_blockchain_base

RUN mkdir /var/config

COPY . /var/contracts
WORKDIR /var/contracts/blockchain

RUN cp ./config.ini /var/config/config.ini

RUN wget https://github.com/EOSIO/eosio.cdt/releases/download/v1.8.1/eosio.cdt_1.8.1-1-ubuntu-18.04_amd64.deb
RUN apt install ./eosio.cdt_1.8.1-1-ubuntu-18.04_amd64.deb

RUN apt update
RUN apt install git -y

CMD ./nodeos.sh

### Intialize the blockchain
FROM tonomy_blockchain_base as tonomy_blockchain_initialized

RUN bash -c "./nodeos.sh &" && sleep 10 && ./initialize-blockchain.sh && ./nodeos.sh stop

### Install nodejs
FROM tonomy_blockchain_initialized as tonomy_blockchain_nodejs

RUN wget nodejs.org/dist/v16.4.1/node-v16.4.1-linux-x64.tar.gz
RUN tar -C /usr/local --strip-components 1 -xzf node-v16.4.1-linux-x64.tar.gz
