#ifndef __SRC_CONFIGURATION_H__
#define __SRC_CONFIGURATION_H__

#include "sensesp/transforms/curveinterpolator.h"

// 1-Wire data pin on SH-ESP32
#define ONEWIRE_PIN 4

// Alternator W-terminal pin on SH-ESP32
#define RPM_PIN 15  // Digital input 1 in the engine top hat (connector pin 2)
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

#define SERIAL1_RX_PIN GPIO_NUM_21
#define SERIAL1_TX_PIN GPIO_NUM_23

#define PZCT02_BURDEN_RESISTANCE 18
#define PZCT02_MULTIPLIER 1 / 1000

// Default capacity value for the fuel tank, in cubic meters (m3)
#define FUEL_TANK_CAPACITY 140. / 1000
// Milimeters reading of the ultrasonic sensor that represents full tank
#define FUEL_TANK_FULL_MM 200
// Milimeters reading of the ultrasonic sensor that represents empty tank
#define FUEL_TANK_EMPTY_MM 0

// Default capacity value for the fresh water tank, in cubic meters (m3)
#define FRESH_WATER_TANK_CAPACITY 300. / 1000

// Channel numbers for the ADS1115 pins (analog pins in the engine hat)
#define FRESH_WATER_TANK_SENSOR_CHANNEL 0     // A
#define ENGINE_OIL_PRESSURE_SENSOR_CHANNEL 1  // B (connector pin 1)
#define ENGINE_COOLANT_TEMP_SENSOR_CHANNEL 2  // C (connector pin 3)
#define ALTERNATOR_OUTPUT_SENSOR_CHANNEL 3    // D

#define ctok(c) c + 273.15
#define bartopa(bar) bar * 100000

namespace sensesp {

class OilPressureSender : public CurveInterpolator {
   public:
    OilPressureSender(String config_path = "")
        : CurveInterpolator(NULL, config_path) {
        // Populate a lookup table tp translate the ohm values returned by
        // our pressure sender to Pa
        clear_samples();
        // addSample(CurveInterpolator::Sample(knownOhmValue, knownPa));
        add_sample(CurveInterpolator::Sample(0, bartopa(0)));
        add_sample(CurveInterpolator::Sample(10, bartopa(0)));
        add_sample(CurveInterpolator::Sample(52, bartopa(2)));
        add_sample(CurveInterpolator::Sample(124, bartopa(6)));
        add_sample(CurveInterpolator::Sample(184, bartopa(10)));
        add_sample(CurveInterpolator::Sample(200, bartopa(10)));
        // TODO: figure out the pressure range of the sender

        set_input_title("Sender Resistance (ohms)");
        set_output_title("Pressure (Pa)");
    }
};

class CoolantTempSender : public CurveInterpolator {
   public:
    CoolantTempSender(String config_path = "")
        : CurveInterpolator(NULL, config_path) {
        // Populate a lookup table tp translate the ohm values returned by
        // our temperatures sender to degrees kelvin
        clear_samples();
        // addSample(CurveInterpolator::Sample(knownOhmValue, knownKelvin));
        add_sample(CurveInterpolator::Sample(356.64, ctok(35)));
        add_sample(CurveInterpolator::Sample(291.46, ctok(40)));
        add_sample(CurveInterpolator::Sample(239.56, ctok(45)));
        add_sample(CurveInterpolator::Sample(197.29, ctok(50)));
        add_sample(CurveInterpolator::Sample(161.46, ctok(55)));
        add_sample(CurveInterpolator::Sample(134.03, ctok(60)));
        add_sample(CurveInterpolator::Sample(113.96, ctok(65)));
        add_sample(CurveInterpolator::Sample(97.05, ctok(70)));
        add_sample(CurveInterpolator::Sample(82.36, ctok(75)));
        add_sample(CurveInterpolator::Sample(70.12, ctok(80)));
        add_sample(CurveInterpolator::Sample(59.73, ctok(85)));
        add_sample(CurveInterpolator::Sample(51.21, ctok(90)));
        add_sample(CurveInterpolator::Sample(44.32, ctok(95)));
        add_sample(CurveInterpolator::Sample(38.47, ctok(100)));
        add_sample(CurveInterpolator::Sample(33.40, ctok(105)));
        add_sample(CurveInterpolator::Sample(29.12, ctok(110)));
        add_sample(CurveInterpolator::Sample(25.53, ctok(115)));
        add_sample(CurveInterpolator::Sample(22.44, ctok(120)));

        set_input_title("Sender Resistance (ohms)");
        set_output_title("Temperature (kelvin)");
    }
};

class TankLevelSender : public CurveInterpolator {
   public:
    TankLevelSender(String config_path = "")
        : CurveInterpolator(NULL, config_path) {
        // Populate a lookup table tp translate the ohm values returned by
        // our water float sender to a ratio level
        clear_samples();
        // addSample(CurveInterpolator::Sample(knownOhmValue, knownLevel));
        add_sample(CurveInterpolator::Sample(0, 0));
        add_sample(CurveInterpolator::Sample(180., 1));
        add_sample(CurveInterpolator::Sample(300., 1));

        set_input_title("Sender Resistance (ohms)");
        set_output_title("Water level (ratio)");
    }
};

}  // namespace sensesp

#endif
