#ifndef __SRC_DS1603L_SENSOR_H__
#define __SRC_DS1603L_SENSOR_H__

#include <DS1603L.h>
#include <HardwareSerial.h>

#include "configuration.h"
#include "sensesp.h"
#include "sensesp/sensors/sensor.h"

namespace sensesp {

class FuelTankSensor : public FloatSensor {
   public:
    FuelTankSensor(int8_t empty_mm, int8_t full_mm, uint read_delay = 500, String config_path = "");
    void start() override final;
    virtual void get_configuration(JsonObject& doc) override final;
    virtual bool set_configuration(const JsonObject& config) override final;
    virtual String get_config_schema() override;
    float getSensorReading();

   private:
    DS1603L* ds1603l_;
    int8_t empty_mm_;
    int8_t full_mm_;
    uint read_delay_;
};

}  // namespace sensesp

#endif
