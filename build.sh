#!/bin/bash

rm ./build/app
mkdir build
cd build
cmake ../ -DDEMO_FILES=$FILES
make
./app ../data.csv
