// SET A — Answer Unit
// BLE Server | PN532 I2C | Green LED pin 18 | Red LED pin 5

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <map>
#include <set>

#define SDA_PIN         21
#define SCL_PIN         22
#define LED_GREEN       18
#define LED_RED         5

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

const std::map<String, String> qa_map = {
  {"E04D975F", "2CEE6C05"},
};

const std::set<String> question_uids = {
  "E04D975F",
};

String lastReceivedUID = "";
String lastScannedUID  = "";

BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

void evaluateMatch();

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s)    { deviceConnected = true; }
  void onDisconnect(BLEServer* s) { deviceConnected = false; BLEDevice::startAdvertising(); }
};

class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) {
    lastReceivedUID = c->getValue().c_str();
    lastReceivedUID.toUpperCase();
    Serial.printf("[BLE RX] Set B scanned: %s\n", lastReceivedUID.c_str());

    // Immediate red if Set B scanned a question tag
    if (question_uids.count(lastReceivedUID)) {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      Serial.println("[WARN] Question tag scanned on Set B → RED on Set A");
      lastReceivedUID = "";
      return;
    }

    evaluateMatch();
  }
};

String uidToString(uint8_t* uid, uint8_t len) {
  String s = "";
  for (int i = 0; i < len; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

void evaluateMatch() {
  if (lastReceivedUID == "" || lastScannedUID == "") return;

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED,   LOW);

  auto it = qa_map.find(lastReceivedUID);
  if (it != qa_map.end() && it->second == lastScannedUID) {
    Serial.println("[EVAL] MATCH → GREEN");
    digitalWrite(LED_GREEN, HIGH);
  } else {
    Serial.println("[EVAL] NO MATCH");
  }

  lastReceivedUID = "";
  lastScannedUID  = "";
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED,   LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) { Serial.println("PN532 not found!"); while(1); }
  nfc.SAMConfig();
  Serial.println("[NFC] PN532 ready (Set A)");

  BLEDevice::init("NFC_SET_A");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new CharCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Advertising as NFC_SET_A");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLen;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100)) {
    lastScannedUID = uidToString(uid, uidLen);
    Serial.printf("[NFC] Set A scanned: %s\n", lastScannedUID.c_str());

    // Immediate red if question tag scanned directly on Set A
    if (question_uids.count(lastScannedUID)) {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      Serial.println("[WARN] Question tag on Set A → RED immediately");
      lastScannedUID = "";
      return;
    }

    evaluateMatch();
  }
}
