#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "sensor.h"
#include "telemetry.h"
#include "webserver.h"
#include "mppt_control.h"
#include "tasks.h"

void setup() {
  Serial.begin(115200);

  // Setup pins
  pinMode(PIN_HIN, OUTPUT);
  pinMode(PIN_NLIN, OUTPUT);
  digitalWrite(PIN_HIN, LOW);
  digitalWrite(PIN_NLIN, HIGH);

  pinMode(PIN_ADC_ALERT, INPUT);
  pinMode(PIN_NTC1, INPUT);
  pinMode(PIN_NTC2, INPUT);

  // Setup H bridge in turned off state
  ledcSetup(HIN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(NLIN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_HIN, HIN_CHANNEL);
  ledcAttachPin(PIN_NLIN, NLIN_CHANNEL);
  ledcWrite(HIN_CHANNEL, 0);
  ledcWrite(NLIN_CHANNEL, 255);

  // Setup wifi related components
  setup_wifi();
  start_telemetry_task();
  start_wifi_monitor_task();
  setup_webserver();
  

  // Setup sensors
  Wire.begin(PIN_SDA, PIN_SCL, I2C_FREQ);
  ina_input.begin();
  ina_output.begin();
  acs_output.begin(ADDR_ACS_OUTPUT);
  ina_input.setCalibration_05ohm();
  ina_output.setCalibration_01ohm();
}

unsigned long loop_time_prev = 0;
void loop() {
  unsigned long now = millis();

  // Read sensors and update MPPT loop
  if (now - loop_time_prev >= MPPT_DELAY_MS) {
    loop_time_prev = now;
    update_sensor_data();
    mppt_loop();
  }

}
