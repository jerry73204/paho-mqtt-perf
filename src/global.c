#include "global.h"

atomic_bool FINISHED = false;
bool VERBOSE = false;
pthread_barrier_t BARRIER;
char TOPIC[256] = {0};
char CLIENT_ID[256] = {0};
long INTERVAL_MILLIS = 1000;
long MAX_MILLIS = 0;
atomic_uint_fast64_t ACCUMULATIVE_BYTES = 0;
uint64_t ACCUMULATIVE_COUNT = 0;
long MAX_COUNT = 0;
struct timespec SINCE_START;
struct timespec SINCE_LAST_RECORD;
thrd_t logger_thread;
