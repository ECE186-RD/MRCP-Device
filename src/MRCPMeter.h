#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPMeter : public MRCPNode{
  public:
    float rate = 2;
    bool timeStarted = false;
    unsigned long previousTime;

    MRCPMeter(){
      name = "MRCP Meter";
      ui_info_doc["rate"] = rate;
    }

    void setup(){
      MRCPNode::setup();
    }

    void onBLEConnect(){
      MRCPNode::onBLEConnect();
    }

    void onBLEDisconnect(){
      MRCPNode::onBLEDisconnect();
    }

    void onBLEWrite() {
      MRCPNode::onBLEWrite();
      if (rx_doc.size() > 0) {
        if (client_authenticated) {
          if(rx_doc["type"] == "START"){
            tx_doc["type"] = "START_ACK";
            sendDoc(tx_doc);
            previousTime = millis();
            timeStarted = true;
          }
        }
      }
    }

    void onClientAuthenticated() {
      MRCPNode::onClientAuthenticated();
    }

    void loop() {
      MRCPNode::loop();
      if(timeStarted){
        unsigned long currentTime = millis();
        unsigned long diff = currentTime - previousTime;
        if(diff >= 20000){
          tx_doc["type"] = "STOP";
          tx_doc["duration"] = diff;
          sendDoc(tx_doc);
          timeStarted = false;
        }
      }
    }
};