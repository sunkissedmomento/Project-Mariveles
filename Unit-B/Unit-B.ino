// SET B — Question Unit
// BLE Client | PN532 I2C | Onboard LED pin 2 (blink=connecting, solid=connected)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

#define SDA_PIN         21
#define SCL_PIN         22
#define LED_STATUS      2

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

BLEClient*               pClient  = nullptr;
BLERemoteService*        pService = nullptr;
BLERemoteCharacteristic* pChar    = nullptr;
bool connected = false;

String uidToString(uint8_t* uid, uint8_t len) {
  String s = "";
  for (int i = 0; i < len; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

bool connectToServer() {
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  BLEScanResults* results = scan->start(5, false);

  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);
    if (dev.getName() == "NFC_SET_A") {
      pClient = BLEDevice::createClient();
      if (!pClient->connect(&dev)) return false;

      pService = pClient->getService(SERVICE_UUID);
      if (!pService) { pClient->disconnect(); return false; }

      pChar = pService->getCharacteristic(CHARACTERISTIC_UUID);
      if (!pChar || !pChar->canWrite()) { pClient->disconnect(); return false; }

      Serial.println("[BLE] Connected to NFC_SET_A");
      return true;
    }
  }
  return false;
}

void blinkUntilConnected() {
  unsigned long lastBlink = 0;
  bool ledState = false;

  while (!connected) {
    if (millis() - lastBlink >= 500) {
      ledState = !ledState;
      digitalWrite(LED_STATUS, ledState);
      lastBlink = millis();
    }
    connected = connectToServer();
    if (!connected) Serial.println("[BLE] Retry...");
  }

  digitalWrite(LED_STATUS, HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) { Serial.println("PN532 not found!"); while(1); }
  nfc.SAMConfig();
  Serial.println("[NFC] PN532 ready (Set B)");

  BLEDevice::init("NFC_SET_B");
  Serial.println("[BLE] Scanning for Set A...");
  blinkUntilConnected();
}

void loop() {
  if (!connected || !pClient->isConnected()) {
    Serial.println("[BLE] Reconnecting...");
    digitalWrite(LED_STATUS, LOW);
    connected = false;
    blinkUntilConnected();
    return;
  }

  uint8_t uid[7];
  uint8_t uidLen;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100)) {
    String uidStr = uidToString(uid, uidLen);
    Serial.printf("[NFC] Set B scanned: %s → sending to Set A\n", uidStr.c_str());
    pChar->writeValue(uidStr.c_str(), uidStr.length());
    delay(500);
  }
}
