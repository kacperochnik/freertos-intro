#ifndef MYMQTTCLIENT_H
#define MYMQTTCLIENT_H

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"

#define ADDRESS "tcp://broker.hivemq.com:1883"
#define CLIENTID "GLintern"
#define TOPIC "gl/internship"
#define QOS 1
#define TIMEOUT 10000L

//int mqtt_init_client();
int mqtt_publish_msg(char *payload);
//int mqtt_disconnect_client();

#endif