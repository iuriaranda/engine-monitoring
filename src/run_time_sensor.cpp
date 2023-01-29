#include "run_time_sensor.h"

#include "sensesp/system/lambda_consumer.h"

namespace sensesp {

RunTimeSensor::RunTimeSensor(ValueProducer<float>* running_sensor, uint update_period, uint save_period, String config_path)
    : FloatSensor(config_path),
      update_period_{update_period},
      save_period_{save_period} {
    last_update_ = millis();

    load_configuration();

    running_sensor->connect_to(new LambdaConsumer<float>([&](float value) {
        if (value > 0) {
            if (!is_running_) {
                // If engine was stopped and got started, reset last_update_
                last_update_ = millis();
            }
            is_running_ = true;
        } else {
            if (is_running_) {
                // If engine was running and it stopped, update and save run time now
                this->update();
                this->save();
            }
            is_running_ = false;
        }
    }));
}

void RunTimeSensor::start() {
    ReactESP::app->onRepeat(update_period_, [this]() { this->update(); });
    ReactESP::app->onRepeat(save_period_, [this]() { this->save(); });
}

void RunTimeSensor::update() {
    if (is_running_) {
        run_time_ += (millis() - last_update_) / 1000.;
    }

    last_update_ = millis();

    this->emit(round(run_time_));

    save_configuration();
};

void RunTimeSensor::save() {
    // Only save it to filesystem if the value changed from last save
    if (last_saved_run_time_ != run_time_) {
        last_saved_run_time_ = run_time_;
        save_configuration();
    }
}

void RunTimeSensor::get_configuration(JsonObject& root) {
    root["update_period"] = update_period_;
    root["save_period"] = save_period_;
    root["run_time"] = run_time_;
};

static const char SCHEMA[] PROGMEM = R"###({
    "type": "object",
    "properties": {
        "update_period": { "title": "Update period", "type": "number", "description": "Number of milliseconds between each run time update" },
        "save_period": { "title": "Save period", "type": "number", "description": "Number of milliseconds between each run time value save in filesystem" },
        "run_time": { "title": "Run time", "type": "number", "description": "The actual run time in seconds" }
    }
  })###";

String RunTimeSensor::get_config_schema() { return FPSTR(SCHEMA); }

bool RunTimeSensor::set_configuration(const JsonObject& config) {
    String expected[] = {"update_period", "save_period", "run_time"};
    for (auto str : expected) {
        if (!config.containsKey(str)) {
            return false;
        }
    }
    update_period_ = config["update_period"];
    save_period_ = config["save_period"];
    run_time_ = config["run_time"];

    return true;
}

}  // namespace sensesp
