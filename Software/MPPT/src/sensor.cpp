#include "sensor.h"
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>
#include "config.h"

SensorData sensor_data;

Adafruit_INA219 ina_input(ADDR_INA_INPUT);
Adafruit_INA219 ina_output(ADDR_INA_OUTPUT);
Adafruit_ADS1115 acs_output;


float get_input_voltage() {
    return ina_input.getBusVoltage_V();
}

float get_output_voltage() {
    return ina_output.getBusVoltage_V();
}

float get_input_current() {
    return (ina_input.getCurrent_mA() / 1000.0) + OFFSET_IIN;
}

float get_output_current_acs() {
    int16_t raw = acs_output.readADC_Differential_0_1();
    float voltage = acs_output.computeVolts(raw);
    float current = voltage / 0.4;
    return current + OFFSET_IOUT_ACS;
}

float get_output_current_ina() {
    return (ina_output.getCurrent_mA() / 1000.0) + OFFSET_IOUT_INA;
}

float get_output_current() {
    if (USE_ACS_CURRENT_SENSOR) {
        return get_output_current_acs();
    } else {
        return get_output_current_ina();
    }
}

void update_sensor_data() {
    float vin_samples[SENSOR_SAMPLES];
    float iin_samples[SENSOR_SAMPLES];
    float vout_samples[SENSOR_SAMPLES];
    float iout_ina_samples[SENSOR_SAMPLES];
    float iout_acs_samples[SENSOR_SAMPLES];

    for (int i = 0; i < SENSOR_SAMPLES; i++) {
        vin_samples[i] = get_input_voltage();
        iin_samples[i] = get_input_current();
        vout_samples[i] = get_output_voltage();
        iout_ina_samples[i] = get_output_current_ina();
        iout_acs_samples[i] = get_output_current_acs();
    }

    sensor_data.vin = 0;
    sensor_data.iin = 0;
    sensor_data.vout = 0;
    sensor_data.iout = 0;
    sensor_data.iout_ina = 0;
    sensor_data.iout_acs = 0;
    for (int i = 0; i < SENSOR_SAMPLES; i++) {
        sensor_data.vin += vin_samples[i];
        sensor_data.iin += iin_samples[i];
        sensor_data.vout += vout_samples[i];
        sensor_data.iout_ina += iout_ina_samples[i];
        sensor_data.iout_acs += iout_acs_samples[i];
    }
    sensor_data.vin /= SENSOR_SAMPLES;
    sensor_data.iin /= SENSOR_SAMPLES;
    sensor_data.vout /= SENSOR_SAMPLES;
    sensor_data.iout_ina /= SENSOR_SAMPLES;
    sensor_data.iout_acs /= SENSOR_SAMPLES;

    if (USE_ACS_CURRENT_SENSOR) {
        sensor_data.iout = sensor_data.iout_acs;
    } else {
        sensor_data.iout = sensor_data.iout_ina;
    }
}
