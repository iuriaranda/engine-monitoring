#ifndef __SRC_CONFIGURATION_H__
#define __SRC_CONFIGURATION_H__

// 1-Wire data pin on SH-ESP32
#define ONEWIRE_PIN 4

// Alternator W-terminal pin on SH-ESP32
#define RPM_PIN 15
#define RPM_MULTIPLIER 1.0 / 1.0

// I2C (SDA and SCL) pins on SH-ESP32
#define SDA_PIN 16
#define SCL_PIN 17

// ADS1115 I2C address
#define ADS1115ADDR 0x4b

// ADS1115 input hardware scale factor (input voltage vs voltage at ADS1115)
#define ADS1115INPUTSCALE 29. / 2.048

// Engine Hat constant measurement current (A)
#define ADS1115MEASUREMENTCURRENT 0.01

// CAN bus (NMEA 2000) pins on SH-ESP32
#define CAN_RX_PIN GPIO_NUM_34
#define CAN_TX_PIN GPIO_NUM_32

#endif
