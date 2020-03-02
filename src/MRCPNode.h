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

class MRCPNode{

    protected:
        String service_uuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
        String characteristic_uuid_rx = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
        String characteristic_uuid_tx = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
        String name = "MRCP Node";
        BLECharacteristicCallbacks* characteristic_callbacks;
        BLEServerCallbacks* server_callbacks;
        BLECharacteristic *pCharacteristic;

    public:
        MRCPNode();
        MRCPNode(String name, BLECharacteristicCallbacks* characteristic_callbacks, BLECharacteristic *pCharacteristic){
          this->characteristic_callbacks = characteristic_callbacks;
          this->server_callbacks = server_callbacks;
          this->name = name;
        }
        void setup(){
          Serial.begin(9600);
          BLEDevice::init(this->name.c_str()); // Give it a name
          // Create the BLE Server
          BLEServer *pServer = BLEDevice::createServer();
          pServer->setCallbacks(this->server_callbacks);

          // Create the BLE Service
          BLEService *pService = pServer->createService(this->service_uuid.c_str());

          // Create a BLE Characteristic
          pCharacteristic = pService->createCharacteristic(
                              this->characteristic_uuid_tx.c_str(),
                              BLECharacteristic::PROPERTY_NOTIFY |
                              BLECharacteristic::PROPERTY_READ
                            );

          pCharacteristic->addDescriptor(new BLE2902());

          BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                                this->characteristic_uuid_rx.c_str(),
                                                BLECharacteristic::PROPERTY_WRITE
                                              );

          pCharacteristic->setCallbacks(this->characteristic_callbacks);

          // Start the service
          pService->start();

          // Start advertising
          BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
                    pAdvertising->addServiceUUID(this->service_uuid.c_str());
                    pAdvertising->setScanResponse(true);
                    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
                    pAdvertising->setMinPreferred(0x12);
                    BLEDevice::startAdvertising();
          Serial.print("Waiting a client connection to notify...");
        }

        void loop(){
          
        }
};