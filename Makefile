.PHONY: default build clean

TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

default: build

build:
ifdef TOOLCHAIN_FILE
	cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE)
else
	cmake -B build -S .
endif

	cmake --build build

clean:
	rm -rf build
