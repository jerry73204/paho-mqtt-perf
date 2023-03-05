#pragma once
#include <MQTTAsync.h>

void on_connection_lost(void *context, char *cause);
void on_disconnect_failure(void *context, MQTTAsync_failureData5 *response);
void on_disconnect(void *context, MQTTAsync_successData5 *response);
void on_connect_failure(void *context, MQTTAsync_failureData5 *response);
void on_connect(void *context, MQTTAsync_successData5 *response);
