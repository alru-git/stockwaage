/*
* The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 * Beispiel für serielle Ausgabe
 * Von   https://github.com/bogde/HX711
 * https://github.com/bogde/HX711/blob/master/examples/HX711SerialBegin/HX711SerialBegin.ino
 * Es wird der Digitaleingang des Arduinio benutzt
 *******************************
 *
 * Arduino Code für das Projekt "Digitale Stockwaage"
 * https://github.com/alru-git/stockwaage
 * 
 *Ver. 1.00:
 *  Erster Einsatz im Betrieb an MSG2
*/

// Enable debug prints to serial monitor
  #define MY_DEBUG

// Enable and select radio type attached
  #define MY_RADIO_NRF24
// MSG1 (Frequenz 2496 MHz)
// #define MY_RF24_CHANNEL 96
// MSG2 (Frequenz 2506 MHz)
  #define MY_RF24_CHANNEL 106

// Optional: Define Node ID
  #define MY_NODE_ID 121
// Node 0: MSG1 oder MSG2
  #define MY_PARENT_NODE_ID 0
  #define MY_PARENT_NODE_IS_STATIC

#include <MySensors.h>

// --- Temp.-Sensor
  #define CHILD_ID_HUM  0
  #define CHILD_ID_TEMP 1
  
  static bool metric = true;
  
  #include <SI7021.h>
  static SI7021 sensor;
// --- Ende

// --- Def. der Waage
  #include "HX711.h"
  
  #define DOUT  3
  #define CLK  2
  HX711 scale(DOUT, CLK);
  
  #define CHILD_ID_WEIGHT1 1
  #define CHILD_ID_WEIGHT2 2
  
  // Ausgabe Berechnung mit manuell gesetztem Tara
    float raw_read;           
    float raw_zero = 106100;      // entspricht Nullwert
    float raw_kg = 28100;         // Kalibrationswert in kg
  // Ausgabewert mit autom. Tara funktioniert nur, wenn bei Neustart die Waage leer ist
    float weight_read;
  
  MyMessage msg1(CHILD_ID_WEIGHT1, V_WEIGHT);
  MyMessage msg2(CHILD_ID_WEIGHT2, V_WEIGHT);
// --- Ende

// --- Spannungsmessung
  int BATTERY_SENSE_PIN = A0;      // select the input pin for the battery sense point
  MyMessage msgV(0,V_VOLTAGE);
// --- Ende

unsigned long SLEEP_TIME = 600000;    // 10Min, Standard-Loop Dauer
unsigned long WAIT_Time1 = 1000;      // 1s, Wartezeit, zum Empfang von ACK
unsigned long WAIT_Time10 = 10000;    // 10s, Wartezeit, bis der 2. Sendeversuch erfolgt

void setup() {
 // --- Setup der Waage 
    Serial.println("Initializing the scale");
    Serial.print("Raw reading: \t\t");
    Serial.println(scale.read());               // print a raw reading from the ADC
    Serial.print("Average of 20 Readings: \t\t");
    Serial.println(scale.read_average(20));     // print the average of 20 readings from the ADC
    Serial.print("get value: \t\t");
    Serial.println(scale.get_value(5));         // print the average of 5 readings from the ADC minus the tare weight (not set yet)
    Serial.print("get units: \t\t");
    Serial.println(scale.get_units(5), 1);      // print the average of 5 readings from the ADC minus tare weight (not set)
  
    scale.set_scale(raw_kg);                    // Wert von raw_kg, siehe oben, this value is obtained by calibrating the scale with known weights
    // scale.set_scale();                       // so wird der Tara Wert wird ohne kg Kalibrierung gesetzt
    scale.tare();                               // reset the scale to 0
  
    Serial.println("After setting up the scale:");
    Serial.print("read: \t\t");
    Serial.println(scale.read());                 // print a raw reading from the ADC
    Serial.print("read average: \t\t");
    Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC
    Serial.print("get value: \t\t");
    Serial.println(scale.get_value(5));           // print the average of 5 readings from the ADC minus the tare weight, set with tare()
    Serial.print("get units: \t\t");
    Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
                                                  // by the SCALE parameter set with set_scale
 // --- Ende
 
 // Vcc (3.3V) wird als Referenzspg. genutzt, um die externe Spg. zu messen
    analogReference(DEFAULT);

 // --- Temp.Sensor
  while (not sensor.begin())
  {
    Serial.println(F("Sensor not detected!"));
    delay(5000);
  }
 // --- Ende

}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Stockwaage1", "1.00");
  // --- Waage
    present(CHILD_ID_WEIGHT1, S_WEIGHT);
    present(CHILD_ID_WEIGHT2, S_WEIGHT);

  // --- Temp.-Sensor
    present(CHILD_ID_HUM, S_HUM,   "Humidity");
    present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  
    metric = getControllerConfig().isMetric;

  // --- Spannungsmessung
    present(0, S_MULTIMETER);  
}

void loop() {
  // ADC wake up after sleep time
    scale.power_up();
  // Warten, damit sich die Spg. nach dem sleep stabilisiert
    wait(WAIT_Time1);
  // Update vom Reading heartbeat und state
    sendHeartbeat();
  
  // --- Waage
    Serial.print("read: \t\t");
    Serial.println(scale.read());               // print a raw reading from the ADC
    Serial.print("10 read: \t\t");
    Serial.println(scale.read_average(10));     // print the average of 10 readings from the ADC
    Serial.print("Average of 10 readings:\t");
    Serial.println(scale.get_units(10), 1);

    raw_read = scale.read_average(10);          // Absoluten Wert der Waage lesen
    raw_read = (raw_read - raw_zero) / raw_kg;  // Manuellen Tara und absoluten kg-Wert   
    weight_read =  scale.get_units(10), 2;      // Rel. kg-Wert mit autom. Tara
    
    {
      send(msg1.set(raw_read, 2));
        // ack Prüfung
        wait(WAIT_Time1);
        if (send(msg1.set(raw_read, 2), true)){       
          Serial.println("ACK war OK");
           }
        else {
          //msg wird noch einmal gesendet
          wait(WAIT_Time10);
          send(msg1.set(raw_read, 2));
          } 
       
      send(msg2.set(weight_read, 2));
        wait(WAIT_Time1);
        if (send(msg2.set(weight_read, 2), true)){       
          Serial.println("ACK war OK");
           }
        else {
          wait(WAIT_Time10);
          send(msg2.set(weight_read, 2));
          } 
    }
    // put the ADC in sleep mode 
    scale.power_down(); 
  // --- Ende

  // --- Temp.-Sensor
    // Read temperature & humidity from sensor.
      const float temperature = float( metric ? sensor.getCelsiusHundredths() : sensor.getFahrenheitHundredths() ) / 100.0;
      const float humidity    = float( sensor.getHumidityBasisPoints() ) / 100.0;
    
      Serial.print(F("Temp "));
      Serial.print(temperature);
      Serial.print(metric ? 'C' : 'F');
      Serial.print(F("\tHum "));
      Serial.println(humidity);
       
    static MyMessage msgHum( CHILD_ID_HUM,  V_HUM );
    static MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
    
    send(msgTemp.set(temperature, 1));
      wait(WAIT_Time1);
      if (send(msgTemp.set(temperature, 1), true)){       
        Serial.println("ACK war OK");
         }
      else {
        wait(WAIT_Time10);
        send(msgTemp.set(temperature, 1));
        } 
 
    send(msgHum.set(humidity, 1));
      wait(WAIT_Time1);
      if (send(msgHum.set(humidity, 1), true)){       
        Serial.println("ACK war OK");
         }
      else {
        wait(WAIT_Time10);
        send(msgHum.set(humidity, 1));
        } 
  // --- Ende
    
  // --- Spannungsmessung
      int sensorValue = analogRead(BATTERY_SENSE_PIN);
      
      #ifdef MY_DEBUG
        Serial.print(F("A0 Spg.-Wert "));
        Serial.println(sensorValue);
      #endif

     float batteryV = sensorValue * 0.02054; // Kalibrations-Faktor mit Multimeter ermittelt
      
     #ifdef MY_DEBUG
        Serial.print("Battery Voltage: ");
        Serial.print(batteryV);
        Serial.println(" V");
     #endif

     // Nachkommastellen müssen begrenzt werden
     send(msgV.set(batteryV, 2));
      wait(WAIT_Time1);
      if (send(msgV.set(batteryV, 2), true)){       
        Serial.println("ACK war OK");
         }
      else {
        wait(WAIT_Time10);
        send(msgV.set(batteryV, 2));
        }
  // --- Ende

  sleep (SLEEP_TIME);

}
