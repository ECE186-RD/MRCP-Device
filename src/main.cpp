#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <MRCPLock.h>

MRCPLock* node;

void setup() {
    node = new MRCPLock();
    node->setup();
}

void loop() {
    node->loop();
}
