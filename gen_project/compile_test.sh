#!/bin/bash
set -e

# 1. Build the main 'aq' executable so we can run the generator
echo "==> Building aq generator executable"
cd ..
python3 build.py
cd gen_project

# 2. Run the generator script to create graph.c
echo "==> Generating graph.c from FE script"
../aq --headless . generator.fe > graph.c
echo "Generated graph.c:"
cat graph.c
echo ""

# 3. Build the combined executable using the Makefile
echo "==> Building combined C executable using the generated graph.c and aq's DSP core"
make

# 4. Run the combined executable
echo "==> Running compiled test_app (you should hear a synth sound for 5s)"
./test_app
