#pragma once
#include <pthread.h>

void reset_barrier(pthread_barrier_t *barrier);
void wait_on_barrier(pthread_barrier_t *barrier);
int timespec_cmp(struct timespec *lhs, struct timespec *rhs);
void timespec_add_millis(struct timespec *tp, long millis);
long timespec_sub_micros(struct timespec *lhs, struct timespec *rhs);
