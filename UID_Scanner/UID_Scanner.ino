#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found. Check wiring.");
    while (1);
  }

  Serial.print("Found PN532 - Firmware version: ");
  Serial.print((versiondata >> 16) & 0xFF);
  Serial.print(".");
  Serial.println((versiondata >> 8) & 0xFF);

  nfc.SAMConfig();
  Serial.println("Waiting for NFC card...");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.print("Card detected! UID: ");
    for (int i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) Serial.print("0");
      Serial.print(uid[i], HEX);
      if (i < uidLength - 1) Serial.print(":");
    }
    Serial.println();
    Serial.print("UID Length: ");
    Serial.print(uidLength);
    Serial.println(" bytes");
    delay(1000); // debounce
  }
}