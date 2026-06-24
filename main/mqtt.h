/**
 * @file    mqtt.h
 * @author  Gabriel Comim
 * 
 * @brief   Biblioteca para utilização do protocolo MQTT.
 *          Faz a recepção e o envio das mensagens MQTT. 
 *             
 * @version 1.5
 * @date    23/10/23
 * @refresh 10/03/25 
*/

#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "mqtt_client.h"

void mqtt_app_start(void);
void mqtt_publish_config(const char *message);
void mqtt_publish_data(char *payload);

#endif