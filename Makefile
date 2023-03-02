.PHONY: default build_debug build_release clean run_publisher run_subscriber

# TOOLCHAIN_FILE=/home/newslab/repos/vcpkg/scripts/buildsystems/vcpkg.cmake

default: build_release

build_debug:
	./scripts/install_paho_mqtt_c_debug.sh
	./scripts/build_debug.sh

build_release:
	./scripts/install_paho_mqtt_c_release.sh
	./scripts/build_release.sh

run_publisher:
	./build/paho-mqtt-perf -r publisher -b tcp://192.168.1.1:1883 -t BENCH -p 32

run_subscriber:
	./build/paho-mqtt-perf -r subscriber -b tcp://192.168.1.1:1883 -t BENCH

clean:
	rm -rf build deps
