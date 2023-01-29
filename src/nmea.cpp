#include "nmea.h"

#include "sensesp/system/valueproducer.h"

namespace sensesp {

Nmea::Nmea() {
    nmea2000_ = new tNMEA2000_esp32(CAN_TX_PIN, CAN_RX_PIN);

    // Reserve enough buffer for sending all messages. This does not work on small
    // memory devices like Uno or Mega
    nmea2000_->SetN2kCANSendFrameBufSize(250);
    nmea2000_->SetN2kCANReceiveFrameBufSize(250);

    // Set Product information
    nmea2000_->SetProductInformation(
        "20230111",                    // Manufacturer's Model serial code (max 32 chars)
        103,                           // Manufacturer's product code
        "SH-ESP32 Engine monitoring",  // Manufacturer's Model ID (max 33 chars)
        "0.1.0.0 (2023-01-11)",        // Manufacturer's Software version code (max 40 chars)
        "0.0.3.1 (2023-01-11)"         // Manufacturer's Model version (max 24 chars)
    );
    // Set device information
    nmea2000_->SetDeviceInformation(
        1,    // Unique number. Use e.g. Serial number.
        130,  // Device function=Analog to NMEA 2000 Gateway. See codes on
              // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
        75,   // Device class=Inter/Intranetwork Device. See codes on
              // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
        2046  // Just choosen free from code list on
              // http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );

    nmea2000_->SetMode(tNMEA2000::N2km_NodeOnly, 22);
    // Disable all msg forwarding to USB (=Serial)
    nmea2000_->EnableForward(false);
    nmea2000_->Open();

    // No need to parse the messages at every single loop iteration; 1 ms will do
    ReactESP::app->onRepeat(1, [&]() { nmea2000_->ParseMessages(); });
}

void Nmea::connect_oil_temperature(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        oil_temperature_ = value;
        this->sendEngineData();
    }));
}

void Nmea::connect_oil_pressure(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        oil_pressure_ = value;
        this->sendEngineData();
    }));
}

void Nmea::connect_coolant_temperature(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        coolant_temperature_ = value;
        this->sendEngineData();
    }));
}

void Nmea::connect_exhaust_temperature(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        this->sendExhaustTemperature(value);
    }));
}

void Nmea::connect_engine_rpms(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        this->sendEngineRpms(value);
    }));
}

void Nmea::connect_engine_run_time(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        engine_hours_ = round(value / 60 / 60);
        this->sendEngineData();
    }));
}

void Nmea::connect_water_level(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        water_level_ = value;
        this->sendFluidLevel();
    }));
}

void Nmea::connect_water_capacity(ValueProducer<float> *p) {
    p->connect_to(new LambdaConsumer<float>([&](float value) {
        water_capacity_ = value;
        this->sendFluidLevel();
    }));
}

void Nmea::sendFluidLevel() {
    tN2kMsg N2kMsg;
    SetN2kFluidLevel(
        N2kMsg,
        0,
        N2kft_Water,
        water_level_,
        water_capacity_);
    nmea2000_->SendMsg(N2kMsg);
}

/**
 * @brief Send Engine Dynamic Parameter data
 *
 * Send engine  data using the Engine Dynamic Parameter PGN.
 * All unused fields that are sent with undefined value except the status
 * bit fields are sent as zero. Hopefully we're not resetting anybody's engine
 * warnings...
 */
void Nmea::sendEngineData() {
    tN2kMsg N2kMsg;
    SetN2kEngineDynamicParam(N2kMsg,
                             0,                     // instance of a single engine is always 0
                             oil_pressure_,         // oil pressure
                             oil_temperature_,      // oil temperature
                             coolant_temperature_,  // coolant_temperature
                             N2kDoubleNA,           // alternator voltage
                             N2kDoubleNA,           // fuel rate
                             engine_hours_,         // engine hours
                             N2kDoubleNA,           // engine coolant pressure
                             N2kDoubleNA,           // engine fuel pressure
                             N2kInt8NA,             // engine load
                             N2kInt8NA,             // engine torque
                             (tN2kEngineDiscreteStatus1)0,
                             (tN2kEngineDiscreteStatus2)0);
    nmea2000_->SendMsg(N2kMsg);
}

void Nmea::sendExhaustTemperature(float temperature) {
    tN2kMsg N2kMsg;
    // hijack the exhaust gas temperature for wet exhaust temperature measurement
    SetN2kTemperature(N2kMsg,
                      1,                            // SID
                      2,                            // TempInstance
                      N2kts_ExhaustGasTemperature,  // TempSource
                      temperature                   // actual temperature
    );
    nmea2000_->SendMsg(N2kMsg);
}

void Nmea::sendEngineRpms(float rpms) {
    tN2kMsg N2kMsg;
    SetN2kPGN127488(
        N2kMsg,
        1,    // SID
        rpms  // RPMs
    );
    nmea2000_->SendMsg(N2kMsg);
}

}  // namespace sensesp
