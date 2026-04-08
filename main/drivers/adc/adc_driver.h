#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include <stdbool.h>

typedef struct {
    adc_oneshot_unit_handle_t unit;
    adc_unit_t unit_id; 
} adc_unit_handle_custom_t;

typedef struct {
    adc_channel_t channel;
    adc_cali_handle_t cali_handle;
    bool calibrated;
} adc_channel_handle_custom_t;

void adc_unit_init(adc_unit_handle_custom_t *adc, adc_unit_t unit_id);

void adc_channel_init(adc_unit_handle_custom_t *adc,
                      adc_channel_handle_custom_t *ch,
                      adc_channel_t channel,
                      adc_atten_t atten);

int adc_read_raw(adc_unit_handle_custom_t *adc,
                 adc_channel_handle_custom_t *ch);

int adc_read_voltage(adc_unit_handle_custom_t *adc,
                     adc_channel_handle_custom_t *ch);

#endif