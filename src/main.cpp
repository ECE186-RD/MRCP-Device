#include <Arduino.h>
#include <MRCPLock.h>
#include <MRCPMeter.h>
#include <MRCPLight.h>

MRCPNode* node;

void setup() {
    //node = new MRCPLock();
    //node = new MRCPMeter();
    node = new MRCPLight();
    node->setup();
}

void loop() {
    node->loop();
}