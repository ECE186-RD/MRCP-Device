#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPLight : public MRCPNode{
    public:
      static const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
  
    MRCPLight(){
      name = "MRCP Light";
    }

    void setup(){
      MRCPNode::setup();
      pinMode(LED, OUTPUT);
    }

    void onClientAuthenticated(){
      MRCPNode::onClientAuthenticated();
      ui_info_doc["led_state"] = "on";
    }

    void onBLEWrite() {
      MRCPNode::onBLEWrite();
    }

    void loop(){
      MRCPNode::loop();
      if(millis() % 1000 > 500){
        digitalWrite(LED, HIGH);
      }else{
        digitalWrite(LED, LOW);
      }
    }
};