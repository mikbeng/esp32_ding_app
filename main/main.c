#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "audio_example_file.h"
#include "esp_adc_cal.h"

//i2s number
#define I2S_NUM           (0)
//i2s sample rate
#define I2S_SAMPLE_RATE   (16000)
//i2s data bits
#define I2S_SAMPLE_BITS   (16)

//I2S read buffer length
#define I2S_WRITE_LEN      (16 * 1024)

//enable display buffer for debug
#define I2S_BUF_DEBUG     (1)

static const char * TAG = "i2s_test";

void i2s_init()
{
	 int i2s_num = I2S_NUM;
	 i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate =  I2S_SAMPLE_RATE,
        .bits_per_sample = 16,
	    .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
	    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   //Mono
	    .intr_alloc_flags = 0,
	    .dma_buf_count = 2,
	    .dma_buf_len = 1024
	 };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = -1                                                       //Not used
    };

	 //install and start i2s driver
	 i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
     i2s_set_pin(i2s_num, &pin_config);
     i2s_set_clk(i2s_num, 16000, I2S_SAMPLE_BITS, I2S_CHANNEL_STEREO);
}

/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contains DAC data.
 */
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
#if (I2S_SAMPLE_BITS == 16)

    for (int i = 0; i < len; i++) {
        d_buff[j++] = s_buff[i];
    }
    return (len);
    /*
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 2);
    */
#else
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 4);
#endif
}

/**
 * @brief debug buffer data
 */
void example_disp_buf(uint8_t* buf, int length)
{
#if I2S_BUF_DEBUG
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
#endif
}

void app_main()
{
    int i2s_write_len = I2S_WRITE_LEN;
    size_t bytes_written;
    uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_write_len, sizeof(char));

    i2s_init();

    while(1){

        //4. Play an example audio file(file format: 8bit/16khz/single channel)
        ESP_LOGI(TAG, "Playing audio file");
        int offset = 0;
        int tot_size = sizeof(audio_table);

        ESP_LOGI(TAG, "Size of audio table: %d", tot_size);

        while (offset < tot_size) {
            int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
            ESP_LOGI(TAG, "play_len: %d", play_len);

            int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);
            i2s_write(I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
            offset += play_len;
            //example_disp_buf((uint8_t*) i2s_write_buff, 32);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
}
