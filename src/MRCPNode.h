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
#include <ArduinoJson.h>
#include <FirebaseESP32.h>
#include <ESPRandom.h>

class MRCPNode{

  public:
      String service_uuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
      String characteristic_uuid_rx = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
      String characteristic_uuid_tx = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
      String name = "MRCP Node";
      BLECharacteristicCallbacks* characteristic_callbacks;
      BLEServerCallbacks* server_callbacks;
      BLECharacteristic *pCharacteristic;
      bool deviceConnected = false;
      StaticJsonDocument<256> rx_doc, tx_doc;
      //Firebase
      String firebase_host = "<project>.firebaseio.com";
      String firebase_auth = "<database auth token>";
      String wifi_ssid = "<AP SSID>";
      String wifi_password = "<AP password>";
      String uid = "u3RBOUI2kXUy2ZQFfN2oLHxgdjI3";
      FirebaseData firebase_data;
      String auth_token;
      StaticJsonDocument<256> client_doc;
      StaticJsonDocument<256> ui_info_doc;
      bool authenticating_client = false;
      bool client_authenticated = false;
      unsigned long auth_check_time = 0;

  class MRCPNodeServerCallbacks: public BLEServerCallbacks {
    public:
      MRCPNode* node;

      MRCPNodeServerCallbacks(MRCPNode* node){
        this->node = node;
      }

      void onConnect(BLEServer* pServer) {
       node->onBLEConnect();
      }

      void onDisconnect(BLEServer* pServer) {
        node->onBLEDisconnect();
      }
  };

  class MRCPNodeCharacteristicCallbacks: public BLECharacteristicCallbacks {
  public:

      MRCPNode* node;

      MRCPNodeCharacteristicCallbacks(MRCPNode* node){
        this->node = node;
      }

      void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        deserializeJson(node->rx_doc, rxValue);
        node->onBLEWrite();
    }
  };

  MRCPNode(){
    ui_info_doc["type"] = "UI_INFO";
  }

  virtual void setup(){
    Serial.begin(9600);
    server_callbacks = new MRCPNodeServerCallbacks(this);
    characteristic_callbacks = new MRCPNodeCharacteristicCallbacks(this);
    BLEDevice::init(name.c_str()); // Give it a name
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(server_callbacks);

    // Create the BLE Service
    BLEService *pService = pServer->createService(service_uuid.c_str());

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        characteristic_uuid_tx.c_str(),
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_READ
                      );

    pCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                          characteristic_uuid_rx.c_str(),
                                          BLECharacteristic::PROPERTY_WRITE
                                        );

    pCharacteristic->setCallbacks(characteristic_callbacks);

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
              pAdvertising->addServiceUUID(service_uuid.c_str());
              pAdvertising->setScanResponse(true);
              pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
              pAdvertising->setMinPreferred(0x12);
              BLEDevice::startAdvertising();
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    Firebase.begin(firebase_host, firebase_auth);
    Firebase.reconnectWiFi(true);

    Serial.println("Waiting a client connection to notify...");
  }

  virtual void onBLEConnect(){
    deviceConnected = true;
    Serial.println("Connected to Client");
  }

  virtual void onBLEDisconnect(){
    deviceConnected = false;
    authenticating_client = false;
    client_authenticated = false;
    auth_token = "";
    Serial.println("Disconnected from Client");
  }

  virtual void onBLEWrite(){
    if (rx_doc.size() > 0) {
      Serial.print("*** Received Value: ");
      serializeJson(rx_doc, Serial);
      Serial.println();

      if(rx_doc["type"] == "SYN") {
        client_doc = rx_doc;
        authenticating_client = true;
      }
      else if (client_authenticated) {
        if(rx_doc["type"] == "ACK"){
          tx_doc = ui_info_doc;
          sendDoc(tx_doc);
          if(!Firebase.setString(firebase_data, "/users/"+uid+"/authToken", "invalid"))
          {
            Serial.print("Error in setString, ");
            Serial.println(firebase_data.errorReason());
          }
        }
      }
    }
  }

  void authenticateClient(){
    String recieved_uid = client_doc["uid"];
    String recieved_token = client_doc["token"];
    if(Firebase.getString(firebase_data, "/users/"+recieved_uid+"/authToken"))
    {
      if(firebase_data.stringData() != "invalid" && firebase_data.stringData() == recieved_token){
        uint8_t uuid_array[16];
        ESPRandom::uuid(uuid_array);
        auth_token = ESPRandom::uuidToString(uuid_array);
        if(Firebase.setString(firebase_data, "/users/"+uid+"/authToken", auth_token))
        {
          client_authenticated = true;
          authenticating_client = false;
          onClientAuthenticated();
        }else{
          Serial.print("Error in setString, ");
          Serial.println(firebase_data.errorReason());
        }
      }else{
        client_authenticated = false;
        authenticating_client = false;
        Serial.println("Authentication Failed");
      }
    }else{
      Serial.print("Error in getString, ");
      Serial.println(firebase_data.errorReason());
      if(!Firebase.setString(firebase_data, "/users/"+uid+"/authToken", "invalid"))
        {
          Serial.print("Error in setString, ");
          Serial.println(firebase_data.errorReason());
          if(firebase_data.errorReason() == "connection refused"){
            authenticating_client = false;
            tx_doc["type"] = "ERROR";
            tx_doc["error"] = "Database Connection Refused";
            sendDoc(tx_doc);
          }
        }
    }
  }

  virtual void onClientAuthenticated(){
    Serial.println("App Authenticated");
    tx_doc["type"] = "SYN_ACK";
    tx_doc["uid"] = uid;
    tx_doc["token"] = auth_token;
    sendDoc(tx_doc);
  }

  void sendFloat(float value, int size = 8){
    char txString[size]; // make sure this is big enuffz
    dtostrf(value, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

    pCharacteristic->setValue(txString);

    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Float: ");
    Serial.print(txString);
    Serial.println();
  }

  void sendString(std::string value){
    pCharacteristic->setValue(value.c_str());
    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent String: ");
    Serial.print(value.c_str());
    Serial.println();
  }

  void sendDoc(StaticJsonDocument<256> doc){
    std::string value;
    serializeJson(doc, value);
    pCharacteristic->setValue(value.c_str());
    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Doc: ");
    Serial.print(value.c_str());
    Serial.println();
    tx_doc.clear();
  }

  virtual void loop(){
    if(authenticating_client && millis() >= auth_check_time){
      authenticateClient();
      auth_check_time = millis() + 2000;
    }
  }
};