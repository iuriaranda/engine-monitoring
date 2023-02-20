#include "voltage_sensor.h"

namespace sensesp {

VoltageSensor::VoltageSensor(Adafruit_ADS1115* ads1115, int channel, uint read_delay, String config_path)
    : FloatSensor(config_path),
      ads1115_{ads1115},
      read_delay_{read_delay},
      channel_{channel} {
    load_configuration();
}

void VoltageSensor::start() {
    ReactESP::app->onRepeat(read_delay_, [this]() { this->update(); });
}

void VoltageSensor::update() {
    int16_t adc_output = ads1115_->readADC_SingleEnded(channel_);
    float adc_output_volts = ads1115_->computeVolts(adc_output);
    this->emit(ADS1115INPUTSCALE * adc_output_volts / ADS1115MEASUREMENTCURRENT);
};

void VoltageSensor::get_configuration(JsonObject& root) {
    root["read_delay"] = read_delay_;
    root["channel"] = channel_;
};

static const char SCHEMA[] PROGMEM = R"###({
    "type": "object",
    "properties": {
        "read_delay": { "title": "Read delay", "type": "number", "description": "Number of milliseconds between each analogRead(A0)" },
        "channel": { "title": "Sensor channel", "type": "number", "description": "Channel in the ADS1115 where the sensor is" }
    }
  })###";

String VoltageSensor::get_config_schema() { return FPSTR(SCHEMA); }

bool VoltageSensor::set_configuration(const JsonObject& config) {
    String expected[] = {"read_delay", "channel"};
    for (auto str : expected) {
        if (!config.containsKey(str)) {
            return false;
        }
    }
    read_delay_ = config["read_delay"];
    channel_ = config["channel"];
    return true;
}

}  // namespace sensesp
