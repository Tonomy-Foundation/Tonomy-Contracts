FROM eosio/eosio:v2.0.13 as tonomy_blockchain_base

RUN mkdir /var/config

# COPY . /var/contracts
# WORKDIR /var/contracts/blockchain

# RUN cp ./config.ini /var/config/config.ini

# CMD ./nodeos.sh

# ### Intialize the blockchain
# FROM tonomy_blockchain_base as tonomy_blockchain_initialized

# RUN bash -c "./nodeos.sh &" && sleep 10 && ./initialize-blockchain.sh && ./nodeos.sh stop