#ifndef __SRC_NMEA_H__
#define __SRC_NMEA_H__

#include <N2kMessages.h>
#include <NMEA2000_esp32.h>

#include "configuration.h"
#include "sensesp.h"
#include "sensesp/system/lambda_consumer.h"

namespace sensesp {

class Nmea {
   public:
    Nmea();

    void connect_oil_temperature(ValueProducer<float> *p);
    void connect_oil_pressure(ValueProducer<float> *p);
    void connect_coolant_temperature(ValueProducer<float> *p);
    void connect_exhaust_temperature(ValueProducer<float> *p);
    void connect_engine_rpms(ValueProducer<float> *p);
    void connect_engine_run_time(ValueProducer<float> *p);
    void connect_water_level(ValueProducer<float> *p);
    void connect_water_capacity(ValueProducer<float> *p);

   private:
    void
    sendEngineData();
    void sendExhaustTemperature(float temperature);
    void sendEngineRpms(float rpms);
    void sendFluidLevel();

    tNMEA2000 *nmea2000_;
    double oil_temperature_ = N2kDoubleNA;
    double coolant_temperature_ = N2kDoubleNA;
    double oil_pressure_ = N2kDoubleNA;
    double engine_hours_ = N2kDoubleNA;
    double water_level_ = N2kDoubleNA;
    double water_capacity_ = N2kDoubleNA;
};

}  // namespace sensesp

#endif
