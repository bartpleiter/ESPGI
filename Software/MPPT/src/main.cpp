// TODO: This is all quite messy PoC code, refactor it later

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>

#ifndef WIFI_SSID
#error "Wi-Fi SSID not defined! Set WIFI_SSID in environment."
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

#define USE_ACS_CURRENT_SENSOR true // Whether to use ACS current sensor instead of INA219
#define DUTY_STEP 1
#define MPPT_DELAY_MS 200
#define SENSOR_SAMPLES 10
#define MPPT_LOW_CURRENT_THRESHOLD 0.03 // In Amps. If the current is below this, the duty cycle will increase
#define MAX_VOUT 13.4 // Maximum output voltage
#define MIN_VIN 3.0 // Minimum input voltage to start MPPT

#define OFFSET_IIN 0.0002 // Offset in Amps to add to the input current sensor
#define OFFSET_IOUT_INA 0.0002 // Offset in Amps to add to the output current sensor (INA219)
#define OFFSET_IOUT_ACS 0.0060 // Offset in Amps to add to the output current sensor (ACS)

#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_HIN 19
#define PIN_NLIN 18
#define PIN_UART2_TX 17
#define PIN_UART2_RX 16
#define PIN_NTC1 34
#define PIN_NTC2 35
#define PIN_ADC_ALERT 32

#define ADDR_INA_INPUT 0x40
#define ADDR_INA_OUTPUT 0x44
#define ADDR_ACS_OUTPUT 0x48

#define HIN_CHANNEL 0
#define NLIN_CHANNEL 1
#define PWM_FREQ 100000
#define PWM_RESOLUTION 8
#define MAX_DUTY_CYCLE 240 // A bit less than max to keep boostrap working
#define MIN_DUTY_CYCLE 64 // Lower than this create a too low output voltage to be useful

#define I2C_FREQ 400000 // 400kHz

#define WEB_SERVER_PORT 80
#define WIFI_CHECK_INTERVAL_MS 10000 // 10s
#define MAX_WIFI_RETRIES 10

IPAddress local_IP(192, 168, 0, 235);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

int wifi_reconnect_attempts = 0;

AsyncWebServer server(WEB_SERVER_PORT);

Adafruit_INA219 ina_input(ADDR_INA_INPUT);
Adafruit_INA219 ina_output(ADDR_INA_OUTPUT);
Adafruit_ADS1115 acs_output;

// MPPT control parameters
int duty_cycle = 120;
float prev_power = 0;
int direction = 1; // 1 = increase duty, -1 = decrease duty

// Struct that holds the input and output voltage and current
struct SensorData {
  float vin;
  float iin;
  float vout;
  float iout;
  float iout_ina;
  float iout_acs;
};
SensorData sensor_data;

// Struct that holds the telemetry data
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
  float ntc1_temp;
  float ntc2_temp;
  int duty_cycle;
};
TelemetryData telemetry_data;

void set_duty_cycle(uint8_t duty) {
  if (duty == 0) {
    // Set NLIN to 1 and HIN to 0 to disable both MOSFETs
    ledcWrite(HIN_CHANNEL, 0);
    ledcWrite(NLIN_CHANNEL, 255);
  }
  ledcWrite(HIN_CHANNEL, duty);
  ledcWrite(NLIN_CHANNEL, duty);
}

float get_input_voltage() {
  // Input voltage in Volts
  return ina_input.getBusVoltage_V();
}

float get_output_voltage() {
  // Output voltage in Volts
  return ina_output.getBusVoltage_V();
}

float get_input_current() {
  // Input current in Amps
  return (ina_input.getCurrent_mA() / 1000.0) + OFFSET_IIN; // Convert mA to A and add offset
}

float get_output_current_acs() {
  // Output current in Amps
  int16_t raw = acs_output.readADC_Differential_0_1();
  float voltage = acs_output.computeVolts(raw);
  float current = voltage / 0.4; // 400mV/A
  return current + OFFSET_IOUT_ACS; // Add offset
}

float get_output_current_ina() {
  // Output current in Amps
  return (ina_output.getCurrent_mA() / 1000.0) + OFFSET_IOUT_INA; // Convert mA to A and add offset
}

float get_output_current() {
  // Output current in Amps
  if (USE_ACS_CURRENT_SENSOR) {
    return get_output_current_acs();
  } else {
    return get_output_current_ina();
  }
}

void set_telemetry_data() {
  // Set the telemetry data struct with the current sensor data
  telemetry_data.vin = sensor_data.vin;
  telemetry_data.iin = sensor_data.iin;
  telemetry_data.vout = sensor_data.vout;
  telemetry_data.iout = sensor_data.iout;
  telemetry_data.iout_ina = sensor_data.iout_ina;
  telemetry_data.iout_acs = sensor_data.iout_acs;
  telemetry_data.power_in = sensor_data.vin * sensor_data.iin;
  telemetry_data.power_out = sensor_data.vout * sensor_data.iout;
  // Set efficiency to 0 if input current is < 0.01A
  if (sensor_data.iin > 0.01 && telemetry_data.power_in > 0 && telemetry_data.power_out > 0) {
    telemetry_data.efficiency = (telemetry_data.power_out / telemetry_data.power_in) * 100.0;
    // Clamp efficiency to 0-100%
    if (telemetry_data.efficiency > 100) {
      telemetry_data.efficiency = 100;
    } else if (telemetry_data.efficiency < 0) {
      telemetry_data.efficiency = 0;
    }
  } else {
    telemetry_data.efficiency = 0;
  }
  telemetry_data.ntc1_temp = 0.0; // TODO: Read NTC1 temperature
  telemetry_data.ntc2_temp = 0.0; // TODO: Read NTC2 temperature
  telemetry_data.duty_cycle = duty_cycle;
}

void update_sensor_data() {
  // Fills the sensor_data struct with the average of multiple sensor readings to reduce noise
  float vin_samples[SENSOR_SAMPLES];
  float iin_samples[SENSOR_SAMPLES];
  float vout_samples[SENSOR_SAMPLES];
  float iout_ina_samples[SENSOR_SAMPLES];
  float iout_acs_samples[SENSOR_SAMPLES];

  for (int i = 0; i < SENSOR_SAMPLES; i++) {
    // A single 2 byte INA219 register read takes about 150uS at 400kKz
    vin_samples[i] = get_input_voltage();
    iin_samples[i] = get_input_current();

    vout_samples[i] = get_output_voltage();
    iout_ina_samples[i] = get_output_current_ina();

    // A single ADS1115 conversion takes about 860uS, plus some I2C overhead
    iout_acs_samples[i] = get_output_current_acs();

    // Therefore, no delay is needed between samples, and this loop will take ~2ms
  }

  // Average the samples
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

  // Set iout based on the selected sensor
  if (USE_ACS_CURRENT_SENSOR) {
    sensor_data.iout = sensor_data.iout_acs;
  } else {
    sensor_data.iout = sensor_data.iout_ina;
  }
}


void mppt_loop() {
  // Main loop of the MPPT algorithm

  // Vars needed for Perturb and Observe
  float vin = sensor_data.vin;
  float iin = sensor_data.iin;
  float vout = sensor_data.vout;
    
    // --- Stop conditions ---
    if (vout >= MAX_VOUT || vout > vin || vin < MIN_VIN) {
        duty_cycle = 0;  // Turn off
        set_duty_cycle(duty_cycle);
        return;
    }

    // --- Increase duty cycle if input current is low ---
    if (iin < MPPT_LOW_CURRENT_THRESHOLD) {
        if (duty_cycle < MIN_DUTY_CYCLE) duty_cycle = MIN_DUTY_CYCLE;
        duty_cycle += DUTY_STEP;
        if (duty_cycle > MAX_DUTY_CYCLE) duty_cycle = MAX_DUTY_CYCLE;
        set_duty_cycle(duty_cycle);
        return;
    }

    // --- Measure input power ---
    float power = vin * iin;

    // --- Perturb duty cycle ---
    if (power > prev_power) {
        // Power increased, continue in same direction
        duty_cycle += direction * DUTY_STEP;
    } else {
        // Power decreased, reverse direction
        direction *= -1;
        duty_cycle += direction * DUTY_STEP;
    }

    // Clamp duty cycle within valid range
    if (duty_cycle > MAX_DUTY_CYCLE) duty_cycle = MAX_DUTY_CYCLE;
    if (duty_cycle < MIN_DUTY_CYCLE) duty_cycle = MIN_DUTY_CYCLE;

    set_duty_cycle(duty_cycle);
    prev_power = power;
}


void report_status_uart() {
  // Calculate input and output power
  float input_power = sensor_data.vin * sensor_data.iin;
  float output_power = sensor_data.vout * sensor_data.iout;
  float efficiency = (output_power / input_power) * 100.0;
  // Set efficiency to 0 if input current is < 0.01A
  if (sensor_data.iin < 0.01) {
    efficiency = 0;
  }
  Serial.print("Vin:      ");
  Serial.print(sensor_data.vin, 3);
  Serial.print(" V\nIin:      ");
  Serial.print(sensor_data.iin, 4);
  Serial.print(" A\nPin:      ");
  Serial.print(input_power, 4);
  Serial.print(" W\nVout:     ");
  Serial.print(sensor_data.vout, 3);
  Serial.print(" V\nIout_ina: ");
  Serial.print(sensor_data.iout_ina, 4);
  Serial.print(" A\nIout_acs: ");
  Serial.print(sensor_data.iout_acs, 4);
  Serial.print(" A\nPout:     ");
  Serial.print(output_power, 4);
  Serial.print(" W\nEff:      ");
  Serial.print(efficiency, 2);
  Serial.print(" %\nPWM:      ");
  Serial.println(duty_cycle);
  Serial.println("==================");
}

void setup_webserver() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello from ESP32!");
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
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
    json += "\"ntc1_temp\":" + String(telemetry_data.ntc1_temp, 2) + ",";
    json += "\"ntc2_temp\":" + String(telemetry_data.ntc2_temp, 2) + ",";
    json += "\"duty_cycle\":" + String(telemetry_data.duty_cycle);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void setup_wifi() {
  // Configure static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure static IP");
  }

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void ensure_wifi_connected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, trying to reconnect...");
    WiFi.disconnect();  // Helps with flaky reconnects
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    // Retry for up to 5 seconds
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
      delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to WiFi");
      wifi_reconnect_attempts = 0;  // Reset counter on success
    } else {
      wifi_reconnect_attempts++;
      Serial.printf("Failed to reconnect (attempt %d/%d)\n", wifi_reconnect_attempts, MAX_WIFI_RETRIES);

      if (wifi_reconnect_attempts >= MAX_WIFI_RETRIES) {
        Serial.println("Too many failed attempts. Restarting...");
        delay(200);  // Brief pause to flush serial output
        ESP.restart();  // Reset the ESP32
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Set up pins
  pinMode(PIN_HIN, OUTPUT);
  pinMode(PIN_NLIN, OUTPUT);
  digitalWrite(PIN_HIN, LOW);
  digitalWrite(PIN_NLIN, HIGH);

  pinMode(PIN_ADC_ALERT, INPUT);
  pinMode(PIN_NTC1, INPUT);
  pinMode(PIN_NTC2, INPUT);

  ledcSetup(HIN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(NLIN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_HIN, HIN_CHANNEL);
  ledcAttachPin(PIN_NLIN, NLIN_CHANNEL);
  ledcWrite(HIN_CHANNEL, duty_cycle);
  ledcWrite(NLIN_CHANNEL, duty_cycle);

  Wire.begin(PIN_SDA, PIN_SCL, I2C_FREQ);

  ina_input.begin();
  ina_output.begin();
  acs_output.begin(ADDR_ACS_OUTPUT);

  ina_input.setCalibration_05ohm();
  ina_output.setCalibration_01ohm();

  setup_wifi();
  setup_webserver();
}

unsigned long loop_time_prev = 0;
unsigned long wifi_check_prev = 0;
void loop() {
  unsigned long now = millis();

  // MPPT loop
  if (now - loop_time_prev >= MPPT_DELAY_MS) {
    loop_time_prev = now;
    update_sensor_data();
    set_telemetry_data();
    mppt_loop();
  }

  // WiFi reconnection logic
  if (now - wifi_check_prev >= WIFI_CHECK_INTERVAL_MS) {
    wifi_check_prev = now;
    ensure_wifi_connected();
  }
}
