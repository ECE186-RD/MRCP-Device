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

std::string delimiter = ","; // Probably Dont need, Delete Russell?
//const double rate = 1/36; // Based off of cost $0.25 per 15 min, but per seconds

time_t start;
double duration;  // double or time_t?

// 10 seconds is just for testing, ideally 300 seconds?
double violationTimeLimit = 10;  // Grace period before violation (seconds)

double sonicSensor = 1;
    //  float rate = 2;


class MRCPMeter : public MRCPNode{
    public:
      bool deviceConnected = false;
      static const int LED = 2;
      float rate = 2;
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
            node->sendString("ACK_START");
            node->previousTime = millis();
            node->timeStarted = true;
            int sonicSensor_init;
            //sonicSensor_init = node.readRange(); // Read in sensor value now
            sonicSensor_init = readRange();

            while((sonicSensor_init + tolerance) >= distance) {
                        /*
                         Will stay in this loop until sensor reads a further distance which means the car has left
                         Not closer since someone may walk infront of meter
                         Could account for Tape attack
                         */
                         // State: s3 = Valid Parking
                         distance = readRange();


                         while(distance < minDetectionDistance){
                           // State: s4 = Maintance Required (VALID), LED = Solid Green
                           // If sensor gets obstructed, then stay here until resolved.
                           digitalWrite(ledGreen, HIGH);
                           distance = readRange();
                         }

                         digitalWrite(ledGreen, HIGH);
                         delay(500);
                         digitalWrite(ledGreen, LOW);
                    }

                    duration = difftime( time(0), start);  // Stopping the clock
                    Serial.println("Stopping!");
                    Serial.println(duration);
                    double totalCost = round(duration * (node->rate));  // Total cost to customer
                    // process payment?
                    Serial.println(totalCost);


          }else if(rxValue.find("REQ_RATE") != -1){
            node->sendFloat(node->rate);
          }else if(rxValue.find("ACK_RATE") != -1){
            // Do we need this? Could just have the REQ_START act as an ACK
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

        if (distance < maxDetectionDistance) {
          while(distance < minDetectionDistance){
            // State: s5 = Maintance Required (Invalid Parking), LED = Solid Red
            // If sensor gets obstructed, then stay here until resolved.
            digitalWrite(ledRed, HIGH);
            distance = readRange();
          }
            digitalWrite(ledRed, LOW);

            int initialDistance = distance; // Saving the distance the car is currently at
            start = time(0);     // Restarting the clock
            while(distance <= (initialDistance+tolerance)){
              duration = difftime( time(0), start);
              if(duration >= violationTimeLimit){
                // State: s2 = violation, LED = Blink RED
                digitalWrite(ledRed, HIGH);
                delay(250);
                digitalWrite(ledRed, LOW);
                delay(250);
              }
              else{
                // State: s1 = standby, LED = Blink Yellow
                digitalWrite(ledGreen, HIGH);
                digitalWrite(ledBlue, HIGH);
                digitalWrite(ledRed, HIGH);
                delay(500);
                digitalWrite(ledGreen, LOW);
                digitalWrite(ledBlue, LOW);
                digitalWrite(ledRed, LOW);
                delay(500);
              }
              distance = readRange();
            }
        }
        // else, State: s0 = Available, LED = off
        delay(100);


        if(this->timeStarted){
          unsigned long currentTime = millis();
          if(currentTime - this->previousTime >= 20000){
            this->sendString("REQ_STOP");
            this->timeStarted = false;
          }
        }

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
