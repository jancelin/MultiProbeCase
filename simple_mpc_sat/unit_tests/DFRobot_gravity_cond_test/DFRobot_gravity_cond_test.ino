#include <DFRobot_EC10.h>
#include <EEPROM.h>


#define ADC_RESOLUTION  10/*bits*/
#define ADC_QUANTUM     3.3/*V*/ / pow(2,ADC_RESOLUTION)/*values*/
#define EC_PIN  33

DFRobot_EC10 sensor;

void setup() {
  
  Serial.begin(9600);
  sensor.begin();
}

float sensorVoltage;
long timer = millis();

void loop() {

  if (millis() - timer > 1000)  {
    
    sensorVoltage = analogRead(EC_PIN) * ADC_QUANTUM * 1000;
    Serial.println("+---------------------------------------+");
    Serial.println("| Voltage (mV) \t| Conductivity (mS/cm)\t|");
    Serial.println("+---------------------------------------+");
    Serial.println("|    " + String(sensorVoltage,1) + "\t|\t" + String(sensor.readEC(sensorVoltage, 23.0), 3) + "\t\t|");
    Serial.println("+---------------------------------------+");
    Serial.println();
    timer = millis();
  }
  sensor.calibration(sensorVoltage, 23);
}
