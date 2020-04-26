#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPMeter : public MRCPNode{
    public:
      bool deviceConnected = false;
      static const int LED = 2;
      float rate = 2;
      bool timeStarted = false;
      unsigned long previousTime;

  class MRCPMeterServerCallbacks: public BLEServerCallbacks {
    public:
      MRCPMeter* node;

      MRCPMeterServerCallbacks(MRCPMeter* node){
        this->node = node;
      }

      void onConnect(BLEServer* pServer) {
       node->deviceConnected = true;
       Serial.println("Connected to Client");
      };

      void onDisconnect(BLEServer* pServer) {
        node->deviceConnected = false;
      }
  };

  class MRCPMeterCharacteristicCallbacks: public BLECharacteristicCallbacks {
      public:

      MRCPMeter* node;

      MRCPMeterCharacteristicCallbacks(MRCPMeter* node){
        this->node = node;
      }

      void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
          Serial.print("*** Received Value: ");

          for (int i = 0; i < rxValue.length(); i++) {
            Serial.print(rxValue[i]);
          }
          Serial.println();
          if (rxValue.find("REQ_START") != -1) {
            node->sendString("ACK_START");
            node->previousTime = millis();
            node->timeStarted = true;
          }else if(rxValue.find("REQ_RATE") != -1){
            node->sendFloat(node->rate);
          }else if(rxValue.find("ACK_RATE") != -1){
            
          }else if(rxValue.find("ACK_STOP") != -1){
            
          }
      }
    };
  };
    
      MRCPMeter(){
        this->name = "MRCP Meter";
        this->server_callbacks = new MRCPMeterServerCallbacks(this);
        this->characteristic_callbacks = new MRCPMeterCharacteristicCallbacks(this);
      }

      void setup(){
        MRCPNode::setup();
      }

      void loop(){
        MRCPNode::loop();
        if(this->timeStarted){
          unsigned long currentTime = millis();
          if(currentTime - this->previousTime >= 20000){
            this->sendString("REQ_STOP");
            this->timeStarted = false;
          }
        }
        
      }
};