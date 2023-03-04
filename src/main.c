#include <MQTTAsync.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include "publish.h"
#include "subscribe.h"
#include "logger.h"
#include "global.h"
#include "utils.h"
#include "connect.h"

typedef enum role { UNSPECIFIED, PUBLISHER, SUBSCRIBER } Role;

int main(int argc, char *argv[]) {
  int rc;

  /* Parse arguments */
  Role role = UNSPECIFIED;
  int opt;
  int qos = 0;
  size_t payload_size = 32;
  char address[256] = {0};

  while ((opt = getopt(argc, argv, "b:n:c:t:p:q:r:i:T:C:v")) != -1) {
    switch (opt) {
    case 'v':
      VERBOSE = true;
      break;
    case 'b':
      strncpy(address, optarg, sizeof(address));
      break;
    case 'n':
      strncpy(CLIENT_ID, optarg, sizeof(CLIENT_ID));
      break;
    case 't':
      strncpy(TOPIC, optarg, sizeof(TOPIC));
      break;
    case 'c': {
      long tmp = atoi(optarg);
      if (tmp < 0) {
        fprintf(stderr, "Invalid count %ld\n", tmp);
        exit(EXIT_FAILURE);
      }
      MAX_COUNT = (long)tmp;
      break;
    }
    case 'p': {
      long tmp = atoi(optarg);
      if (tmp < 4) {
        fprintf(stderr, "Invalid payload size %ld\n", tmp);
        exit(EXIT_FAILURE);
      }
      payload_size = (size_t)tmp;
      break;
    }
    case 'q': {
      long tmp = atoi(optarg);
      if (tmp < 0 || tmp > 2) {
        fprintf(stderr, "Invalid QoS %ld\n", tmp);
        exit(EXIT_FAILURE);
      }
      qos = (int)tmp;
      break;
    }
    case 'i': {
      long tmp = atoi(optarg);
      if (tmp <= 0) {
        fprintf(stderr, "Invalid interval %ld in milliseconds\n", tmp);
        exit(EXIT_FAILURE);
      }
      INTERVAL_MILLIS = (long)tmp;
      break;
    }
    case 'T': {
      long tmp = atoi(optarg);
      if (tmp <= 0) {
        fprintf(stderr, "Invalid maximum time %ld in milliseconds\n", tmp);
        exit(EXIT_FAILURE);
      }
      MAX_MILLIS = (long)tmp;
      break;
    }
    case 'r': {
      int cmp;

      if (role != UNSPECIFIED) {
        fprintf(stderr, "role must not be specified more than once\n");
        exit(EXIT_FAILURE);
      }

      cmp = strncmp(optarg, ROLE_PUBLISHER, sizeof(ROLE_PUBLISHER));
      if (cmp == 0) {
        role = PUBLISHER;
        break;
      }

      cmp = strncmp(optarg, ROLE_SUBSCRIBER, sizeof(ROLE_SUBSCRIBER));
      if (cmp == 0) {
        role = SUBSCRIBER;
        break;
      }

      fprintf(stderr, "unknown role \"%s\"\n", optarg);
      exit(EXIT_FAILURE);
    } break;
    default: /* '?' */
      fprintf(stderr,
              "Usage: %s -t topic -r {publisher|subscriber} [-b "
              "broker_address] [-n client_id] [-p payload_size] [-q qos]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  /* Check the topic */
  if (strlen(TOPIC) == 0) {
    fprintf(stderr, "-t TOPIC is not specified\n");
    exit(EXIT_FAILURE);
  }

  /* Check the role */
  if (role == UNSPECIFIED) {
    fprintf(stderr, "-r {publisher|subscriber} is required\n");
    exit(EXIT_FAILURE);
  }

  /* Check the address */
  if (strlen(address) == 0) {
    if (VERBOSE) {
      fprintf(stderr, "Using default broker address \"%s\"\n", DEFAULT_ADDRESS);
    }
    strncpy(address, DEFAULT_ADDRESS, sizeof(DEFAULT_ADDRESS));
  }

  /* Check the client_id */
  if (strlen(CLIENT_ID) == 0) {
    if (role == PUBLISHER) {
      strncpy(CLIENT_ID, DEFAULT_PUBLISHER_ID, sizeof(DEFAULT_PUBLISHER_ID));
    } else if (role == SUBSCRIBER) {
      strncpy(CLIENT_ID, DEFAULT_SUBSCRIBER_ID, sizeof(DEFAULT_SUBSCRIBER_ID));
    } else {
      fprintf(stderr, "unreachable\n");
      exit(EXIT_FAILURE);
    }

    if (VERBOSE) {
      fprintf(stderr, "Using default client ID \"%s\"\n", CLIENT_ID);
    }
  }

  /* Create a client */
  MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;
  create_opts.MQTTVersion = MQTTVERSION_5;
  MQTTAsync client;
  rc = MQTTAsync_createWithOptions(&client, address, CLIENT_ID,
                                   MQTTCLIENT_PERSISTENCE_NONE, NULL, &create_opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to create client object, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  /* Set callbacks */
  int (*msg_arrive_callback)(void*, char*, int, MQTTAsync_message*) = NULL;
  if (role == PUBLISHER) {
      msg_arrive_callback = on_message_arrived_for_pub;
  } else if (role == SUBSCRIBER) {
      msg_arrive_callback = on_message_arrived_for_sub;
  }
  
  rc = MQTTAsync_setCallbacks(client, NULL, on_connection_lost,
                              msg_arrive_callback, NULL);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to set callback, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  /* Connect to the broker */
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = on_connect;
  conn_opts.onFailure = on_connect_failure;
  conn_opts.context = client;
  conn_opts.MQTTVersion = MQTTVERSION_5;

  reset_barrier(&BARRIER);
  rc = MQTTAsync_connect(client, &conn_opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to start connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  wait_on_barrier(&BARRIER);

  /* run publishing or subscription */
  if (role == PUBLISHER) {
    run_publisher(client, TOPIC, payload_size, qos);
  } else if (role == SUBSCRIBER) {
    run_subscriber(client, TOPIC, qos);
  } else {
    fprintf(stderr, "unreachable\n");
    exit(EXIT_FAILURE);
  }

  /* disconnect */
  MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
  opts.onSuccess = on_disconnect;
  opts.onFailure = on_disconnect_failure;
  opts.context = client;

  reset_barrier(&BARRIER);
  rc = MQTTAsync_disconnect(client, &opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to start disconnect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  wait_on_barrier(&BARRIER);

  MQTTAsync_destroy(&client);
  return rc;
}
