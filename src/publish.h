#pragma once
#include <MQTTAsync.h>

void run_publisher(MQTTAsync client, char *topic, size_t payload_size, int qos);
int on_message_arrived_for_pub(void *context, char *topic_name, int topic_len,
                               MQTTAsync_message *msg);
