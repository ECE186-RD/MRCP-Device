#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPLock : public MRCPNode{
    public:
      bool deviceConnected = false;
      float txValue = 0;

      static const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
      static const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

      static const int mPin1 = 4;  // GPIO 4, input into h-bridge
      static const int mPin2 = 17; // GPIO 17, input into h-bridge
      static const int enPin = 18; // GPIO 18, enables L293D
      static const int swPin = 19; // GPIO 21, Monitors the state the lock is in
      static const int testoutPin = 16;  // Used for testing

  class MRCPLockServerCallbacks: public BLEServerCallbacks {
    public:
      MRCPLock* node;

      MRCPLockServerCallbacks(MRCPLock* node){
        this->node = node;
      }

      void onConnect(BLEServer* pServer) {
       node->deviceConnected = true;
      };

      void onDisconnect(BLEServer* pServer) {
        node->deviceConnected = false;
      }
  };

class MRCPLockCharacteristicCallbacks: public BLECharacteristicCallbacks {
    public:

     MRCPLock* node;

    MRCPLockCharacteristicCallbacks(MRCPLock* node){
      this->node = node;
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        if (rxValue.find("ON") != -1) {
          Serial.println("Turning ON!");
          digitalWrite(node->LED, HIGH);
          node->txValue=1;

        } else if (rxValue.find("OFF") != -1) {
          Serial.println("Turning OFF!");
          digitalWrite(node->LED, LOW);
          node->txValue=0;
        }

        // currentState is determined by swPin, which is affected by the gears.
        // Need to figure out which state it is in at the time of connection
        // to determine if the user is going to unlock/lock the door.
        bool currentState = digitalRead(node->swPin);
        bool nextState = ! currentState;

        if (digitalRead(node->swPin) == HIGH){
          digitalWrite(node->mPin2, LOW);
          digitalWrite(node->mPin1, HIGH);
        }
        else {
          digitalWrite(node->mPin1, LOW);
          digitalWrite(node->mPin2, HIGH);
        }

        digitalWrite(node->enPin, HIGH); // Activates the L493D

        // Turns on motor until switch state changes
         while(currentState != nextState){
          delay(50);
          if (digitalRead(node->swPin) == HIGH){
            currentState = true;
          }
          else{
            currentState = false;
          }
          }
          delay(500);
        digitalWrite(node->enPin, LOW); // Turns off L493D
        node->txValue = !(node->txValue); // Added this here

        // Added this
        if (node->deviceConnected) {
          node->sendFloat(node->txValue);
        }
      // USED FOR TESTING
      /*
        if (digitalRead(swPin)){
          digitalWrite(testoutPin, LOW);
        }
        else {
        digitalWrite(testoutPin, HIGH);
      }
      */

        Serial.println();
        Serial.println("*********");
    }
  }
};
    
      MRCPLock(){
        this->name = "MRCP Lock";
        this->server_callbacks = new MRCPLockServerCallbacks(this);
        this->characteristic_callbacks = new MRCPLockCharacteristicCallbacks(this);
      }

      void setup(){
        MRCPNode::setup();
        pinMode(LED, OUTPUT); // Create the BLE Device
        pinMode (mPin1, OUTPUT);
        pinMode (mPin2, OUTPUT);
        pinMode (enPin, OUTPUT);
        pinMode (swPin, INPUT);
        //pinMode (testPin, INPUT);
        pinMode (testoutPin, OUTPUT);
      }

      void loop(){
        MRCPNode::loop();
        if (deviceConnected) {
          // Let's convert the value to a char array:
          char txString[8]; // make sure this is big enuffz
          dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

          pCharacteristic->setValue(txString);

          pCharacteristic->notify(); // Send the value to the app!
          Serial.print("*** Sent Value: ");
          Serial.println(txString);
          Serial.print(" ***");
        }
        delay(100);
      }
};