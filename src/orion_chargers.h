#ifndef __SRC_ORION_CHARGERS_H__
#define __SRC_ORION_CHARGERS_H__

#include "sensesp/system/configurable.h"
#include "sensesp/system/startable.h"

namespace sensesp {

class OrionChargers : public Configurable, Startable {
   public:
    OrionChargers(String config_path = "");
    void start() override final;
    virtual void get_configuration(JsonObject& doc) override final;
    virtual bool set_configuration(const JsonObject& config) override final;
    virtual String get_config_schema() override;
};

}  // namespace sensesp

#endif
