#!/usr/bin/env bash

cd 3rdParty/redox

./make.sh

cd ../..

mkdir -p build &&
cd build &&
cmake ..  &&
time make &&
cd ..
