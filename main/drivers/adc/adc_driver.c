#include "adc_driver.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include <stdbool.h>

static bool example_adc_calibration_init(adc_unit_t unit,
                                         adc_channel_t channel,
                                         adc_atten_t atten,
                                         adc_cali_handle_t *out_handle);

void adc_unit_init(adc_unit_handle_custom_t *adc, adc_unit_t unit_id)
{
    adc->unit_id = unit_id;

    adc_oneshot_unit_init_cfg_t cfg = {
        .unit_id = unit_id
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&cfg, &adc->unit));
}

void adc_channel_init(adc_unit_handle_custom_t *adc,
                      adc_channel_handle_custom_t *ch,
                      adc_channel_t channel,
                      adc_atten_t atten)
{
    adc_oneshot_chan_cfg_t cfg = {
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    ch->channel = channel;

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc->unit, channel, &cfg));

    ch->calibrated = example_adc_calibration_init(
    adc->unit_id,
    channel,
    atten,
    &ch->cali_handle
    );
}

int adc_read_raw(adc_unit_handle_custom_t *adc,
                 adc_channel_handle_custom_t *ch)
{
    int raw = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc->unit, ch->channel, &raw));

    return raw;
}

int adc_read_voltage(adc_unit_handle_custom_t *adc,
                     adc_channel_handle_custom_t *ch)
{
    int raw = 0;
    int voltage = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc->unit, ch->channel, &raw));

    if (ch->calibrated) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(ch->cali_handle, raw, &voltage));
        return voltage;
    }

    return raw; // fallback
}

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI("Calibration", "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI("Calibration", "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI("Calibration", "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW("Calibration", "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE("Calibration", "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI("Calibration", "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI("Calibration", "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}