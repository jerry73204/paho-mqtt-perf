#include <MQTTAsync.h>
#include <stdlib.h>
#include "connect.h"
#include "global.h"
#include "utils.h"

void on_connection_lost(void *context, char *cause) {
  MQTTAsync client = (MQTTAsync)context;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

  fprintf(stderr, "\nConnection lost\n");
  fprintf(stderr, "     cause: %s\n", cause);

  exit(EXIT_FAILURE);
}

void on_disconnect_failure(void *context, MQTTAsync_failureData5 *response) {
  fprintf(stderr, "Disconnection failed\n");
  exit(EXIT_FAILURE);
}

void on_disconnect(void *context, MQTTAsync_successData5 *response) {
  if (VERBOSE) {
    fprintf(stderr, "Successful disconnection\n");
  }
  wait_on_barrier(&BARRIER);
}

void on_connect_failure(void *context, MQTTAsync_failureData5 *response) {
  fprintf(stderr, "Connection failed, rc %d\n", response ? response->code : 0);
  exit(EXIT_FAILURE);
}

void on_connect(void *context, MQTTAsync_successData5 *response) {
  if (VERBOSE) {
    fprintf(stderr, "Successful connection\n");
  }
  wait_on_barrier(&BARRIER);
}
