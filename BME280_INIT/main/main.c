#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/Task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "bme280.h"

#define TAG_BME280 "BME280"

#define SDA_PIN GPIO_NUM_6
#define SCL_PIN GPIO_NUM_7

#define I2C_MASTER_ACK 0
#define I2C_MASTER_NACK 1

#define ESP_CHANNEL 1

static uint8_t peer_mac [ESP_NOW_ETH_ALEN] = {0x48,0x27,0xe2,0xfd,0x66,0x5c}; //Response's MAC direction
static const char * TAG = "esp_now_init";
//-------------------------------ESP-NOW---------------------------------
// Initialize wifi 
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
// Initialize ESP-NOW parameters
void recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
    ESP_LOGI(TAG, "Data received" MACSTR "%s", MAC2STR(esp_now_info->src_addr), data);

}
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
// Initialize ESP-NOW
static esp_err_t init_esp_now(void)
{
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);
    esp_now_register_send_cb(send_cb);

    ESP_LOGI(TAG, "esp now init completed");
    return ESP_OK;
}
//Register a new device to send information
static esp_err_t register_peer(uint8_t *peer_addr)
{
    esp_now_peer_info_t esp_now_peer_info = {};
    memcpy(esp_now_peer_info.peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    esp_now_peer_info.channel = ESP_CHANNEL;
    esp_now_peer_info.ifidx = ESP_IF_WIFI_STA;

    esp_now_add_peer(&esp_now_peer_info);
    return ESP_OK;
}
//Configure function to send data
static esp_err_t esp_now_send_data(const uint8_t *peer_addr, const uint8_t *data, uint8_t len)
{
    esp_now_send(peer_addr, data, len);
    return ESP_OK;
}
//-----------------------------------------------------------------------

//--------------------------------BME280---------------------------------
// Initialize I2C communication parameters
void i2c_master_init()
{
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA_PIN,
		.scl_io_num = SCL_PIN,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 1000000};
	i2c_param_config(I2C_NUM_0, &i2c_config);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}
// BME280 I2C write function
s8 BME280_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = BME280_INIT_VALUE;

	esp_err_t espRc;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, reg_addr, true);
	i2c_master_write(cmd, reg_data, cnt, true);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
	if (espRc == ESP_OK)
	{
		iError = SUCCESS;
	}
	else
	{
		iError = ERROR;
	}
	i2c_cmd_link_delete(cmd);

	return (s8)iError;
}
// BME280 I2C read function
s8 BME280_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = BME280_INIT_VALUE;
	esp_err_t espRc;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg_addr, true);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);

	if (cnt > 1)
	{
		i2c_master_read(cmd, reg_data, cnt - 1, I2C_MASTER_ACK);
	}
	i2c_master_read_byte(cmd, reg_data + cnt - 1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
	if (espRc == ESP_OK)
	{
		iError = SUCCESS;
	}
	else
	{
		iError = ERROR;
	}

	i2c_cmd_link_delete(cmd);

	return (s8)iError;
}
// BME280 I2C delay function
void BME280_delay_msek(u32 msek)
{
	vTaskDelay(msek / portTICK_PERIOD_MS);
}
// Read data from BME280, convert data to unsigned data and send data to another device.
void Publisher_Task(void *params)
{
	// BME280 I2C communication structure
	struct bme280_t bme280 = {
		.bus_write = BME280_I2C_bus_write,
		.bus_read = BME280_I2C_bus_read,
		.dev_addr = BME280_I2C_ADDRESS1,
		.delay_msec = BME280_delay_msek};

	s32 com_rslt;
	s32 v_uncomp_pressure_s32;
	s32 v_uncomp_temperature_s32;
	s32 v_uncomp_humidity_s32;

	// Initialize BME280 sensor and set internal parameters
	com_rslt = bme280_init(&bme280);
	printf("com_rslt %d\n", com_rslt);

	com_rslt += bme280_set_oversamp_pressure(BME280_OVERSAMP_16X);
	com_rslt += bme280_set_oversamp_temperature(BME280_OVERSAMP_2X);
	com_rslt += bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);

	com_rslt += bme280_set_standby_durn(BME280_STANDBY_TIME_1_MS);
	com_rslt += bme280_set_filter(BME280_FILTER_COEFF_16);

	com_rslt += bme280_set_power_mode(BME280_NORMAL_MODE);
	if (com_rslt == SUCCESS)
	{
		while (true)
		{
			vTaskDelay(pdMS_TO_TICKS(15000));

			// Read BME280 data
			com_rslt = bme280_read_uncomp_pressure_temperature_humidity(
				&v_uncomp_pressure_s32, &v_uncomp_temperature_s32, &v_uncomp_humidity_s32);

			double temp = bme280_compensate_temperature_double(v_uncomp_temperature_s32);
			char temperature[10];
			sprintf(temperature, "%.2f °C", temp);

			double press = bme280_compensate_pressure_double(v_uncomp_pressure_s32) / 100; // Pa -> hPa
			char pressure[10];
			sprintf(pressure, "%.2f hPa", press);

			double hum = bme280_compensate_humidity_double(v_uncomp_humidity_s32);
			char humidity[10];
			sprintf(humidity, "%.2f %%", hum);

			//Convert char values to uint8
            uint8_t temperature_u[10];
            uint8_t pressure_u[10];
            uint8_t humidity_u[10];

            for (int i = 0; i<10;i++){
                pressure_u[i] = (uint8_t)pressure[i];
                temperature_u[i] = (uint8_t)temperature[i]; 
                humidity_u[i] = (uint8_t)humidity[i];

            }

			// Print BME data and send it to the response device
			if (com_rslt == SUCCESS)
			{
				printf("Temperature: %s\n",temperature_u);
                printf("Pressure: %s\n",pressure_u);
                printf("Humidity: %s\n",humidity_u);
                printf("-------------------------\n");

                esp_now_send_data(peer_mac, temperature_u ,32);
                esp_now_send_data(peer_mac, pressure_u ,32);
                esp_now_send_data(peer_mac, humidity_u ,32);
                
                
			}
			else
			{
				ESP_LOGE(TAG_BME280, "measure error. code: %d", com_rslt);
			}
		}
	}
	else
	{
		ESP_LOGE(TAG_BME280, "init or setting error. code: %d", com_rslt);
	}
}
//------------------------------------------------------------------------


void app_main(void)
{
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(register_peer(peer_mac));


//----------------------------BME280-----------------------------------------------
    esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize I2C parameters
	i2c_master_init();

	// Read the data from BME280 sensor
	xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
//--------------------------------------------------------------------------------
 
}