#include <Adafruit_PN532.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define PN532_SS 5
Adafruit_PN532 nfc(PN532_SS);

BLECharacteristic *pCharacteristic;
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"

void setup() {
  BLEDevice::init("QuestionScanner");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->start();

  nfc.begin();
  nfc.SAMConfig();
}

void loop() {
  uint8_t uid[7]; uint8_t uidLen;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) {
    String uidStr = "";
    for (int i = 0; i < uidLen; i++) uidStr += String(uid[i], HEX);
    pCharacteristic->setValue(uidStr.c_str());
    pCharacteristic->notify();
    delay(1000); // debounce
  }
}