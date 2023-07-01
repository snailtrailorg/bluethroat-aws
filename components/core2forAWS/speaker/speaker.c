#include "freertos/FreeRTOS.h"
#include "esp_idf_version.h"
#include "speaker.h"

speaker_device_t * Speaker_Init(i2s_port_t i2s_port, int bck_io_num, int ws_io_num, int data_out_num, int data_in_num, int sample_rate, i2s_bits_per_sample_t bit_per_sample) {
    speaker_device_t * speaker = malloc(sizeof(speaker_device_t));
    if (speaker == NULL) {
        return NULL;
    } else {
        speaker->i2s_port = i2s_port;
        speaker->bck_io_num = bck_io_num;
        speaker->ws_io_num = ws_io_num;
        speaker->data_out_num = data_out_num;
        speaker->data_in_num = data_in_num;
        speaker->sample_rate = data_in_num;
        speaker->bit_per_sample = bit_per_sample;
    }

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = sample_rate,
        .bits_per_sample = bit_per_sample, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
#else
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
    };

    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.use_apll = false;
    i2s_config.tx_desc_auto_clear = true;
    esp_err_t err = i2s_driver_install(i2s_port, &i2s_config, 0, NULL);

    if(err != ESP_OK){
        free(speaker);
        return NULL;
    }

    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = bck_io_num;
    tx_pin_config.ws_io_num = ws_io_num;
    tx_pin_config.data_out_num = data_out_num;
    tx_pin_config.data_in_num = data_in_num;
    err = i2s_set_pin(i2s_port, &tx_pin_config);

    if(err != ESP_OK){
        i2s_driver_uninstall(i2s_port);
        free(speaker);
        return NULL;
    }

    err = i2s_set_clk(i2s_port, sample_rate, bit_per_sample, I2S_CHANNEL_MONO);

    if(err != ESP_OK){
        gpio_reset_pin(bck_io_num);
        gpio_reset_pin(ws_io_num);
        gpio_reset_pin(data_out_num);
        gpio_reset_pin(data_in_num);
        i2s_driver_uninstall(i2s_port);
        free(speaker);
        return NULL;
    }

    return speaker;
}

esp_err_t Speaker_WriteBuff(speaker_device_t * speaker, uint8_t* buff, uint32_t len, uint32_t timeout) {
    size_t bytes_written = 0;
    return i2s_write(speaker->i2s_port, buff, len, &bytes_written, portMAX_DELAY);
}

esp_err_t Speaker_Deinit(speaker_device_t * speaker) {
    esp_err_t err = ESP_OK;
    err += i2s_driver_uninstall(speaker->i2s_port);
    err += gpio_reset_pin(speaker->bck_io_num);
    err += gpio_reset_pin(speaker->ws_io_num);
    err += gpio_reset_pin(speaker->data_out_num);
    err += gpio_reset_pin(speaker->data_in_num);
    free(speaker);
    if(err != ESP_OK){
        err = ESP_FAIL;
    }
    return err;
}