#pragma once
#include <MQTTAsync.h>

void on_subscribe(void *context, MQTTAsync_successData5 *response);
void on_subscribe_failure(void *context, MQTTAsync_failureData5 *response);
void run_subscriber(MQTTAsync client, char *topic, int qos);
int on_message_arrived_for_sub(void *context, char *topic_name, int topic_len,
                               MQTTAsync_message *msg);
