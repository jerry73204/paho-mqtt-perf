#include "subscribe.h"
#include "global.h"
#include "logger.h"
#include "utils.h"
#include <MQTTAsync.h>
#include <stdlib.h>

void on_subscribe(void *context, MQTTAsync_successData *response) {
  if (VERBOSE) {
    fprintf(stderr, "Subscribe succeeded\n");
  }

  clock_gettime(CLOCK_MONOTONIC, &SINCE_START);
  SINCE_LAST_RECORD = SINCE_START;

  int rc = thrd_create(&logger_thread, run_logger, NULL);
  if (rc != thrd_success) {
    fprintf(stderr, "thrd_create() failed\n");
    exit(EXIT_FAILURE);
  }

  wait_on_barrier(&BARRIER);
}

void on_subscribe_failure(void *context, MQTTAsync_failureData *response) {
  fprintf(stderr, "Subscribe failed, rc %d\n", response->code);
  exit(EXIT_FAILURE);
}

void run_subscriber(MQTTAsync client, char *topic, int qos) {
  int rc;
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

  opts.onSuccess = on_subscribe;
  opts.onFailure = on_subscribe_failure;
  opts.context = client;

  reset_barrier(&BARRIER);
  rc = MQTTAsync_subscribe(client, topic, qos, &opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to start subscribe, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  wait_on_barrier(&BARRIER);

  /* wait until the all tasks finish */
  rc = thrd_join(logger_thread, NULL);
  if (rc != thrd_success) {
    fprintf(stderr, "thrd_join() failed with return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
}

int on_message_arrived_for_sub(void *context, char *topic_name, int topic_len,
                               MQTTAsync_message *msg) {
  if (FINISHED) {
    return 1;
  }

  MQTTAsync client = (MQTTAsync)context;

  if (VERBOSE) {
    fprintf(stderr, "A message received\n");
  }

  /* Ignore duplicates */
  if (msg->dup) {
    if (VERBOSE) {
      fprintf(stderr, "Ignore a duplicated message\n");
    }
    return 1;
  }

  /* Record payload length */
  atomic_fetch_add_explicit(&ACCUMULATIVE_BYTES, (uint_fast64_t)msg->payloadlen,
                            memory_order_acq_rel);

  MQTTAsync_freeMessage(&msg);

  /* Check message count limit */
  if (MAX_COUNT != 0) {
    ACCUMULATIVE_COUNT += 1;
    if (ACCUMULATIVE_COUNT > MAX_COUNT) {
      FINISHED = true;
      return 1;
    }
  }

  return 1;
}
