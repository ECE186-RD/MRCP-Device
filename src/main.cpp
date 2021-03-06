#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
//#include <BLENode.h>

//Following code from https://iotbyhvm.ooo/esp32-ble-tutorials/
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;

const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

const int mPin1 = 4;  // GPIO 4, input into h-bridge
const int mPin2 = 17; // GPIO 17, input into h-bridge
const int enPin = 18; // GPIO 18, enables L293D
const int swPin = 19; // GPIO 21, Monitors the state the lock is in
const int testoutPin = 16;  // Used for testing

//std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
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
          digitalWrite(LED, HIGH);
          txValue=1;

        } else if (rxValue.find("OFF") != -1) {
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
          // Let's convert the value to a char array:
          char txString[8]; // make sure this is big enuffz
          dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

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

void setup() {
  Serial.begin(11500);
  pinMode(LED, OUTPUT); // Create the BLE Device
  pinMode (mPin1, OUTPUT);
  pinMode (mPin2, OUTPUT);
  pinMode (enPin, OUTPUT);
  pinMode (swPin, INPUT);
  //pinMode (testPin, INPUT);
  pinMode (testoutPin, OUTPUT);

  BLEDevice::init("MRCP Lock"); // Give it a name
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_READ
                    );

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->addServiceUUID(SERVICE_UUID);
            pAdvertising->setScanResponse(true);
            pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
            pAdvertising->setMinPreferred(0x12);
            BLEDevice::startAdvertising();
  Serial.print("Waiting a client connection to notify...");
}

void loop() {
/*
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
  */
}
