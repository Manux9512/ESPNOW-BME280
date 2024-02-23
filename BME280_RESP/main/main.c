#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/Task.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "led_strip.h"

static uint8_t peer_mac [ESP_NOW_ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

static const char * TAG = "esp_now_resp";

//Initialize wifi
static esp_err_t init_wifi(void)
{
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_netif_init();
    esp_event_loop_create_default();
    nvs_flash_init();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wifi init completed");
    return ESP_OK;
} 
//Configure recive data funtion
void recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
    ESP_LOGI(TAG, "Data received: " MACSTR " %s", MAC2STR(esp_now_info->src_addr), data);
    printf("Data value %s \n", data);
 
}
//Configure send data funtion (This funtion dont work with a receiver device)
void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if(status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "ESP_NOW_SEND_SUCCEDD");
    }
    else{
        ESP_LOGW(TAG, "ESP_NOW_SEND_FAIL");
    }
}
//Configure and initilize ESP-NOW
static esp_err_t init_esp_now(void)
{
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);
    esp_now_register_send_cb(send_cb);

    ESP_LOGI(TAG, "esp now init completed");
    return ESP_OK;
}
static esp_err_t esp_now_send_data(const uint8_t *peer_addr, const uint8_t *data, uint8_t len)
{
    esp_now_send(peer_addr, data, len);
    return ESP_OK;
}


void app_main(void)
{
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
}