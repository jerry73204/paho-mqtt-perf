.PHONY: default build clean

TOOLCHAIN_FILE=/home/newslab/repos/vcpkg/scripts/buildsystems/vcpkg.cmake

default: build

build:
ifdef TOOLCHAIN_FILE
	cmake -B build -S . \
		-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) \
		-DCMAKE_BUILD_TYPE=Release
else
	cmake -B build -S . \
		-DCMAKE_BUILD_TYPE=Release
endif

	cmake --build build

clean:
	rm -rf build
