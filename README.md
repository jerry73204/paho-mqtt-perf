# paho-mqtt-perf

MQTT throughput and latency meter based on [Eclipse Paho C client
library](https://github.com/eclipse/paho.mqtt.c).

## Build This Project

`cmake` is required for compilation.

```sh
git clone https://github.com/jerry73204/paho-mqtt-perf.git
cd paho-mqtt-perf

cmake -B build -S .
cmake --build build
```

## Usage

## Start the Broker

A MQTT broker, such as Eclipse Mosquitto, must be established in
anticipation. Ubuntu users can start the mosquitto daemon this way.

```sh
sudo apt install mosquitto
sudo systemctl start mosquitto.service
```


Mosquitto can also built manually from source code. To start the
mosquitto in manualy way,

```sh
mosquitto -c mosquitto.conf
```

## Run the Benchmark

The benchmark goes by running a publisher and a subscriber on two
sites. The subscriber always run first. In this exmaple, it performs a
benchmark in which the publisher keeps sending 4096-byte messages.

```sh
# Run the subscriber on the 1st site
./build/paho-mqtt-perf -r subscriber -t MYTOPIC

# Run the publisher on the 1st site
./build/paho-mqtt-perf -r publisher -t MYTOPIC -p 4096
```
