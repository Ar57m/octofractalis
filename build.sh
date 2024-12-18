#!/bin/bash

find . -maxdepth 1 -type f -name '*.so' -exec rm {} \;

g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp -shared -o libfract.so fract.cpp
