#!/bin/bash

PORT=8888
NOSERVER=""


while [[ $# -gt 0 ]]; do
    case $1 in
        --port)
            PORT="$2"
            shift
            shift
            ;;
        --noserver)
            NOSERVER="--noserver"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

find . -maxdepth 1 -type f -name '*.so' -exec rm {} \;

g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -ffast-math -fopenmp -shared -o libfract.so fract.cpp

python runner.py --port "$PORT" $NOSERVER

