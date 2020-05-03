#pragma once

#include <Arduino.h>
#include <MRCPNode.h>

class MRCPMeter : public MRCPNode{
    public:
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
      static const int LED = 2;
      float rate = 5; // Set the cost ($) per hour here

      MRCPMeter(){
        name = "MRCP Meter";
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

      void onBLEWrite() {
        MRCPNode::onBLEWrite();
        if (rx_doc.size() > 0) {
          if (client_authenticated) {
          if (rx_doc["type"] == "START") {
            // State: s3 = Valid Parking, LED = Blink Green
            state = 3;
            tx_doc["type"] = "START_ACK";
            sendDoc(tx_doc);
            sonicSensor_init = readRange();
          }else if(rx_doc["type"] == "STOP_ACK"){
            //Reset ESP32
            ESP.restart(); // Not sure if this works
          }
        }
      }
    }

    int readRange(){
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      durationSoundWave = pulseIn(echoPin, HIGH);
      return durationSoundWave*0.034/2; // Returning the distance
    }

      void loop(){
        MRCPNode::loop();
        if(client_authenticated){
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
                   tx_doc["type"] = "STOP";
                   sendDoc(tx_doc);
                   state = 0;
                 }else{
                  if(millis() % 1000 > 500){
                    digitalWrite(ledGreen, LOW);
                  }else{
                    digitalWrite(ledGreen, HIGH);
                  }
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
                    if(millis() % 500 > 250){
                     digitalWrite(ledRed, LOW);
                    }else{
                     digitalWrite(ledRed, HIGH);
                    };
                  }
                else{
                  // State: s1 = standby, LED = Blink Yellow
                  state = 1;
                  if(millis() % 1000 > 500){
                    digitalWrite(ledGreen, HIGH);
                    digitalWrite(ledBlue, HIGH);
                    digitalWrite(ledRed, HIGH);
                  }else{
                    digitalWrite(ledGreen, LOW);
                    digitalWrite(ledBlue, LOW);
                    digitalWrite(ledRed, LOW);
                  }
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
      }
};