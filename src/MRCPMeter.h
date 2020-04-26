#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

int readRange();

// Pins and variables for Ultrasonic
const int echoPin = 19;   // Red wire
const int trigPin = 21;   // Brown wire
long durationSoundWave;
int distance;
const int maxDetectionDistance = 100; // Distance a car needs to be within
const int minDetectionDistance = 15;   // Too close == error
const int tolerance = 3;              // incorporate to account for error

// Pins for RGB LED
const int ledRed = 18;
const int ledGreen = 17;
const int ledBlue = 16;

int state = 0;
int sonicSensor_init = 0;
int initialDistance = 0;

time_t start;
double duration;  // double or time_t?

// 10 seconds is just for testing/demo, ideally 300 seconds (5 min)?
double violationTimeLimit = 10;  // Grace period before violation (seconds)

double sonicSensor = 1;

class MRCPMeter : public MRCPNode{
    public:
      bool deviceConnected = false;
      static const int LED = 2;
      float rate = 5; // Set the cost ($) per hour here
      bool timeStarted = false;
      unsigned long previousTime;
      int sonicSensor_init;

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
      friend int readRange();
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
            // State: s3 = Valid Parking, LED = Blink Green
            state = 3;
            node->sendString("ACK_START");
            node->previousTime = millis();
            node->timeStarted = true;
            node -> sonicSensor_init = readRange();

          }else if(rxValue.find("REQ_RATE") != -1){
            node->sendFloat(node->rate);
          }else if(rxValue.find("ACK_RATE") != -1){
            // Do we need this? Could just have the REQ_START act as an ACK
          }else if(rxValue.find("ACK_STOP") != -1){
            //Reset ESP32
            ESP.restart(); // Not sure if this works

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
        pinMode(LED, OUTPUT);
        pinMode (echoPin, INPUT);
        pinMode (trigPin, OUTPUT);
        pinMode (ledRed, OUTPUT);
        pinMode (ledGreen, OUTPUT);
        pinMode (ledBlue, OUTPUT);
      }

      void loop(){
        MRCPNode::loop();

        distance = readRange();

        if(distance > 40) // Used for Testing
          digitalWrite(LED, HIGH);
        else
          digitalWrite(LED, LOW);
        Serial.print("Distance: ");
        Serial.println(distance);


        if(state == 3) {
                 // State: s3 = Valid Parking
                 distance = readRange();

                 if(distance < minDetectionDistance){
                   // State: s4 = Maintance Required (VALID), LED = Solid Green
                   // If sensor gets obstructed, then stay here until resolved.
                   digitalWrite(ledGreen, HIGH);
                   distance = readRange();
                 }
                 else if(distance>(sonicSensor_init+tolerance))
                 {
                   duration = difftime( time(0), start);  // Stopping the clock
                   Serial.println("Stopping!");
                   Serial.println(duration);
                   double totalCost = round(duration * (rate));  // Total cost to customer
                   // process payment?
                   Serial.println(totalCost);

                   sendString("REQ_STOP");
                   state = 0;
                 }else{
                   digitalWrite(ledGreen, HIGH);
                   delay(500);
                   digitalWrite(ledGreen, LOW);
                 }

            }else if (distance < maxDetectionDistance) {
              if(distance < minDetectionDistance){
                // State: s5 = Maintance Required (Invalid Parking), LED = Solid Red
                // If sensor gets obstructed, then stay here until resolved.
                digitalWrite(ledRed, HIGH);
                distance = readRange();
                }
              else{
                digitalWrite(ledRed, LOW);

                if(state == 0){
                  initialDistance = distance; // Saving the distance the car is currently at
                  start = time(0);     // Restarting the clock
                }

                if(distance <= (initialDistance+tolerance)){
                  duration = difftime( time(0), start);
                  if(duration >= violationTimeLimit){
                    // State: s2 = violation, LED = Blink RED
                    state = 2;
                    digitalWrite(ledRed, HIGH);
                    delay(250);
                    digitalWrite(ledRed, LOW);
                    delay(250);
                    }
                else{
                  // State: s1 = standby, LED = Blink Yellow
                  state = 1;
                  digitalWrite(ledGreen, HIGH);
                  digitalWrite(ledBlue, HIGH);
                  digitalWrite(ledRed, HIGH);
                  delay(500);
                  digitalWrite(ledGreen, LOW);
                  digitalWrite(ledBlue, LOW);
                  digitalWrite(ledRed, LOW);
                  delay(500);
                }
              }
              distance = readRange();
            }
        }
        else{
          state = 0;

        }
        // else, State: s0 = Available, LED = off
        delay(100);
      }
};

int readRange(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  durationSoundWave = pulseIn(echoPin, HIGH);
  return durationSoundWave*0.034/2; // Returning the distance
}
