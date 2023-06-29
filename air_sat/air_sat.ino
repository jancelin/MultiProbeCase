/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "SparkFunBME280.h"
#include <TinyGPSPlus.h>

// SETUP FOR BLUETOOTH MODULE ( Adresse Mac 98:d3:71:fe:09:0f )
#define COMM_BAUDRATE         115200
#define CONF_BAUDRATE         38400



SensirionI2CScd4x scd4x;
BME280 sensorBME280;

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

//Create variable to track time
int start_log = 0;
unsigned long updateTime = 0;
uint32_t logInterval = 10000;
unsigned long previousLogTime = 0;
TinyGPSPlus gps;

void setup() {

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");

    // BME 280
    if (sensorBME280.beginI2C() == false) //Begin communication over I2C
    {
      Serial.println("The sensor did not respond. Please check wiring.");
      while(1); //Freeze
    }
    else
    {
      Serial.println("BME280 started & configured");
    }

    /* BLUETOOTH CONFIG */
    Serial1.begin(COMM_BAUDRATE);
    String ans = "";

    /****************************/
    /*      GNSS From RX5       */
    /****************************/
    Serial5.begin(9600);
     start_log = 1;
  logInterval = 2000;
}

void loop() {

    // Reading RX5 for GNSS data
    while (Serial5.available())
    {
      char c;
      c = Serial5.read();
      gps.encode(c);
    }

    if (  ((millis() - previousLogTime) >= logInterval || previousLogTime == 0 ) && start_log ) 
    {

    uint16_t error;
    char errorMessage[256];

    delay(100);

    // Read Measurement
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool isDataReady = false;
    error = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        Serial.print("Error trying to execute getDataReadyFlag(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        //Serial.println("CO2 Data not ready YET...");
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        String json = "{";
        json += "\"id\":\"Air_98:d3:71:fe:09:0f\",";
        json += "\"time\":\"" + (String)gps.date.year() + "/" + (String)gps.date.month() + "/" + (String)gps.date.day() + " ";
        json += (String)gps.time.hour() + ":" + (String)gps.time.minute() + ":" + (String)gps.time.second() + "." +  (String)gps.time.centisecond() + "\",";
        json += "\"lon\":" + String(gps.location.lng(),8) + ","; 
        json += "\"lat\":" + String(gps.location.lat(),8) + ",";
        json += "\"Co2\":" + (String)co2 + ",";
        json += "\"Co2_Temperature\":" + (String)temperature + ",";
        json += "\"Co2_Humidity\":" + (String)humidity + ",";
        json += "\"BME_Temperature\":" + (String)sensorBME280.readTempC() + ",";
        json += "\"BME_Humidity\":" + (String)sensorBME280.readFloatHumidity() + ",";
        json += "\"BME_Pressure\":" + (String)sensorBME280.readFloatPressure();
        json += "}";

        Serial.println(json);

        // Sending over Bluetooth
        Serial1.println(json);
        previousLogTime = millis(); 
    }
    }
}