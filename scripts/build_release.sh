#!/usr/bin/env bash
set -e

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source "$script_dir/env.sh"

cmake \
    -B build \
    -S . \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build
