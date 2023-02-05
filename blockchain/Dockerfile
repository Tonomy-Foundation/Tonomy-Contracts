FROM eosio/eosio:v2.0.13 as base

RUN mkdir /var/config

COPY . /var/repo
WORKDIR /var/repo/blockchain

RUN cp ./config.ini /var/config/config.ini

CMD ./nodeos.sh

### Intialize the blockchain
FROM base as initialized

RUN touch ./nodeos16.log

RUN nohup bash -c "./nodeos.sh &" && sleep 12 && ./initialize-blockchain.sh && ./nodeos-stop.sh

# FROM initialized as nodejs

# RUN wget nodejs.org/dist/v16.4.1/node-v16.4.1-linux-x64.tar.gz
# RUN sudo tar -C /usr/local --strip-components 1 -xzf node-v16.4.1-linux-x64.tar.gz
