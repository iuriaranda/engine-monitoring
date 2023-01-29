#include <Adafruit_ADS1X15.h>

#include "configuration.h"
#include "nmea.h"
#include "resistance_sensor.h"
#include "run_time_sensor.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/transforms/curveinterpolator.h"
#include "sensesp/transforms/frequency.h"
#include "sensesp/transforms/linear.h"
#include "sensesp_app_builder.h"
#include "sensesp_onewire/onewire_temperature.h"

using namespace sensesp;

ReactESP app;

// Convenience function to print the addresses found on the I2C bus
void ScanI2C(TwoWire *i2c) {
    uint8_t error, address;

    Serial.println("Scanning...");

    for (address = 1; address < 127; address++) {
        i2c->beginTransmission(address);
        error = i2c->endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("");
        } else if (error == 4) {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    Serial.println("done");
}

#define debugValueProducer(p, fmt)                             \
    p->connect_to(new LambdaConsumer<float>([&](float value) { \
        debugD(fmt, value);                                    \
    }));

class OilPressureSender : public CurveInterpolator {
   public:
    OilPressureSender(String config_path = "")
        : CurveInterpolator(NULL, config_path) {
        // Populate a lookup table tp translate the ohm values returned by
        // our pressure sender to Pa
        clear_samples();
        // addSample(CurveInterpolator::Sample(knownOhmValue, knownPa));
        add_sample(CurveInterpolator::Sample(10000, 0));
        add_sample(CurveInterpolator::Sample(240, 0));
        add_sample(CurveInterpolator::Sample(153, 172369));
        add_sample(CurveInterpolator::Sample(103, 344738));
        add_sample(CurveInterpolator::Sample(67, 517107));
        add_sample(CurveInterpolator::Sample(33, 689476));
        add_sample(CurveInterpolator::Sample(0, 689476));

        set_input_title("Sender Resistance (ohms)");
        set_output_title("Pressure (Pa)");
    }
};

class CoolantTempSender : public CurveInterpolator {
   public:
    CoolantTempSender(String config_path = "")
        : CurveInterpolator(NULL, config_path) {
        // Populate a lookup table tp translate the ohm values returned by
        // our temperatures sender to degrees kelvin
        clear_samples();
        // addSample(CurveInterpolator::Sample(knownOhmValue, knownKelvin));
        add_sample(CurveInterpolator::Sample(2382, 294.26));
        add_sample(CurveInterpolator::Sample(1838, 299.82));
        add_sample(CurveInterpolator::Sample(1431, 305.37));
        add_sample(CurveInterpolator::Sample(888, 316.48));
        add_sample(CurveInterpolator::Sample(460, 333.15));
        add_sample(CurveInterpolator::Sample(210, 355.37));
        add_sample(CurveInterpolator::Sample(123.7, 372.04));
        add_sample(CurveInterpolator::Sample(76.24, 388.71));
        add_sample(CurveInterpolator::Sample(32.4, 422.04));

        set_input_title("Sender Resistance (ohms)");
        set_output_title("Temperature (kelvin)");
    }
};

class TankLevelSender : public CurveInterpolator {
   public:
    TankLevelSender(String config_path = "")
        : CurveInterpolator(NULL, config_path) {
        // Populate a lookup table tp translate the ohm values returned by
        // our water float sender to a ratio level
        clear_samples();
        // addSample(CurveInterpolator::Sample(knownOhmValue, knownLevel));
        add_sample(CurveInterpolator::Sample(0, 0));
        add_sample(CurveInterpolator::Sample(180., 1));
        add_sample(CurveInterpolator::Sample(300., 1));

        set_input_title("Sender Resistance (ohms)");
        set_output_title("Water level (ratio)");
    }
};

class TankCapacity : public ValueProducer<float>, public Startable {
   public:
    TankCapacity(Linear *linear) : _linear{linear} {}
    void start() override final {
        app.onRepeat(60000, [&]() { this->update(); });
        this->update();
    }

   private:
    Linear *_linear;
    void update() {
        DynamicJsonDocument jsonDoc(2048);
        JsonObject obj = jsonDoc.createNestedObject("root");
        _linear->get_configuration(obj);
        emit(obj["multiplier"]);
    }
};

void setup() {
#ifndef SERIAL_DEBUG_DISABLED
    SetupSerialDebug(115200);
#endif

    // Initialize the NMEA 2000 subsystem
    auto nmea = new Nmea();

    // initialize the I2C bus
    TwoWire *i2c = new TwoWire(0);
    i2c->begin(SDA_PIN, SCL_PIN);

    ScanI2C(i2c);

    // Initialize ADS1115
    auto ads1115 = new Adafruit_ADS1115();
    ads1115->setGain(GAIN_ONE);
    bool ads_initialized = ads1115->begin(ADS1115ADDR, i2c);
    debugD("ADS1115 initialized: %d", ads_initialized);

    DallasTemperatureSensors *dts = new DallasTemperatureSensors(ONEWIRE_PIN);

    SensESPAppBuilder builder;

    sensesp_app = builder.set_hostname("EngineMonitoring")
                      ->enable_ip_address_sensor()
                      ->enable_system_info_sensors()
                      ->enable_uptime_sensor()
                      ->enable_wifi_signal_sensor()
                      ->get_app();

    // Set up sensors

    // Engine room temperature
    auto engine_room_temperature = new OneWireTemperature(dts, 1000, "/data/engine_room_temperature/sensor");
    engine_room_temperature->connect_to(new SKOutputFloat(
        "environment.inside.engineRoom.temperature"
        "/data/engine_room_temperature/sk_path",
        "K"));

    // Engine alternator temperature
    auto engine_alternator_temperature = new OneWireTemperature(dts, 1000, "/data/engine_alternator_temperature/sensor");
    engine_alternator_temperature->connect_to(new SKOutputFloat(
        "electrical.alternators.engine.temperature"
        "/data/engine_alternator_temperature/sk_path",
        "K"));

    // Engine RPMs
    auto engine_rpms_raw = new DigitalInputCounter(RPM_PIN, INPUT, RISING, 500, "/data/engine_rpms/sensor");
    auto engine_rpms = engine_rpms_raw->connect_to(new Frequency(RPM_MULTIPLIER, "/data/engine_rpms/multiplier"));
    nmea->connect_engine_rpms(engine_rpms);
    engine_rpms->connect_to(new SKOutputFloat(
        "propulsion.main.revolutions",
        "/data/engine_rpms/sk_path",
        "Hz"));

    // Engine run time
    auto engine_runtime = new RunTimeSensor(engine_rpms, 10000, 5 * 60000, "/data/engine_runtime");
    nmea->connect_engine_run_time(engine_runtime);
    engine_runtime->connect_to(new SKOutputFloat(
        "propulsion.main.runTime",
        "/data/engine_runtime/sk_path",
        "s"));

    // Engine exhaust temperature
    auto engine_exhaust_temperature = new OneWireTemperature(dts, 1000, "/data/engine_exhaust_temperature/sensor");
    nmea->connect_exhaust_temperature(engine_exhaust_temperature);
    engine_exhaust_temperature->connect_to(new SKOutputFloat(
        "propulsion.main.exhaustTemperature",
        "/data/engine_exhaust_temperature/sk_path",
        "K"));

    // Engine coolant temperature
    auto engine_coolant_temperature_resistance = new ResistanceSensor(ads1115, 0, 500, "/data/engine_coolant_temperature/sensor");
    auto engine_coolant_temperature = engine_coolant_temperature_resistance->connect_to(new CoolantTempSender("/data/engine_coolant_temperature/interpolator"));
    nmea->connect_coolant_temperature(engine_coolant_temperature);
    engine_coolant_temperature->connect_to(new SKOutputFloat(
        "propulsion.main.coolantTemperature",
        "/data/engine_coolant_temperature/sk_path",
        "K"));

    // Treat coolant temperature as the actual engine temperature
    engine_coolant_temperature->connect_to(new SKOutputFloat(
        "propulsion.main.temperature",
        "/data/engine_temperature/sk_path",
        "K"));

    // Engine oil pressure
    auto engine_oil_pressure_resistance = new ResistanceSensor(ads1115, 1, 500, "/data/engine_oil_pressure/sensor");
    auto engine_oil_pressure = engine_oil_pressure_resistance->connect_to(new OilPressureSender("/data/engine_oil_pressure/interpolator"));
    nmea->connect_oil_pressure(engine_oil_pressure);
    engine_oil_pressure->connect_to(new SKOutputFloat(
        "propulsion.main.oilPressure",
        "/data/engine_oil_pressure/sk_path",
        "Pa"));

    // Fresh water tank level
    auto fresh_water_tank_level_resistence = new ResistanceSensor(ads1115, 3, 500, "/data/fresh_water_tank_level/sensor");
    auto fresh_water_tank_level = fresh_water_tank_level_resistence->connect_to(new TankLevelSender("/data/fresh_water_tank_level/interpolator"));
    auto fresh_water_tank_capacity_linear = new Linear(300. / 1000, 0, "/data/fresh_water_tank_volume/capacity");
    auto fresh_water_tank_volume = fresh_water_tank_level->connect_to(fresh_water_tank_capacity_linear);
    fresh_water_tank_level->connect_to(new SKOutputFloat(
        "tanks.freshWater.main.currentLevel",
        "/data/fresh_water_tank_level/sk_path",
        "ratio"));
    fresh_water_tank_volume->connect_to(new SKOutputFloat(
        "tanks.freshWater.main.currentVolume",
        "/data/fresh_water_tank_volume/sk_path",
        "m3"));
    nmea->connect_water_level(fresh_water_tank_level);

    // Send fresh water tank info
    auto fresh_water_tank_capacity = new TankCapacity(fresh_water_tank_capacity_linear);
    fresh_water_tank_capacity->connect_to(new SKOutputFloat(
        "tanks.freshWater.main.capacity",
        "/data/fresh_water_tank_capacity/sk_path",
        "m3"));
    nmea->connect_water_capacity(fresh_water_tank_capacity);

#ifndef SERIAL_DEBUG_DISABLED
    // debugValueProducer(engine_room_temperature, "Engine room temp: %f K");
    // debugValueProducer(engine_exhaust_temperature, "Engine exhaust temp: %f K");
    // debugValueProducer(engine_alternator_temperature, "Alternator temp: %f K");
    debugValueProducer(engine_rpms_raw, "Engine RPMs (raw): %f");
    debugValueProducer(engine_rpms, "Engine RPMs: %f");
    debugValueProducer(engine_runtime, "Engine runtime: %f seconds");
    // debugValueProducer(engine_coolant_temperature_resistance, "Engine coolant sensor resistance: %f Ohms");
    // debugValueProducer(engine_coolant_temperature, "Engine coolant temperature: %f K");
    // debugValueProducer(engine_oil_pressure_resistance, "Engine oil pressure resistance: %f Ohms");
    // debugValueProducer(engine_oil_pressure, "Engine oil pressure: %f Pa");
    // debugValueProducer(fresh_water_tank_level, "Fresh water tank level: %f %");
    // debugValueProducer(fresh_water_tank_volume, "Fresh water tank volume: %f m3");
    debugValueProducer(fresh_water_tank_capacity, "Fresh water tank capacity: %f m3");
#endif

    sensesp_app->start();
}

// main program loop
void loop() { app.tick(); }
