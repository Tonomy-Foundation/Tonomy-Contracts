# Tonomy-Contracts

Smart contracts to run the governance, identity, DAO, token and other ecosystem tools.

## Dependencies

- [Docker](http://docs.docker.com) v20.10+

For development, you don't need these, but they allow you to have correctly configured intellisense in VS Code and other IDEs

- [C++17](https://en.wikipedia.org/wiki/C%2B%2B) suggested to use [g++](#)

Run `./setup-cpp.sh` to install these.

### Install the Antelope cdt locally so that the VS Code Intellisense will be able to follow Antelope library classes and assets

Run `./setup-cpp.sh` to install these.

## Build

`./build-contracts.sh`

### Build contracts for testing

To build the contracts for testing (with the Tonomy-ID-SDK integration tests):

```bash
export BUILD_TEST=true
./delete-built-contracts.sh
./build-contracts.sh
```

## Build blockchain node container

`./blockchain/build-docker.sh`
