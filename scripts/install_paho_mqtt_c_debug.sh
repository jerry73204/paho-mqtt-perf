#!/usr/bin/env bash
set -e

mkdir -p deps
cd deps

wget --no-clobber "https://github.com/eclipse/paho.mqtt.c/archive/refs/tags/v1.3.11.tar.gz"

tar -xf v1.3.11.tar.gz
cd paho.mqtt.c-1.3.11

cmake -S . \
      -B build  \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_FLAGS=-pg \
      -DCMAKE_EXE_LINKER_FLAGS=-pg \
      -DCMAKE_SHARED_LINKER_FLAGS=-pg \
      -DPAHO_WITH_SSL=TRUE \
      -DPAHO_BUILD_SAMPLES=TRUE \
      -DPAHO_BUILD_STATIC=TRUE \
      -DCMAKE_INSTALL_PREFIX=$PWD/install
cmake --build build --parallel
cmake --install build
