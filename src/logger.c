#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "global.h"
#include "utils.h"
#include "logger.h"

int run_logger(void *arg) {
  while (true) {
    if (FINISHED) {
      break;
    }

    /* Sleep for a while */
    usleep(INTERVAL_MILLIS * 1000);

    /* Save current time */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    /* Check time limit */
    if (MAX_MILLIS != 0) {
      long elapsed_micros = timespec_sub_micros(&now, &SINCE_START);
      if (elapsed_micros > MAX_MILLIS * 1000) {
        FINISHED = true;
        break;
      }
    }

    /* Print byte rate rate */
    long elapsed_micros = timespec_sub_micros(&now, &SINCE_LAST_RECORD);
    uint_fast64_t acc_bytes =
        atomic_exchange_explicit(&ACCUMULATIVE_BYTES, 0, memory_order_acq_rel);
    double rate = (double)acc_bytes / (double)elapsed_micros * 1000000.0;
    printf("%lf\n", rate);

    SINCE_LAST_RECORD = now;
  }
}
