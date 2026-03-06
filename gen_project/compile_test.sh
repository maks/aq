#!/bin/bash
set -e

# 1. Build the main 'aq' executable
cd ..
python3 build.py
cd gen_project

# 2. Run the generator script (now named main.fe for auto-loading)
echo "==> Generating graph.c (Silence)"
../aq --headless ../demo ../gen_project/generator.fe > graph.c

# 3. Build the native test app
echo "==> Compiling native test app"
make clean
make

# 4. Run the test app for 2 loops only
echo "==> Running native test app (2 loops only)"
./test_app
