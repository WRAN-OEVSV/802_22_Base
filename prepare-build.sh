#!/bin/sh

echo "make build dir and run cmake"
mkdir build
cd build
cmake ..
echo "cd build; make"