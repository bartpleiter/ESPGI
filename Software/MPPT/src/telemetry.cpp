#include <Arduino.h>
#include <string>
#include "telemetry.h"
#include "sensor.h"
#include "mppt_control.h"

TelemetryData telemetry_data;

void set_telemetry_data() {
    telemetry_data.vin = sensor_data.vin;
    telemetry_data.iin = sensor_data.iin;
    telemetry_data.vout = sensor_data.vout;
    telemetry_data.iout = sensor_data.iout;
    telemetry_data.iout_ina = sensor_data.iout_ina;
    telemetry_data.iout_acs = sensor_data.iout_acs;
    telemetry_data.power_in = sensor_data.vin * sensor_data.iin;
    telemetry_data.power_out = sensor_data.vout * sensor_data.iout;
    if (sensor_data.iin > 0.01 && telemetry_data.power_in > 0 && telemetry_data.power_out > 0) {
        telemetry_data.efficiency = (telemetry_data.power_out / telemetry_data.power_in) * 100.0;
        if (telemetry_data.efficiency > 100) telemetry_data.efficiency = 100;
        else if (telemetry_data.efficiency < 0) telemetry_data.efficiency = 0;
    } else {
        telemetry_data.efficiency = 0;
    }
    telemetry_data.duty_cycle = duty_cycle;
}

String telemetry_json() {
    String json = "{";
    json += "\"vin\":" + String(telemetry_data.vin, 3) + ",";
    json += "\"iin\":" + String(telemetry_data.iin, 4) + ",";
    json += "\"vout\":" + String(telemetry_data.vout, 3) + ",";
    json += "\"iout\":" + String(telemetry_data.iout, 4) + ",";
    json += "\"iout_ina\":" + String(telemetry_data.iout_ina, 4) + ",";
    json += "\"iout_acs\":" + String(telemetry_data.iout_acs, 4) + ",";
    json += "\"power_in\":" + String(telemetry_data.power_in, 4) + ",";
    json += "\"power_out\":" + String(telemetry_data.power_out, 4) + ",";
    json += "\"efficiency\":" + String(telemetry_data.efficiency, 2) + ",";
    json += "\"duty_cycle\":" + String(telemetry_data.duty_cycle);
    json += "}";
    return json;
}
