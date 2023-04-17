#!/bin/bash

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

echo "Installing c++ and libraries"
sudo apt install g++ -y

echo "Setup Boost library"
wget -O boost_1_67_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.gz/download
mkdir boost
tar xzvf boost_1_67_0.tar.gz -C boost
rm boost_1_67_0.tar.gz

echo "Cloning Antelope cdt for libraries"
git clone git@github.com:AntelopeIO/cdt.git
cd cdt
git checkout v3.1.0
