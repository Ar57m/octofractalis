#!/bin/bash

rm *so
g++ -O3 -Wall -Wextra -pedantic  -march=native -fPIC -funroll-loops -ffast-math -fopenmp -shared -o libfract.so fract.cpp
python runner.py
