#ifndef __SRC_VOLTAGE_SENSOR_H__
#define __SRC_VOLTAGE_SENSOR_H__

#include <Adafruit_ADS1X15.h>

#include "configuration.h"
#include "sensesp.h"
#include "sensesp/sensors/sensor.h"

namespace sensesp {

class VoltageSensor : public FloatSensor {
   public:
    VoltageSensor(Adafruit_ADS1115* ads1115, int channel, uint read_delay = 500, String config_path = "");
    void start() override final;
    virtual void get_configuration(JsonObject& doc) override final;
    virtual bool set_configuration(const JsonObject& config) override final;
    virtual String get_config_schema() override;

   protected:
    Adafruit_ADS1115* ads1115_;
    uint read_delay_;
    int channel_;
    void update();
};

}  // namespace sensesp

#endif
