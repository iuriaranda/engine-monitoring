#include "fuel_tank_sensor.h"

namespace sensesp {

FuelTankSensor::FuelTankSensor(int8_t empty_mm, int8_t full_mm, uint read_delay, String config_path) : FloatSensor(config_path),
                                                                                                       empty_mm_{empty_mm},
                                                                                                       full_mm_{full_mm},
                                                                                                       read_delay_{read_delay} {
    ds1603l_ = new DS1603L(Serial1);
    load_configuration();
}

void FuelTankSensor::start() {
    Serial1.begin(9600, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);
    ds1603l_->begin();  // Initialise the sensor library.

    ReactESP::app->onRepeat(read_delay_, [this]() {
        long last_time = micros();
        while (micros() - last_time < 100) {
            yield();
        }

        int16_t reading = this->getSensorReading();
        if (reading > -1) {
            const float range = full_mm_ - empty_mm_;
            const float divisor = range / 100.0;
            const float multiplier = 1.0 / divisor;              //  (1 / 4.5 = 0.0222222)
            const float offset = 100.0 - full_mm_ * multiplier;  // (100 - (500 x 0.0222222) = 38.8889)
            this->emit(multiplier * reading + offset);
        }
    });
}

void FuelTankSensor::get_configuration(JsonObject& root) {
    root["read_delay"] = read_delay_;
    root["full_mm"] = full_mm_;
    root["empty_mm"] = empty_mm_;
};

static const char SCHEMA[] PROGMEM = R"###({
    "type": "object",
    "properties": {
        "read_delay": { "title": "Read delay", "type": "number", "description": "Number of milliseconds between each read" },
        "full_mm": { "title": "Full tank mm value", "type": "number", "description": "Milimeters reading of the ultrasonic sensor that represents full tank" },
        "empty_mm": { "title": "Empty tank mm value", "type": "number", "description": "Milimeters reading of the ultrasonic sensor that represents empty tank" }
    }
  })###";

String FuelTankSensor::get_config_schema() { return FPSTR(SCHEMA); }

bool FuelTankSensor::set_configuration(const JsonObject& config) {
    String expected[] = {"read_delay", "full_mm", "empty_mm"};
    for (auto str : expected) {
        if (!config.containsKey(str)) {
            return false;
        }
    }
    read_delay_ = config["read_delay"];
    full_mm_ = config["full_mm"];
    empty_mm_ = config["empty_mm"];
    return true;
}

float FuelTankSensor::getSensorReading() {
    uint8_t reading = ds1603l_->readSensor();   // Call this as often or as little as you want - the sensor transmits every 1-2 seconds.
    byte sensorStatus = ds1603l_->getStatus();  // Check the status of the sensor (not detected; checksum failed; reading success).
    switch (sensorStatus) {                     // For possible values see DS1603L.h
        case DS1603L_NO_SENSOR_DETECTED:        // No sensor detected: no valid transmission received for >10 seconds.
            debugD("No sensor detected (yet). If no sensor after 1 second, check whether your connections are good.");
            break;

        case DS1603L_READING_CHECKSUM_FAIL:  // Checksum of the latest transmission failed.
            debugD("Data received; checksum failed. Latest level reading: %d mm.", reading);
            break;

        case DS1603L_READING_SUCCESS:  // Latest reading was valid and received successfully.
            debugD("Reading success. Water level: %d mm.", reading);
            return reading;
            break;
    }

    return -1;
}

}  // namespace sensesp
