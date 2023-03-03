#include <MQTTAsync.h>
#include <errno.h>
#include <threads.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "logger.h"
#include "utils.h"
#include "publish.h"

void on_send_failure(void *context, MQTTAsync_failureData *response) {
  fprintf(stderr, "Message send failed token %d error code %d\n",
          response->token, response->code);
  exit(EXIT_FAILURE);
}

void on_send(void *context, MQTTAsync_successData *response) {
  if (VERBOSE) {
    fprintf(stderr, "Message with token value %d delivery confirmed\n",
            response->token);
  }
}

void run_publisher(MQTTAsync client, char *topic, size_t payload_size,
                   int qos) {
  int rc;

  /* Generate payload buffer */
  uint8_t *payload = (uint8_t *)calloc(1, payload_size);
  if (payload == NULL) {
    fprintf(stderr, "Unable to allocate payload. errno = %d\n", errno);
    exit(EXIT_FAILURE);
  }

  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  opts.onSuccess = on_send;
  opts.onFailure = on_send_failure;
  opts.context = client;

  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  pubmsg.payload = payload;
  pubmsg.payloadlen = (int)payload_size;
  pubmsg.qos = qos;
  pubmsg.retained = 0;

  /* Record first timestamp */
  clock_gettime(CLOCK_MONOTONIC, &SINCE_START);
  SINCE_LAST_RECORD = SINCE_START;

  /* Spawn the logger thread */
  rc = thrd_create(&logger_thread, run_logger, NULL);
  if (rc != thrd_success) {
    fprintf(stderr, "thrd_create() failed\n");
    exit(EXIT_FAILURE);
  }

  /* Produce messages */
  long cnt = 0;

  while (!FINISHED) {
    cnt += 1;

    /* Check count limit */
    if (MAX_COUNT != 0 && cnt > MAX_COUNT) {
      FINISHED = true;
      break;
    }

    memcpy(payload, &cnt, sizeof(cnt));

    rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
      fprintf(stderr, "Failed to start sendMessage, return code %d\n", rc);
      exit(EXIT_FAILURE);
    }
    atomic_fetch_add_explicit(&ACCUMULATIVE_BYTES, payload_size,
                              memory_order_acq_rel);
  }

  rc = thrd_join(logger_thread, NULL);
  if (rc != thrd_success) {
    fprintf(stderr, "thrd_join() failed\n");
    exit(EXIT_FAILURE);
  }
}

int on_message_arrived_for_pub(void *context, char *topic_name, int topic_len,
                               MQTTAsync_message *msg) {
    return 1;
}
