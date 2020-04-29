#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPLock : public MRCPNode{
    public:
      static const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
      static const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

      static const int mPin1 = 4;  // GPIO 4, input into h-bridge
      static const int mPin2 = 17; // GPIO 17, input into h-bridge
      static const int enPin = 18; // GPIO 18, enables L293D
      static const int swPin = 19; // GPIO 21, Monitors the state the lock is in
      static const int testoutPin = 16;  // Used for testing

      int txValue = 0;
      int txValue_prev = 0;
  
    MRCPLock(){
      name = "MRCP Lock";
    }

    void onClientAuthenticated(){
      MRCPNode::onClientAuthenticated();
      ui_info_doc["led_state"] = "on";
    }

    void onBLEWrite() {
      MRCPNode::onBLEWrite();
      if (rx_doc.size() > 0 && client_authenticated) {
        if(rx_doc["type"] == "LED_CMD"){
          if (rx_doc["led_cmd"] == "on") {
            Serial.println("Turning ON!");
            digitalWrite(LED, HIGH);
            txValue=1;

          } else if (rx_doc["led_cmd"] == "off") {
            Serial.println("Turning OFF!");
            digitalWrite(LED, LOW);
            txValue=0;
          }

        // currentState is determined by swPin, which is affected by the gears.
        // Need to figure out which state it is in at the time of connection
        // to determine if the user is going to unlock/lock the door.
        bool currentState = digitalRead(swPin);
        bool nextState = ! currentState;

        if (digitalRead(swPin) == HIGH){
          digitalWrite(mPin2, LOW);
          digitalWrite(mPin1, HIGH);
        }
        else {
          digitalWrite(mPin1, LOW);
          digitalWrite(mPin2, HIGH);
        }

        digitalWrite(enPin, HIGH); // Activates the L493D

        // Turns on motor until switch state changes
         while(currentState != nextState){
          delay(50);
          if (digitalRead(swPin) == HIGH){
            currentState = true;
          }
          else{
            currentState = false;
          }
          }
          delay(500);
        digitalWrite(enPin, LOW); // Turns off L493D
        txValue = !(txValue); // Added this here

        // Added this
        if (deviceConnected) {
          tx_doc["type"] = "LED_STATE";
          tx_doc["led_state"] = txValue ? "on" : "off";
          sendDoc(tx_doc);
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
    }

    void loop(){
      MRCPNode::loop();
      if (client_authenticated && txValue_prev != txValue) {
        tx_doc["type"] = "LED_STATE";
        tx_doc["led_state"] = txValue ? "on" : "off";
        sendDoc(tx_doc);
        txValue_prev = txValue;
      }
    }
};