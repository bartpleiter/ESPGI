#pragma once

#define USE_ACS_CURRENT_SENSOR true // Whether to use ACS current sensor instead of INA219
#define DUTY_STEP 1
#define MPPT_DELAY_MS 500
#define SENSOR_SAMPLES 10
#define MPPT_LOW_CURRENT_THRESHOLD 0.03 // In Amps. If the current is below this, the duty cycle will increase
#define MAX_VOUT 13.6 // Maximum output voltage
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

// Network configuration
#define LOCAL_IP {192, 168, 0, 235}
#define GATEWAY {192, 168, 0, 1}
#define SUBNET {255, 255, 255, 0}
#define PRIMARY_DNS {8, 8, 8, 8}
#define SECONDARY_DNS {1, 1, 1, 1}

#define TELEMETRY_UPDATE_INTERVAL_MS 1000 // 1 second

