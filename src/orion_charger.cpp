#include "BLEDevice.h"
#include "HardwareSerial.h"

// BLE Server name to connect to
#define bleServerName "ORIONSOMETHINGSOMETHING"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID orionServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");

static BLEUUID chargerModeCharacteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress orionAddress((uint8_t*)"\0\0\0\0\0\0");

BLEClient* pClient = BLEDevice::createClient();

void setupBLE() {
    // Init BLE device
    BLEDevice::init("SH-ESP32 Engine monitoring");
}

void enableCharger() {
    pClient->connect(orionAddress);
    pClient->setValue(orionServiceUUID, chargerModeCharacteristicUUID, "on");
    pClient->disconnect();
}

// - Subscribe to RPM
// - If RPMs are < 1200 for more than 30s, turn off chargers
// - If RPMs are >= 1200 for more than 30s, turn on chargers
// - Soft start. Let alternator and belt to warm up before turning on the chargers
// - Subscribe to alternator temperature
// - If alt. temperature > defined high threshold, turn off charger
// - If alt. temperature < defined low threshold, turn on charger
// - Publish charger state to SignalK path
// - Only connect to BT when engine is on (RPMs > defined engine on threshold)
