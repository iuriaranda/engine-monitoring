#include "orion_chargers.h"

#include "BLEDevice.h"
#include "sensesp/system/local_debug.h"

/* UUID's of the service, characteristic that we want to read */
// BLE Service
static BLEUUID chargerModeServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
static BLEUUID chargerModeCharacteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class OrionAdvertisedDeviceCallback : public BLEAdvertisedDeviceCallbacks {
    /**
     * Called for each advertising BLE server.
     */

    void onResult(BLEAdvertisedDevice advertisedDevice) {
        debugD("BLE Advertised Device found: %s", advertisedDevice.toString().c_str());

        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(chargerModeServiceUUID)) {
            debugI("BLE Advertised Device is an Orion");
            // Found an Orion, save it in config
            // TODO
            BLEClient* pClient = BLEDevice::createClient();
            debugI(" - Created client");

            advertisedDevice.pClient->setClientCallbacks(new MyClientCallback());

            // Connect to the remove BLE Server.
            pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
            debugI(" - Connected to server");
        }
    }
};

class BLESecurity : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
        debugI("PassKeyRequest");
        return 123456;
    }
    void onPassKeyNotify(uint32_t pass_key) {
        debugI("The passkey Notify number: %d", pass_key);
    }
    bool onConfirmPIN(uint32_t pass_key) {
        debugI("The passkey YES/NO number: %d", pass_key);
        vTaskDelay(5000);
        return true;
    }
    bool onSecurityRequest() {
        debugI("SecurityRequest");
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
        debugI("Starting BLE work!");
    }
};

namespace sensesp {

OrionChargers::OrionChargers(String config_path = "") : Configurable(config_path) {
    load_configuration();
}

void OrionChargers::start() {
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new OrionAdvertisedDeviceCallback());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30);

    BLEDevice::setSecurityCallbacks(new BLESecurity());
}

void OrionChargers::get_configuration(JsonObject& doc) {
}

bool OrionChargers::set_configuration(const JsonObject& config) {
}

static const char SCHEMA[] PROGMEM = R"###({
    "type": "object",
    "properties": {
        "chargers_on_rpm_threshold": { "title": "RPM threshold to enable charging", "type": "number", "description": "Orion chargers will be enabled when engine RPMs are higher than this threshold for more than 30 seconds" },
        "chargers_on_alternator_temp_threshold": { "title": "Alternator temperature threshold to enable charging", "type": "number", "description": "Orion chargers will be enabled when the alternator temperature is lower than this threshold for more than 30 seconds" },
        "chargers_off_rpm_threshold": { "title": "RPM threshold to disable charging", "type": "number", "description": "Orion chargers will be disabled when engine RPMs are lower than this threshold for more than 30 seconds" },
        "chargers_off_alternator_temp_threshold": { "title": "Full tank mm value", "type": "number", "description": "Orion chargers will be disabled when the alternator temperature is higher than this threshold for more than 30 seconds" },
        "orion_charger_service_uuid": { "title": "Orion charger BLE service UUID", "type": "string", "description": "Bluetooth LE service UUID for the Orion charger" },
        "orion_charger_characteristic_uuid": { "title": "Orion charger BLE characteristic UUID", "type": "string", "description": "Bluetooth LE characteristic UUID for the orion charger" },
        "bluetooth_discovery_enabled": { "title": "Enable Bluetooth discovery", "type": "boolean", "default": true, "description": "Discovered Orion devices will show up in 'Discovered Orion devices'" },
        "discovered_orions": {
            "title": "Discovered Orion devices",
            "type": "array",
            "format": "table",
            "items": {
                "type": "object",
                "properties": {
                    "name": { "type": "string", "title": "Name of the Bluetooth device", "readonly": true },
                    "address": { "type": "string", "title": "Address of the Bluetooth device", "readonly": true },
                    "pin": { "type": "number", "title": "Bluetooth Pin to connect to the Orion charger" },
                    "manage": { "type": "boolean", "title": "Enable management of this Orion charger" }
                }
            }
        }
    }
  })###";

String OrionChargers::get_config_schema() { return FPSTR(SCHEMA); }

}  // namespace sensesp

// Configurable items:
// - Chargers bluetooth addresses
// - NPM threshold
// - Temp. min and max threshold
// - Orion service and characteristics UUIDs
