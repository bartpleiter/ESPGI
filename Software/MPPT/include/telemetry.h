#pragma once
#include <Arduino.h>
#include "sensor.h"

struct TelemetryData {
    float vin;
    float iin;
    float vout;
    float iout;
    float iout_ina;
    float iout_acs;
    float power_in;
    float power_out;
    float efficiency;
    int duty_cycle;
};

extern TelemetryData telemetry_data;

void set_telemetry_data();
String telemetry_json();
