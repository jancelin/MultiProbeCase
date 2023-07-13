// ADC quantum that represents the increment in voltage for every bit set to 1
#define ADC_QUANTUM   3.3/*V*/ / pow(2,10)/*values*/ // Voltage range / Number of possible values
// Voltage devider factor used to convert voltage measured by Teensy to actual sensor output voltage.
#define VOLT_DIV_FACTOR   2.87/*kOhms*/ / (2.87/*kOhms*/ + 1/*kOhms*/) // R1 / (R1 + R2) 
// Sensor data pin
#define TURB_PIN  16

void setup() {
  
  Serial.begin(9600);
}


void loop() {

  int analogValue = analogRead(TURB_PIN);
  // Get measured voltage on voltage divider
  float analogVoltage = analogValue * ADC_QUANTUM;
  // Compute the actual sensor voltage
  float sensorVoltage = analogVoltage / (VOLT_DIV_FACTOR);
  // Compute turbidity value with turbidity/voltage sensor curve
  float turb = -3169.75074208108*sensorVoltage + 11220.523803196;
  // Print results
  Serial.println("Analog Value: " + String(analogValue) + "\tAnalog Voltage: " + String(analogVoltage, 4) + "V \t Sensor : " + String(sensorVoltage, 4) + "V \t Turbidity : " + String(turb)); // print out the value you read:
  // 1s delay
  delay(1000);
}
