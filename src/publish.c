#include "publish.h"
#include "global.h"
#include "logger.h"
#include "utils.h"
#include <MQTTAsync.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

void on_send_failure(void *context, MQTTAsync_failureData5 *response) {
  fprintf(stderr, "Message send failed token %d error code %d\n",
          response->token, response->code);
  exit(EXIT_FAILURE);
}

void on_send(void *context, MQTTAsync_successData5 *response) {
  if (VERBOSE) {
    fprintf(stderr, "Message with token value %d delivery confirmed\n",
            response->token);
  }
}

void run_publisher(MQTTAsync client, char *topic, size_t payload_size,
                   int qos) {
  int rc;

  /* Generate a message */
  uint8_t *payload = (uint8_t *)calloc(1, payload_size);
  if (payload == NULL) {
    fprintf(stderr, "Unable to allocate payload. errno = %d\n", errno);
    exit(EXIT_FAILURE);
  }

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

  /* Configure response callbacks */
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  opts.onSuccess5 = on_send;
  opts.onFailure5 = on_send_failure;
  opts.context = client;

  /* Produce messages */
  uint32_t cnt = 0;

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
      if (rc == MQTTASYNC_MAX_BUFFERED_MESSAGES) {
        /* When the buffer is full, back off for 1ms. */
        usleep(1000);
      } else {
        fprintf(stderr, "Failed to start sendMessage, return code %d\n", rc);
        exit(EXIT_FAILURE);
      }
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
  MQTTAsync_freeMessage(&msg);
  return 1;
}
