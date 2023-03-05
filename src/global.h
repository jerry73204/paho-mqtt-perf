#pragma once
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

static const char ROLE_PUBLISHER[] = "publisher";
static const char ROLE_SUBSCRIBER[] = "subscriber";
static const char DEFAULT_PUBLISHER_ID[] = "mqtt_bench_pub";
static const char DEFAULT_SUBSCRIBER_ID[] = "mqtt_bench_sub";
static const char DEFAULT_ADDRESS[] = "tcp://localhost:1883";

extern atomic_bool FINISHED;
extern bool VERBOSE;
extern pthread_barrier_t BARRIER;
extern char TOPIC[256];
extern char CLIENT_ID[256];
extern long INTERVAL_MILLIS;
extern long MAX_MILLIS;
extern atomic_uint_fast64_t ACCUMULATIVE_BYTES;
extern uint64_t ACCUMULATIVE_COUNT;
extern long MAX_COUNT;
extern struct timespec SINCE_START;
extern struct timespec SINCE_LAST_RECORD;
extern thrd_t logger_thread;
