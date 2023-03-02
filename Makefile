.PHONY: default build clean install_paho_mqtt_c run_publisher run_subscriber

# TOOLCHAIN_FILE=/home/newslab/repos/vcpkg/scripts/buildsystems/vcpkg.cmake

default: build

build: ./build/paho-mqtt-perf

./build/paho-mqtt-perf:
	./scripts/build.sh

run_publisher:
	./build/paho-mqtt-perf -r publisher -b tcp://192.168.1.1:1883 -t BENCH -p 32

run_subscriber:
	./build/paho-mqtt-perf -r subscriber -b tcp://192.168.1.1:1883 -t BENCH

clean:
	rm -rf build

install_paho_mqtt_c:
	./scripts/install_paho_mqtt_c.sh
