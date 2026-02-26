#include <Wire.h>
#include <Adafruit_PN532.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; Serial.println("Unit A connected."); }
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; Serial.println("Unit A disconnected."); BLEDevice::getAdvertising()->start(); }
};

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) { Serial.println("PN532 not found."); while (1); }
  nfc.SAMConfig();
  Serial.println("PN532 ready.");

  BLEDevice::init("QuestionScanner");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->start();
  Serial.println("BLE advertising. Waiting for Unit A...");
}

void loop() {
  if (!deviceConnected) return;

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String uidStr = "";
    for (int i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) uidStr += "0";
      uidStr += String(uid[i], HEX);
    }
    uidStr.toLowerCase();
    Serial.print("Question scanned: ");
    Serial.println(uidStr);
    pCharacteristic->setValue(uidStr.c_str());
    pCharacteristic->notify();
    delay(1500); // debounce
  }
}