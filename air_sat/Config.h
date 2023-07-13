/* LIBRARY CONFIG TO SET PARAMETER AND SENSOR DATA */
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <SD.h>

// SETUP FOR BLUETOOTH MODULE ( Adresse Mac 98:d3:71:fe:09:0f )
#define COMM_BAUDRATE         115200
#define CONF_BAUDRATE         38400
#define GNSS_BAUDRATE         9600
// Pin used to set bluetooth module in config mode
#define KEY_PIN 6

String btName, macAddr, UARTConf;

#define SATELITE_NAME "AIR_SAT"
#define VERSION "1.0"

/********************************/
/* Config To Serail             */
/********************************/
void configToJson(){
    DynamicJsonDocument jsonConfig(256);
    jsonConfig["NAME"] = SATELITE_NAME;
    jsonConfig["VERSION"] = VERSION;
    jsonConfig["BT_NAME"] = btName;
    jsonConfig["BT_MACADDR"] = macAddr;
    jsonConfig["BT_UARTCONF"] = UARTConf;

    JsonObject Sensor1=jsonConfig.createNestedObject();
    Sensor1["SENSOR_NAME"]="BME280";
    
    JsonObject Sensor2=jsonConfig.createNestedObject();
    Sensor2["SENSOR_NAME"]="scd4x";
    Sensor2["CONSTRUCTOR"]="Sensirion";
    Sensor2["REF"]="https://sensirion.com/products/catalog/SCD41/";
    
    JsonObject Sensor3=jsonConfig.createNestedObject();
    Sensor3["SENSOR_NAME"]="GNSS";
    Sensor3["CONSTRUCTOR"]="Drotek";
    Sensor3["REF"]="https://sensirion.com/products/catalog/SCD41/";
    
    jsonConfig.add(Sensor1);
    jsonConfig.add(Sensor2);
    jsonConfig.add(Sensor3);

    // Using Logging Stream to send config to Serial(s)
    WriteLoggingStream loggedFile(Serial1, Serial);
    serializeJson(jsonConfig, loggedFile );

}

bool sendATCommand(const String& cmd, String* pAns = NULL) {

  char c;
  String tmp;
  // Writing command to module
  Serial1.println(cmd);
  // Waitng for data
  delay(150);
  // Reading data
  while (Serial1.available() && c != '\n') {
    c = Serial1.read();
    tmp += c;
  }
  // Clearing remaining OK answer
  Serial1.clear();
  // Checking fo errors
  if (tmp.length() == 0 || tmp.indexOf("ERROR") != -1) {
    tmp.remove(tmp.length()-1);
    Serial.println("An error occured when writing command '" + cmd + "' : " + ((tmp.length() == 0) ? "No response" : tmp ) + '.');
    return false;
  }
  // Affecting tmp to answer string if provided
  if (pAns)
    *pAns = tmp;

  return true;
}


void waitForReboot(const String& msg = "")  {
  if (msg != "")
    Serial.println(msg);
  Serial.println("Waiting for reboot...");
  //while(1);
}

bool readBTModuleConfig(String& btName, String& macAddr, String& UARTConf) {
  String temp;

  // Reading module Bluetooth name
  if (!sendATCommand("AT+NAME", &btName))
    return false;
  btName.remove(0,6); // Remove AT prefix
  btName.remove(btName.length()-2,2); // Remove "\r\n" final chars
  // Reading module MAC address
  if (!sendATCommand("AT+ADDR", &macAddr))
    return false;
  macAddr.remove(0,6);
  macAddr.remove(macAddr.length()-2,2);

  // Adding ":" to MACC ADDRESS ( 98d3:71:fe090f )
  temp = macAddr.substring(0,2) + ":" + macAddr.substring(2,10) + ":" + macAddr.substring(10, 12) + ":" + macAddr.substring(12,14) ;
  macAddr = temp.toUpperCase();

  // Reading module UART conf
  if (!sendATCommand("AT+UART", &UARTConf))
    return false;
  UARTConf.remove(0,6);
  UARTConf.remove(UARTConf.length()-2,2);

  return true;
}

String addChar(String str, char ch, int position) {
    return str.substring(0, position) + ch + str.substring(position);
}

bool configureBTModule(const String& btName, const String& UARTConf)  {

  if (!sendATCommand("AT+NAME=" + btName))
    return false;
  if (!sendATCommand("AT+UART=" + UARTConf))
    return false;
 
  return true;
}

/* ##############   SD CARD    ################ */
/*
   @brief: sets up sd card
*/
void setupSDCard()  {

  Serial.println("SD card setup... ");
  // Try to open SD card
  if (!SD.begin(BUILTIN_SDCARD))  {
    Serial.println("Failed, waiting for reboot...\n");;
    while (1);
  }
  else
    Serial.println("Setup SD Card done.\n");
//   deviceConnected = true;
}

bool logToSD(File& file, String log) {
  // Check if log file is open

  if (!file) {
    Serial.println("Error file doesn't exist");
    return false;
  }

  // Log into log file
  file.println(log);
  file.flush();
  // Serial.println("Log on SD Card done");
  return true;
}
