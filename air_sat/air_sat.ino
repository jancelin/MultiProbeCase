/* TEST
 * @inspiration:
 *    sensirion sensor
 *   
 * @brief:
 *    This program send over Bluetooth Air Temperature, Humidity, CO2 and Atmospheric pressure 
 *    using a mix of GNSS time and Teensy clock.
 *   
 * @board :
 *    Teensy 3.5
 *    
 * @GNSS :
 *    Drotek DP0601 RTK GNSS (XL F9P)
 *
 * @wiring:
 *      Teensy RX5       -> Drotek - DP0601 UART1 B3 (TX)
 *      Teensy Vin (5V)  -> Drotek - DP0601 UART1 B1 (5V)
 *      Teensy GND       -> Drotek - DP0601 UART1 B6 (GND)
 *      Teensy SCL  -> BME280 & I2CS CO2
 *      Teensy SDA  -> BME280 & I2CS CO2
 *      Teensy 30   -> RS485 DE 
 *      Teensy 3V3  -> RS485 3V3
 *      Teensy GND  -> RS485 GND
 *      URM14 Blue  -> RS485 B
 *      URM14 White -> RS485 A
 *      URM14 Brown -> 12V                    
 *      URM14 Black -> 0V
 *      DS18B20 Red    -> Teensy 3V3
 *      DS18B20 Black  -> Teensy GND
 *      DS18B20 Yellow -> Teensy 21
 *      
 * @ports:
 *      Serial (115200 baud)
 *      GNSS_SERIAL (115200 baud for DP0601)
 *      URM14_SERIAL (9600 baud for URM14)
 * --------------------------
 */

#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "SparkFunBME280.h"
#include <TinyGPSPlus.h>
#include "Config.h"

// SENSORS
SensirionI2CScd4x scd4x;
BME280 sensorBME280;
TinyGPSPlus gps;

//Create variable to track time
int start_log = 1;
unsigned long updateTime = 0;
uint32_t logInterval = 2000;
unsigned long previousLogTime = 0;

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
        Serial.print("Error for SC4X trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error for SCD4X trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error for SCD4X trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");

    // BME 280
    if (sensorBME280.beginI2C() == false) //Begin communication over I2C
    {
      Serial.println("The sensor BME280 did not respond. Please check wiring.");
      //while(1); //Freeze
    }
    else
    {
      Serial.println("BME280 started & configured");
    }

    /* BLUETOOTH CONFIG */
    Serial1.begin(COMM_BAUDRATE);
    String ans = "";

    /*      GNSS From RX5       */
    Serial5.begin(GNSS_BAUDRATE);
}

/* *********************** */
/* ****** LOOP *********** */
/* *********************** */
void loop() {
    // Reading RX5 for GNSS data
    while (Serial5.available())
    {
      char c;
      c = Serial5.read();
      gps.encode(c);
    }

    // Receiving order from Bluetooth
    if (Serial1.available() > 0) {
        String data = Serial1.readStringUntil('\n');
        Serial.println( " - message received : ");
        Serial.println(data);
        commandManager(data);
        Serial1.println();
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

/********************************/
/* Management Command order     */
/********************************/
void commandManager(String message) {
  DynamicJsonDocument jsonDoc(256); 
  DeserializationError error = deserializeJson(jsonDoc, message);
  if(error) {
    Serial.println("parseObject() failed");
    //return false;
  }

  if ( jsonDoc["order"] == "getConfig") {
    Serial.println(" - getConfig received ");
    configToJson();
  }
  else if (jsonDoc["order"] == "restart") {
    Serial.println( " - RESTART in progress ");
    //_reboot_Teensyduino_();
  }
  // {"order":"update_interval", "value":1000}
  else if (jsonDoc["order"] == "update_interval" ) {
    Serial.print(" - Interval update received = ");
    Serial.println( jsonDoc["value"].as<long>() );
    if (jsonDoc["value"].as<long>() > 500 ) {
      logInterval = jsonDoc["value"].as<long>();
    }
    else { 
      Serial.println("Interval < 500 limit");
    }
    Serial.print(" - new Interval log set : " );
    Serial.println(logInterval);
    Serial1.println("{\"update_intervalAnswer\":{\"newInterval\":"+(String)logInterval + "}}");
    //logInterval = newInterval;
  }
  else if (jsonDoc["order"] == "startLog") {
    Serial.println( " - StartLog receivede");
    start_log = 1;
  }
  else if (jsonDoc["order"] == "stopLog") {
    Serial.println( " - StopLog received ");
    start_log = 0;
  }
}


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
