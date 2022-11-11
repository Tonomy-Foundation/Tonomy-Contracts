# Tonomy-Contracts

Smart contracts to run the governance, identity, DAO, token and other ecosystem tools.

## Dependancies

- [Docker](http://docs.docker.com) v20.10+
- [Docker Compose](http://docs.docker.com/compose/) v1.29+

For development, you don't need these, but they allow you to have correctly configured intellisense in VS Code and other IDEs

- [C++17](https://en.wikipedia.org/wiki/C%2B%2B) suggested to use [g++](#)

Run `./setup-cpp.sh` to install these.

### Install the eosio.cdt locally so that the VS Code Intellisense will be able to follow EOSIO library classes and assets

```bash
git clone git@github.com:EOSIO/eosio.cdt.git
cd eosio.cdt
git checkout v1.8.1
```

## Build

`./build-contracts.sh`
