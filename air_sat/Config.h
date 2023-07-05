/* LIBRARY CONFIG TO SET PARAMETER AND SENSOR DATA */
#include <ArduinoJson.h>

// SETUP FOR BLUETOOTH MODULE ( Adresse Mac 98:d3:71:fe:09:0f )
#define COMM_BAUDRATE         115200
#define CONF_BAUDRATE         38400
#define GNSS_BAUDRATE         9600

#define SATELITE_NAME "AIR_SAT"
#define VERSION "1.0"


/********************************/
/* Config To Serail             */
/********************************/
void configToJson(){
    DynamicJsonDocument jsonConfig(256);
    jsonConfig["NAME"] = SATELITE_NAME;
    jsonConfig["VERSION"] = VERSION;

    JsonObject Sensor1=jsonConfig.createNestedObject();
    Sensor1["SENSOR_NAME"]="BME280";
    
    JsonObject Sensor2=jsonConfig.createNestedObject();
    Sensor2["SENSOR_NAME"]="scd4x";
    Sensor2["CONSTRUCTOR"]="Sensirion";
    Sensor2["REF"]="https://sensirion.com/products/catalog/SCD41/";
    
    jsonConfig.add(Sensor1);
    jsonConfig.add(Sensor2);

    serializeJson(jsonConfig,Serial);
    
    // String json = "{";
    // json += "\"NAME\":\""+ (String)SATELITE_NAME +"\",";
	// json += "\"VERSION\":\""+ (String)VERSION +" \",";
    // json += "\"SENSORS\":\""+ (String)VERSION +" \",";
    
    // json += "}";
    // Serial.println(json);
    // return json;
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
}
