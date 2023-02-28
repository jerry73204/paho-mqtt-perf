#include <MQTTAsync.h>
#include <bits/time.h>
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

static const char ROLE_PUBLISHER[] = "publisher";
static const char ROLE_SUBSCRIBER[] = "subscriber";
static const char DEFAULT_PUBLISHER_ID[] = "mqtt_bench_pub";
static const char DEFAULT_SUBSCRIBER_ID[] = "mqtt_bench_sub";
static const char DEFAULT_ADDRESS[] = "tcp://localhost:1883";

typedef enum role { UNSPECIFIED, PUBLISHER, SUBSCRIBER } Role;

/* Global variables */
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

void reset_barrier() { pthread_barrier_init(&BARRIER, NULL, 2); }

void wait_on_barrier() {
  int rc = pthread_barrier_wait(&BARRIER);
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

void on_connection_lost(void *context, char *cause) {
  MQTTAsync client = (MQTTAsync)context;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

  fprintf(stderr, "\nConnection lost\n");
  fprintf(stderr, "     cause: %s\n", cause);

  exit(EXIT_FAILURE);
}

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

  wait_on_barrier();
}

void on_disconnect_failure(void *context, MQTTAsync_failureData *response) {
  fprintf(stderr, "Disconnect failed\n");
  exit(EXIT_FAILURE);
}

void on_disconnect(void *context, MQTTAsync_successData *response) {
  if (VERBOSE) {
    fprintf(stderr, "Successful disconnection\n");
  }
  wait_on_barrier();
}

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
  wait_on_barrier();
}

void on_connect_failure(void *context, MQTTAsync_failureData *response) {
  fprintf(stderr, "Connect failed, rc %d\n", response ? response->code : 0);
  exit(EXIT_FAILURE);
}

void on_connect(void *context, MQTTAsync_successData *response) {
  if (VERBOSE) {
    fprintf(stderr, "Successful connection\n");
  }
  wait_on_barrier();
}

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
}

void on_subscribe_failure(void *context, MQTTAsync_failureData *response) {
  fprintf(stderr, "Subscribe failed, rc %d\n", response->code);
  exit(EXIT_FAILURE);
}

int on_message_arrived(void *context, char *topic_name, int topic_len,
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

  /* Produce messages */
  for (uint32_t cnt = 0; MAX_COUNT == 0 || cnt < (uint32_t)MAX_COUNT;
       cnt += 1) {
    memcpy(payload, &cnt, sizeof(cnt));

    reset_barrier();
    rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
      fprintf(stderr, "Failed to start sendMessage, return code %d\n", rc);
      exit(EXIT_FAILURE);
    }
    wait_on_barrier();
  }
}

void run_subscriber(MQTTAsync client, char *topic, int qos) {
  int rc;
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

  opts.onSuccess = on_subscribe;
  opts.onFailure = on_subscribe_failure;
  opts.context = client;

  reset_barrier();
  rc = MQTTAsync_subscribe(client, topic, qos, &opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to start subscribe, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  /* wait until the all tasks finish */
  wait_on_barrier();
  rc = thrd_join(logger_thread, NULL);
  if (rc != thrd_success) {
    fprintf(stderr, "thrd_join() failed\n");
    exit(EXIT_FAILURE);
  }
}

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
  MQTTAsync client;
  rc = MQTTAsync_create(&client, address, CLIENT_ID,
                        MQTTCLIENT_PERSISTENCE_NONE, NULL);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to create client object, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  /* Set callbacks */
  rc = MQTTAsync_setCallbacks(client, NULL, on_connection_lost,
                              on_message_arrived, NULL);
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

  reset_barrier();
  rc = MQTTAsync_connect(client, &conn_opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to start connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  wait_on_barrier();

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

  reset_barrier();
  rc = MQTTAsync_disconnect(client, &opts);
  if (rc != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "Failed to start disconnect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  wait_on_barrier();

  MQTTAsync_destroy(&client);
  return rc;
}
