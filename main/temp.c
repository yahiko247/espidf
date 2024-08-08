#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "ssd1306.h"
#include "dht.h"
#include "driver/gpio.h"

#define DHT_PIN GPIO_NUM_26
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define OLED_ADDR 0x3C

void init_i2c()
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);
}

void display_text(ssd1306_t *dev, int row, const char *text)
{
    ssd1306_draw_string(dev, 0, row * 10, text, 16, true);
    ssd1306_show(dev);
}

void dht_task(void *pvParameter)
{
    ssd1306_t dev = {
        .addr = OLED_ADDR,
        .screen = SSD1306_SCREEN,
        .reset = SSD1306_RESET,
        .width = 128,
        .height = 64,
    };

    ssd1306_init(&dev, I2C_MASTER_NUM);

    while (1)
    {
        int16_t temperature = 0;
        int16_t humidity = 0;

        if (dht_read_data(DHT_TYPE_DHT11, DHT_PIN, &humidity, &temperature) == ESP_OK)
        {
            float temp_celsius = temperature / 10.0;
            float hum_percent = humidity / 10.0;

            char temp_text[20];
            char humidity_text[20];
            char condition_text[20];

            sprintf(temp_text, "Temp: %.1f°C", temp_celsius);
            sprintf(humidity_text, "Humidity: %.1f%%", hum_percent);

            if (temp_celsius < 25)
            {
                strcpy(condition_text, "Condition: Cold");
            }
            else if (temp_celsius >= 25 && temp_celsius <= 30)
            {
                strcpy(condition_text, "Condition: Normal");
            }
            else
            {
                strcpy(condition_text, "Condition: Hot");
            }

            unsigned long startMillis = xTaskGetTickCount() * portTICK_PERIOD_MS;
            while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - startMillis < 10000)
            {
                for (int x = 0; x < 128; x++)
                {
                    ssd1306_clear_screen(&dev);
                    display_text(&dev, 1, temp_text);
                    display_text(&dev, 3, humidity_text);
                    ssd1306_draw_rect(&dev, x, 50, 10, 10);
                    vTaskDelay(20 / portTICK_PERIOD_MS);

                    if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - startMillis >= 10000)
                    {
                        break;
                    }
                }
            }

            ssd1306_clear_screen(&dev);
            display_text(&dev, 3, condition_text);
            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
        else
        {
            printf("Failed to read data from sensor\n");
        }
    }
}

void app_main()
{
    init_i2c();
    dht_init(DHT_PIN);
    xTaskCreate(&dht_task, "dht_task", 2048, NULL, 5, NULL);
}
