/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   updates by chegewara
   https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
   Adapted to BLENode class by Daniel Wright
*/

#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

bool deviceConnected;
bool led_on = false;
const int LED = 2;

class BLENode{

    private:
        String service_uuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
        String characteristic_uuid_rx = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
        String characteristic_uuid_tx = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
        BLECharacteristic *pCharacteristic;

    public:

        void setup(){
            Serial.begin(9600);
            Serial.println("Starting BLE work!");

            BLEDevice::init("ESP32 LED");
            BLEServer *pServer = BLEDevice::createServer();
            BLEService *pService = pServer->createService(service_uuid.c_str());

            pCharacteristic = pService->createCharacteristic(
                                                    characteristic_uuid_tx.c_str(),
                                                    BLECharacteristic::PROPERTY_NOTIFY
                                                );
            pCharacteristic->addDescriptor(new BLE2902());
            pCharacteristic = pService->createCharacteristic(
                                       characteristic_uuid_rx.c_str(),
                                       BLECharacteristic::PROPERTY_WRITE
                                     );
            pService->start();
            // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
            BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->addServiceUUID(service_uuid.c_str());
            pAdvertising->setScanResponse(true);
            pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
            pAdvertising->setMinPreferred(0x12);
            BLEDevice::startAdvertising();
            Serial.println("Characteristic defined! Now you can read it in your phone!");
        }

        void loop(){
            if (deviceConnected) {
                if(led_on){
                    pCharacteristic->setValue("1");
                }else{
                    pCharacteristic->setValue("0");
                }
                
                
                pCharacteristic->notify(); // Send the value to the app!
                Serial.print("*** Sent Value: ");
                Serial.print(led_on);
                Serial.println(" ***");
            }
            delay(1000);
        }

};

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
        }
        Serial.println();
        // Do stuff based on the command received from the app
        if (rxValue[0] == '1') {
          Serial.print("Turning ON!");
          digitalWrite(LED, HIGH);
        }
        else if (rxValue[0] == '0') {
          Serial.print("Turning OFF!");
          digitalWrite(LED, LOW);
        }
        Serial.println();
        Serial.println("*********");
      }
    }
};