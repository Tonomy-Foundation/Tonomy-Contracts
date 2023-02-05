FROM eosio/eosio:v2.0.13 as base

RUN mkdir /var/config

COPY . /var/repo
WORKDIR /var/repo/blockchain

RUN cp ./config.ini /var/config/config.ini

CMD ./nodeos.sh

### Intialize the blockchain
FROM base as initialized

RUN bash -c "./nodeos.sh &" && sleep 10 && ./initialize-blockchain.sh && ./nodeos.sh stop

### Install nodejs 16.4.1
FROM initialized as nodejs

RUN wget nodejs.org/dist/v16.4.1/node-v16.4.1-linux-x64.tar.gz
RUN tar -C /usr/local --strip-components 1 -xzf node-v16.4.1-linux-x64.tar.gz
