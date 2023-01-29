#ifndef __SRC_RUN_TIME_SENSOR_H__
#define __SRC_RUN_TIME_SENSOR_H__

#include "sensesp.h"
#include "sensesp/sensors/sensor.h"

namespace sensesp {

class RunTimeSensor : public FloatSensor {
   public:
    RunTimeSensor(ValueProducer<float>* up_sensor, uint update_period = 1000, uint save_period = 5 * 60000, String config_path = "");
    void start() override final;
    virtual void get_configuration(JsonObject& doc) override final;
    virtual bool set_configuration(const JsonObject& config) override final;
    virtual String get_config_schema() override;

   private:
    void update();
    void save();
    uint update_period_;
    uint save_period_;
    float last_update_;
    bool is_running_ = false;
    float run_time_ = 0;
    float last_saved_run_time_ = 0;
};

}  // namespace sensesp

#endif
