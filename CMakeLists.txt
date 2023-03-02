cmake_minimum_required(VERSION 3.15)
project(paho-mqtt-perf VERSION 0.1.0 LANGUAGES C)

find_package(Threads REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)

add_executable(paho-mqtt-perf src/main.c src/publish.c src/logger.c src/utils.c src/global.c src/subscribe.c src/connect.c)

target_link_libraries(paho-mqtt-perf PRIVATE Threads::Threads)
target_link_libraries(paho-mqtt-perf PUBLIC eclipse-paho-mqtt-c::paho-mqtt3a)
