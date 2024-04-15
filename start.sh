#!/bin/bash
python setup.py build_ext --inplace
echo "Running fractal_generator..."
python runner.py
