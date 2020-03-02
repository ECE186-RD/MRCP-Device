#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPLockServerCallbacks: public BLEServerCallbacks {
    public:
      bool* deviceConnected;

    MRCPLockServerCallbacks(bool* deviceConnected){
      this->deviceConnected = deviceConnected;
    };

    void onConnect(BLEServer* pServer) {
      *deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      *deviceConnected = false;
    }
};

class MRCPLockCharacteristicCallbacks: public BLECharacteristicCallbacks {
    public:
      bool deviceConnected;
      float* txValue;

    MRCPLockCharacteristicCallbacks(bool deviceConnected, float* txValue){
      this->deviceConnected = deviceConnected;
      this->txValue = txValue;
    };

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        } Serial.println(); // Do stuff based on the command received from the app
        if (rxValue.find("ON") != -1) {
          Serial.println("Turning ON!");
          digitalWrite(MRCPLock::LED, HIGH);
          *txValue=1;

        } else if (rxValue.find("OFF") != -1) {
          Serial.println("Turning OFF!");
          digitalWrite(MRCPLock::LED, LOW);
          *txValue=0;
        }

        // currentState is determined by swPin, which is affected by the gears.
        // Need to figure out which state it is in at the time of connection
        // to determine if the user is going to unlock/lock the door.
        bool currentState = digitalRead(MRCPLock::swPin);
        bool nextState = ! currentState;

        if (digitalRead(MRCPLock::swPin) == HIGH){
          digitalWrite(MRCPLock::mPin2, LOW);
          digitalWrite(MRCPLock::mPin1, HIGH);
        }
        else {
          digitalWrite(MRCPLock::mPin1, LOW);
          digitalWrite(MRCPLock::mPin2, HIGH);
        }

        digitalWrite(MRCPLock::enPin, HIGH); // Activates the L493D

        // Turns on motor until switch state changes
         while(currentState != nextState){
          delay(50);
          if (digitalRead(MRCPLock::swPin) == HIGH){
            currentState = true;
          }
          else{
            currentState = false;
          }
          }
          delay(500);
        digitalWrite(MRCPLock::enPin, LOW); // Turns off L493D
        *txValue = !(*txValue); // Added this here

        // Added this
        if (deviceConnected) {
          // Let's convert the value to a char array:
          char txString[8]; // make sure this is big enuffz
          dtostrf(*txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

          pCharacteristic->setValue(txString);

          pCharacteristic->notify(); // Send the value to the app!
          Serial.print("*** Sent Value: ");
          Serial.println(txString);
          Serial.print(" ***");
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

class MRCPLock : public MRCPNode{
    private:
      bool* deviceConnected = new bool(false);
      float* txValue = 0;

    public:
      static const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
      static const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

      static const int mPin1 = 4;  // GPIO 4, input into h-bridge
      static const int mPin2 = 17; // GPIO 17, input into h-bridge
      static const int enPin = 18; // GPIO 18, enables L293D
      static const int swPin = 19; // GPIO 21, Monitors the state the lock is in
      static const int testoutPin = 16;  // Used for testing
    
      MRCPLock(){
        this->name = "MRCP Lock";
        this->server_callbacks = new MRCPLockServerCallbacks(deviceConnected);
        this->characteristic_callbacks = new MRCPLockCharacteristicCallbacks(*deviceConnected, txValue);
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
          dtostrf(*txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

          pCharacteristic->setValue(txString);

          pCharacteristic->notify(); // Send the value to the app!
          Serial.print("*** Sent Value: ");
          Serial.println(txString);
          Serial.print(" ***");
        }
        delay(100);
      }
};