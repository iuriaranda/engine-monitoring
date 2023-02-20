#ifndef __SRC_RESISTANCE_SENSOR_H__
#define __SRC_RESISTANCE_SENSOR_H__

#include "voltage_sensor.h"

namespace sensesp {

class ResistanceSensor : public VoltageSensor {
   public:
    ResistanceSensor(Adafruit_ADS1115* ads1115, int channel, uint read_delay = 500, String config_path = "") : VoltageSensor(ads1115, channel, read_delay, config_path){};

   private:
    void update() {
        int16_t adc_output = ads1115_->readADC_SingleEnded(channel_);
        float adc_output_volts = ads1115_->computeVolts(adc_output);
        this->emit(ADS1115INPUTSCALE * adc_output_volts / ADS1115MEASUREMENTCURRENT);
    };
};

}  // namespace sensesp

#endif
