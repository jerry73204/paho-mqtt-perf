#!/usr/bin/env bash
set -e

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source "$script_dir/env.sh"

cmake \
    -B build \
    -S . \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_FLAGS=-pg \
    -DCMAKE_EXE_LINKER_FLAGS=-pg \
    -DCMAKE_SHARED_LINKER_FLAGS=-pg
cmake --build build
