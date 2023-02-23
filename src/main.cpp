#include <Adafruit_ADS1X15.h>

#include "configuration.h"
#include "fuel_tank_sensor.h"
#include "nmea.h"
#include "resistance_sensor.h"
#include "run_time_sensor.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/transforms/frequency.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/transforms/moving_average.h"
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

#ifndef SERIAL_DEBUG_DISABLED
#define debugValueProducer(p, fmt)                             \
    p->connect_to(new LambdaConsumer<float>([&](float value) { \
        debugD(fmt, value);                                    \
    }));
#else
#define debugValueProducer(p, fmt)
#endif

// Every minute reads the configured multiplier of a Linear transform,
// representing the total capacity of a tank, and emits it. This is
// used to broadcast the total capacity of a tank, which can be updated
// through the Linear transform configuration.
class TankCapacity : public ValueProducer<float>,
                     public Startable {
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

void setupDieselTank(Nmea *nmea) {
    // Tank level
    auto fuel_tank_level = (new FuelTankSensor(FUEL_TANK_EMPTY_MM, FUEL_TANK_FULL_MM, 2000, "/data/fuel_tank_level/sensor"))
                               ->connect_to(new MovingAverage(10, 1.0, "/data/fuel_tank_level/samples"));
    fuel_tank_level->connect_to(new SKOutputFloat("tanks.fuel.main.currentLevel",
                                                  "/data/fuel_tank_level/sk_path",
                                                  "ratio"));
    nmea->connect_fuel_level(fuel_tank_level);

    // Tank volume
    auto fuel_tank_capacity_linear = new Linear(FUEL_TANK_CAPACITY, 0, "/data/fresh_water_tank_volume/capacity_m3");
    fuel_tank_level
        ->connect_to(fuel_tank_capacity_linear)
        ->connect_to(new SKOutputFloat("tanks.fuel.main.currentVolume",
                                       "/data/fuel_tank_level/sk_path",
                                       "m3"));

    // Tank capacity
    auto fuel_tank_capacity = new TankCapacity(fuel_tank_capacity_linear);
    fuel_tank_capacity->connect_to(new SKOutputFloat(
        "tanks.fuel.main.capacity",
        "/data/fuel_tank_capacity/sk_path",
        "m3"));
    nmea->connect_fuel_capacity(fuel_tank_capacity);

    debugValueProducer(fuel_tank_level, "Diesel tank level: %f m3");
}

void setup1WireTempSensors(Nmea *nmea) {
    DallasTemperatureSensors *dts = new DallasTemperatureSensors(ONEWIRE_PIN);

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

    // Engine exhaust temperature
    auto engine_exhaust_temperature = new OneWireTemperature(dts, 1000, "/data/engine_exhaust_temperature/sensor");
    nmea->connect_exhaust_temperature(engine_exhaust_temperature);
    engine_exhaust_temperature->connect_to(new SKOutputFloat(
        "propulsion.main.exhaustTemperature",
        "/data/engine_exhaust_temperature/sk_path",
        "K"));

    debugValueProducer(engine_room_temperature, "Engine room temp: %f K");
    debugValueProducer(engine_alternator_temperature, "Alternator temp: %f K");
    debugValueProducer(engine_exhaust_temperature, "Engine exhaust temp: %f K");
}

void setupWaterTank(Nmea *nmea, Adafruit_ADS1115 *ads1115) {
    // Tank level
    auto fresh_water_tank_level = (new ResistanceSensor(ads1115, FRESH_WATER_TANK_SENSOR_CHANNEL, 500, "/data/fresh_water_tank_level/sensor"))
                                      ->connect_to(new MovingAverage(10, 1.0, "/data/fresh_water_tank_level/samples"))
                                      ->connect_to(new TankLevelSender("/data/fresh_water_tank_level/interpolator"));
    fresh_water_tank_level->connect_to(new SKOutputFloat(
        "tanks.freshWater.main.currentLevel",
        "/data/fresh_water_tank_level/sk_path",
        "ratio"));
    nmea->connect_water_level(fresh_water_tank_level);

    // Tank volume
    auto fresh_water_tank_capacity_linear = new Linear(FRESH_WATER_TANK_CAPACITY, 0, "/data/fresh_water_tank_volume/capacity_m3");
    fresh_water_tank_level
        ->connect_to(fresh_water_tank_capacity_linear)
        ->connect_to(new SKOutputFloat(
            "tanks.freshWater.main.currentVolume",
            "/data/fresh_water_tank_volume/sk_path",
            "m3"));

    // Tank capacity
    auto fresh_water_tank_capacity = new TankCapacity(fresh_water_tank_capacity_linear);
    fresh_water_tank_capacity->connect_to(new SKOutputFloat(
        "tanks.freshWater.main.capacity",
        "/data/fresh_water_tank_capacity/sk_path",
        "m3"));
    nmea->connect_water_capacity(fresh_water_tank_capacity);

    debugValueProducer(fresh_water_tank_level, "Fresh water tank level: %f %");
    debugValueProducer(fresh_water_tank_capacity, "Fresh water tank capacity: %f m3");
}

void setupEngineRpmsAndRuntime(Nmea *nmea) {
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

    debugValueProducer(engine_rpms_raw, "Engine RPMs (raw): %f");
    debugValueProducer(engine_rpms, "Engine RPMs: %f");
    debugValueProducer(engine_runtime, "Engine runtime: %f seconds");
}

void setupEngineCoolantTemperature(Nmea *nmea, Adafruit_ADS1115 *ads1115) {
    auto engine_coolant_temperature_resistance = new ResistanceSensor(ads1115, ENGINE_COOLANT_TEMP_SENSOR_CHANNEL, 500, "/data/engine_coolant_temperature/sensor");
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

    debugValueProducer(engine_coolant_temperature_resistance, "Engine coolant sensor resistance: %f Ohms");
    debugValueProducer(engine_coolant_temperature, "Engine coolant temperature: %f K");
}

void setupEngineOilTemperature(Nmea *nmea, Adafruit_ADS1115 *ads1115) {
    auto engine_oil_pressure_resistance = new ResistanceSensor(ads1115, ENGINE_OIL_PRESSURE_SENSOR_CHANNEL, 500, "/data/engine_oil_pressure/sensor");
    auto engine_oil_pressure = engine_oil_pressure_resistance->connect_to(new OilPressureSender("/data/engine_oil_pressure/interpolator"));
    nmea->connect_oil_pressure(engine_oil_pressure);
    engine_oil_pressure->connect_to(new SKOutputFloat(
        "propulsion.main.oilPressure",
        "/data/engine_oil_pressure/sk_path",
        "Pa"));

    debugValueProducer(engine_oil_pressure_resistance, "Engine oil pressure resistance: %f Ohms");
    debugValueProducer(engine_oil_pressure, "Engine oil pressure: %f Pa");
}

void setupAlternatorOutput(Nmea *nmea, Adafruit_ADS1115 *ads1115) {
    auto alternator_output_voltage = new VoltageSensor(ads1115, ALTERNATOR_OUTPUT_SENSOR_CHANNEL, 500, "/data/alternator_output/sensor");
    // Alt. I = (V / R) * transformer multiplier
    auto alternator_output = alternator_output_voltage->connect_to(new Linear(PZCT02_MULTIPLIER * (1 / PZCT02_BURDEN_RESISTANCE), 0, "/data/alternator_output/linear"));
    alternator_output->connect_to(new SKOutputFloat(
        "electrical.alternators.engine.current",
        "/data/alternator_output/sensor",
        "A"));

    debugValueProducer(alternator_output_voltage, "Alternator current sensor voltage: %f V");
    debugValueProducer(alternator_output, "Alternator output current: %f A");
}

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
    ads1115->setGain(GAIN_ONE);  // 1x gain +/- 4.096V 1 bit = 0.125mV
    bool ads_initialized = ads1115->begin(ADS1115ADDR, i2c);
    debugD("ADS1115 initialized: %d", ads_initialized);

    SensESPAppBuilder builder;

    sensesp_app = builder.set_hostname("EngineMonitoring")
                      ->enable_ip_address_sensor()
                      ->enable_system_info_sensors()
                      ->enable_uptime_sensor()
                      ->enable_wifi_signal_sensor()
                      ->get_app();

    // Set up sensors
    setup1WireTempSensors(nmea);
    setupEngineRpmsAndRuntime(nmea);
    setupEngineCoolantTemperature(nmea, ads1115);
    setupEngineOilTemperature(nmea, ads1115);
    setupWaterTank(nmea, ads1115);
    setupAlternatorOutput(nmea, ads1115);
    setupDieselTank(nmea);

    sensesp_app->start();
}

// main program loop
void loop() { app.tick(); }
