/* MQTT + WiFi helper for HiveMQ Cloud
 * - Call mqtt_app_start() from your `app_main()` to initialize NVS, connect
 *   to Wi‑Fi and start MQTT over TLS to HiveMQ Cloud.
 * - Edit `CONFIG_WIFI_SSID` / `CONFIG_WIFI_PASSWORD` in menuconfig or
 *   sdkconfig to set the Wi-Fi credentials.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

#define TAG "mqtt_module"

#ifndef CONFIG_WIFI_SSID
#define CONFIG_WIFI_SSID "ESP32"
#endif

#ifndef CONFIG_WIFI_PASSWORD
#define CONFIG_WIFI_PASSWORD "123123457457"
#endif

#define MQTT_URI      "mqtts://cebea07170a6456b831f1051f9f9ce47.s1.eu.hivemq.cloud:8883"
#define MQTT_USER     "virtual_display"
#define MQTT_PASSWORD "Virtual_display0@"

// RECEBE
#define TOPIC_CONFIG "virtualDisplay/config"
#define TOPIC_BUTTON_1 "virtualDisplay/button/Lamp. 1"
#define TOPIC_BUTTON_2 "virtualDisplay/button/Lamp. 2"
// ENVIA
#define TOPIC_RESPONSE_CONFIG "virtualDisplay/response_config"
#define TOPIC_DATA "virtualDisplay/data"


/* Certificate (PEM) embedded as binary in the app image (see CMakeLists)
   The project should link the certificate file as a binary blob named
   isrgrootx1.pem -> _binary_isrgrootx1_pem_start/_end. */
extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t isrgrootx1_pem_end[]   asm("_binary_isrgrootx1_pem_end");

static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static esp_mqtt_client_handle_t mqtt_client = NULL;
static TaskHandle_t publish_task_handle = NULL;

/* Valores dos widgets para publicação recorrente */
static float temperature = 60.45f;
static float speed = 91.0f;
static bool lamp1 = true;
static bool lamp2 = false;
static char alerts[100] = "Erro não identificado";

static void on_mqtt_connected(void);

void mqtt_publish_data(char *payload)
{
    if (!mqtt_client) {
        ESP_LOGW(TAG, "MQTT client not started yet");
    }
    int len = snprintf(payload, 512,
                          "{\"values\":"
                          "{\"Sensor temp.\":\"%.2f\","
                          "\"Lamp. 1\":%s,"
                          "\"Lamp. 2\":%s,"
                          "\"Veloc.\":\"%.0f\","
                          "\"Alertas\":\"%s\"}}",
                          temperature,
                          lamp1 ? "true" : "false",
                          lamp2 ? "true" : "false",
                          speed,
                          alerts);
        
        if (len > 0 && len < 512) {
            esp_mqtt_client_publish(mqtt_client, TOPIC_DATA, payload, strlen(payload), 1, 0);
        }
}

void mqtt_publish_config(const char *message)
{
    if (!mqtt_client) {
        ESP_LOGW(TAG, "MQTT client not started yet");
        return;
    }

    if (!message) {
        ESP_LOGW(TAG, "mqtt_publish_config called with null message");
        return;
    }

    esp_mqtt_client_publish(mqtt_client, TOPIC_RESPONSE_CONFIG, message, strlen(message), 1, 1);
}

static void Task_publishValues(void *pvParameters)
{
    char *payload = malloc(512);
    if (!payload) {
        ESP_LOGE(TAG, "Failed to allocate publish payload buffer");
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(4000)); 
        mqtt_publish_data(payload);
    }
}

static void start_publish_task(void)
{
    if (publish_task_handle == NULL) {
        xTaskCreate(Task_publishValues, "publish_task", 4096, NULL, 5, &publish_task_handle);
        ESP_LOGI(TAG, "CRIADO TASK DE ENVIO");
    }
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_DATA: {
        char buff_topic[50] = {0};
        char buff_data[100] = {0};
        size_t topic_len = event->topic_len < sizeof(buff_topic) - 1 ? event->topic_len : sizeof(buff_topic) - 1;
        size_t data_len = event->data_len < sizeof(buff_data) - 1 ? event->data_len : sizeof(buff_data) - 1;

        memcpy(buff_topic, event->topic, topic_len);
        memcpy(buff_data, event->data, data_len);

        if (strcmp(buff_topic, TOPIC_CONFIG) == 0)
        {
            ESP_LOGI(TAG, "PUB CONFIG:");
            // mqtt_publish_config(
            //     "{\"device\":\"ESP32\",\"widgets\":["
            //     "{\"id\":\"temperature\",\"type\":\"number\",\"title\":\"Sensor temp.\",\"decimal\":\"2\",\"unit\":\"celsius\",\"min\":\"-55.0\",\"max\":\"200\",\"history\":\"true\",\"value\":\"10\"},"
            //     "{\"id\":\"speed\",\"type\":\"number\",\"title\":\"Veloc.\",\"decimal\":\"1\",\"unit\":\"RPM\",\"min\":\"0\",\"max\":\"200\",\"history\":\"true\",\"value\":\"95\"},"
            //     "{\"id\":\"led\",\"type\":\"bool\",\"title\":\"Lamp. 1\",\"value\":\"true\"},"
            //     "{\"id\":\"gps\",\"type\":\"bool\",\"title\":\"Lamp. 2\",\"value\":\"false\"},"
            //     "{\"id\":\"message\",\"type\":\"string\",\"title\":\"Alertas\",\"value\":\"Erro na comunicação\"}"
            //     "]}"
            // );
        }
        else if(!(strcmp(buff_topic, TOPIC_BUTTON_1)))
        {
            ESP_LOGI("MQTT", "RECEBEU BOTÃO 1");
        }
        else if(!(strcmp(buff_topic, TOPIC_BUTTON_2)))
        {
            ESP_LOGI("MQTT", "RECEBEU BOTÃO 2");
        }
        break;
    }
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        on_mqtt_connected();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Message published (msg_id=%d)", event->msg_id);
        break;
    default:
        break;
    }
}

static void start_mqtt(void)
{
    if (mqtt_client) {
        ESP_LOGI(TAG, "MQTT already started");
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWORD,
        .broker.verification.certificate = (const char *)isrgrootx1_pem_start,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    ESP_LOGI(TAG, "MQTT client started");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        // ESP_LOGI(TAG, "Got IP: %s", (char *)esp_ip4addr_ntoa(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        /* Start MQTT once we have an IP */
        start_mqtt();
    }
}

static void on_mqtt_connected(void)
{
    ESP_LOGI(TAG, "SUBS: %s", TOPIC_CONFIG);
    esp_mqtt_client_subscribe(mqtt_client, TOPIC_CONFIG, 0);
    esp_mqtt_client_subscribe(mqtt_client, TOPIC_BUTTON_1, 0);
    esp_mqtt_client_subscribe(mqtt_client, TOPIC_BUTTON_2, 0);
    // Envia configuração
     mqtt_publish_config(
                "{\"device\":\"ESP32\",\"widgets\":["
                "{\"id\":\"temperature\",\"type\":\"number\",\"title\":\"Sensor temp.\",\"decimal\":\"2\",\"unit\":\"celsius\",\"min\":\"-55.0\",\"max\":\"200\",\"history\":\"true\",\"value\":\"10\"},"
                "{\"id\":\"speed\",\"type\":\"number\",\"title\":\"Veloc.\",\"decimal\":\"1\",\"unit\":\"RPM\",\"min\":\"0\",\"max\":\"200\",\"history\":\"true\",\"value\":\"95\"},"
                "{\"id\":\"led\",\"type\":\"bool\",\"title\":\"Lamp. 1\",\"value\":\"true\"},"
                "{\"id\":\"gps\",\"type\":\"bool\",\"title\":\"Lamp. 2\",\"value\":\"false\"},"
                "{\"id\":\"message\",\"type\":\"string\",\"title\":\"Alertas\",\"value\":\"Erro na comunicação\"}"
                "]}"
            );
    vTaskDelay(pdMS_TO_TICKS(200));
    // Inicializa envio de dados periodicamente
    start_publish_task();
}

esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) return ESP_FAIL;

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_LOGI(TAG, "Connecting to WiFi SSID:%s", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    /* Wait for connection */
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    return ESP_OK;
}

/* Public helper: initialize NVS, connect Wi‑Fi and start MQTT. Call from `app_main()` */
void mqtt_app_start(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(wifi_init_sta());
}
