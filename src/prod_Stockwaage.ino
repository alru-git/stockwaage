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
   
  Beispiel für serielle Ausgabe
  Von   https://github.com/bogde/HX711

  https://github.com/bogde/HX711/blob/master/examples/HX711SerialBegin/HX711SerialBegin.ino
  Es wird der Digitaleingang des Arduinio benutzt

Ver. 0.60:
  Testversion, um die Spg.-Versorgung zu testen
  Leerlaufspg. am 06.01.19 / 12:50 : 2x 1,59V
  Rohdaten der Waage
  Kalibrierter Wert der Waage
  Spannungpegel
  Messzyklus 1h
Ver. 0.61:
  SLEEP_TIME auf genau 1 h angepasst // ZuTestzwecken auf 1s gesetzt !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Ver. 0.70:
  mit Si7021 Sensor und interner Spannungsmessung für den ProMini
  die externe Spg.-Messung am A0 wurde deaktiviert
Ver. 0.71:
  Depricated Code entfernt
Ver. 0.80:
  Spannung an A0 aktiviert, interne Messung deaktiviert
  Spannungsmessung in Volt und %
  Heartbeat eingefügt
  

ToDo:
  Gateway anpassen
  SleepTime anpassen
  Prüfung auf Temp.Sensor nötig?

*/

// Enable debug prints to serial monitor
  #define MY_DEBUG

// Enable and select radio type attached
  #define MY_RADIO_NRF24
// MSG1 (Frequenz 2496 MHz)
  #define MY_RF24_CHANNEL 96
// MSG2 (Frequenz 2506 MHz)
// #define MY_RF24_CHANNEL 106

// Optional: Define Node ID
  #define MY_NODE_ID 199
// Node 0: MSG1 oder MSG2
// Node 120: Wasser.Terrasse, ist Repeater
  #define MY_PARENT_NODE_ID 0
  #define MY_PARENT_NODE_IS_STATIC

#include <MySensors.h>

// Temp.-Sensor
  #define CHILD_ID_HUM  0
  #define CHILD_ID_TEMP 1
  
  static bool metric = true;
  
  #include <SI7021.h>
  static SI7021 sensor;
// Ende


// Def. der Waage
  #include "HX711.h"
  
  #define DOUT  3
  #define CLK  2
  HX711 scale(DOUT, CLK);
  
  #define CHILD_ID_WEIGHT1 1
  #define CHILD_ID_WEIGHT2 2
  
  float raw_read;
  float weight_read;
  
  MyMessage msg1(CHILD_ID_WEIGHT1, V_WEIGHT);
  MyMessage msg2(CHILD_ID_WEIGHT2, V_WEIGHT);
// Ende

// Spannungsmessung
  int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
  int oldBatteryPcnt = 0;
  MyMessage msgV(0,V_VOLTAGE);
//Ende

// unsigned long SLEEP_TIME = 3496364; // 1h Sleep time between ADC ON/OFF (in milliseconds)
unsigned long SLEEP_TIME = 1000;

void setup() {
 // Setup der Waage 
    Serial.println("Initializing the scale");
    Serial.print("Raw reading: \t\t");
    Serial.println(scale.read());     // print a raw reading from the ADC
    Serial.print("Average of 20 Readings: \t\t");
    Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC
    Serial.print("get value: \t\t");
    Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)
    Serial.print("get units: \t\t");
    Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set)
  
    // scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
    scale.set_scale(); 
    scale.tare();               // reset the scale to 0
  
    Serial.println("After setting up the scale:");
    Serial.print("read: \t\t");
    Serial.println(scale.read());                 // print a raw reading from the ADC
    Serial.print("read average: \t\t");
    Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC
    Serial.print("get value: \t\t");
    Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()
    Serial.print("get units: \t\t");
    Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
              // by the SCALE parameter set with set_scale
 // Ende

 // Prüfung auf Temp.-Sensor
    while (not sensor.begin())
      {
        Serial.println(F("Sensor not detected!"));
        delay(5000);
      }

 // Vcc (3.3V) wird als Referenzspg. genutzt, um die externe Spg. zu messen
    analogReference(DEFAULT);
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
    sendSketchInfo("prod_Stockwaage", "0.80");
  // Waage
    present(CHILD_ID_WEIGHT1, S_WEIGHT);
    present(CHILD_ID_WEIGHT2, S_WEIGHT);

  // Temp.-Sensor
    present(CHILD_ID_HUM, S_HUM,   "Humidity");
    present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  
    metric = getControllerConfig().isMetric;

  // Spannungsmessung
    present(0, S_MULTIMETER);  
}

void loop() {
  // ADC wake up after sleep time
    scale.power_up();
  // Warten, damit sich die Spg. nach dem sleep stabilisiert
    wait(1000);
  // Update vom Reading heartbeat und state
    sendHeartbeat();
  
  // Waage
    Serial.print("read: \t\t");
    Serial.println(scale.read());                 // print a raw reading from the ADC
    Serial.print("10 read: \t\t");
    Serial.println(scale.read_average(10));   // print the average of 10 readings from the ADC
    Serial.print("Average of 10 readings:\t");
    Serial.println(scale.get_units(10), 1);

    raw_read = scale.read_average(10);
    weight_read =  scale.get_units(10), 1;
    
    {
      send(msg1.set(raw_read, 0));
      send(msg2.set(weight_read, 1));
    }
  // Ende

  // Temp.-Sensor
    // Read temperature & humidity from sensor.
      const float temperature = float( metric ? sensor.getCelsiusHundredths() : sensor.getFahrenheitHundredths() ) / 100.0;
      const float humidity    = float( sensor.getHumidityBasisPoints() ) / 100.0;
    
    #ifdef MY_DEBUG
      Serial.print(F("Temp "));
      Serial.print(temperature);
      Serial.print(metric ? 'C' : 'F');
      Serial.print(F("\tHum "));
      Serial.println(humidity);
    #endif
    
    static MyMessage msgHum( CHILD_ID_HUM,  V_HUM );
    static MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
    
    send(msgTemp.set(temperature, 2));
    send(msgHum.set(humidity, 2));
  // Ende
    
  // Spannungsmessung
      int sensorValue = analogRead(BATTERY_SENSE_PIN);
      #ifdef MY_DEBUG
        Serial.print(F("A0 Spg.-Wert "));
        Serial.println(sensorValue);
      #endif

      int batteryPcnt = sensorValue / 10 * 1.236;

      #ifdef MY_DEBUG
        float batteryV  = sensorValue * 0.003363075;
        Serial.print("Battery Voltage: ");
        Serial.print(batteryV);
        Serial.println(" V");
      
        Serial.print("Battery percent: ");
        Serial.print(batteryPcnt);
        Serial.println(" %");
      #endif

      if (oldBatteryPcnt != batteryPcnt) {
        sendBatteryLevel(batteryPcnt);
        oldBatteryPcnt = batteryPcnt;
        // Nachkommastellen müssen begrenzt werden
        send(msgV.set(batteryV,2));  
      }
  // Ende
  
  // put the ADC in sleep mode, sleep 
    scale.power_down(); 

    sleep (SLEEP_TIME);

}
