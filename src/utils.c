#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void reset_barrier(pthread_barrier_t *barrier) {
  pthread_barrier_init(barrier, NULL, 2);
}

void wait_on_barrier(pthread_barrier_t *barrier) {
  int rc = pthread_barrier_wait(barrier);
  if (rc != PTHREAD_BARRIER_SERIAL_THREAD && rc != 0) {
    fprintf(stderr, "`pthread_barrier_wait()` failed. return code = %d\n", rc);
    exit(EXIT_FAILURE);
  }
}

int timespec_cmp(struct timespec *lhs, struct timespec *rhs) {
  if (lhs->tv_sec > rhs->tv_sec) {
    return 1;
  } else if (lhs->tv_sec < rhs->tv_sec) {
    return -1;
  } else if (lhs->tv_nsec > rhs->tv_nsec) {
    return 1;
  } else if (lhs->tv_nsec < rhs->tv_nsec) {
    return -1;
  } else {
    return 0;
  }
}

void timespec_add_millis(struct timespec *tp, long millis) {
  long nanos = millis * 1000 * 1000;
  tp->tv_nsec += nanos;

  long rem = tp->tv_nsec % 1000000000;
  long quot = tp->tv_nsec / 1000000000;

  tp->tv_sec += quot;
  tp->tv_nsec = rem;
}

long timespec_sub_micros(struct timespec *lhs, struct timespec *rhs) {
  time_t diff_sec = lhs->tv_sec - rhs->tv_sec;
  long diff_nsec = lhs->tv_nsec - rhs->tv_nsec;
  return diff_sec * 1000000 + diff_nsec / 1000;
}
