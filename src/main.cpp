#include <Arduino.h>
#include <MRCPLock.h>
#include <MRCPMeter.h>

MRCPNode* node;

void setup() {
    //node = new MRCPLock();
    node = new MRCPMeter();
    node->setup();
}

void loop() {
    node->loop();
}