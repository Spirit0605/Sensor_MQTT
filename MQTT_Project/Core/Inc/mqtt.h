#ifndef __MQTT_H
#define __MQTT_H

#include "main.h"
#include "usart.h"
#include "task.h"
#include <stdbool.h>

#define WIFI_NAME "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define CLIENT_KEY "YOUR_CLIENT_KEY"
#define SERVER_ADDRESS "mqtt.bemfa.com"
#define SERVER_PORT 9501

#define CLIENT_LIGHT_TOPIC "light004"
#define CLIENT_SOIL_TOPIC "soil004"
#define CLIENT_CO2_TOPIC "co004"
#define CLIENT_TEMP_TOPIC "temp004"
#define CLIENT_HUMI_TOPIC "humi004"
#define CLIENT_RELAY_TOPIC "relay003"

// 为了获取UTF-8格式的字符串长度的MSB和LSB
#define MQTT_UTF8_LSB(len) (*((uint8_t *)(&len) + 0))
#define MQTT_UTF8_MSB(len) (*((uint8_t *)(&len) + 1))

bool MQTT_CONNECT(char *client_id);
bool MQTT_SUBSCRIBE(char *topic, uint8_t Qos);
void MQTT_Publish(char *topic, char *msg);

#endif