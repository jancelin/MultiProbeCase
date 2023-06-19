// ADC quantum that represents the increment in voltage for every bit set to 1
#define ADC_QUANTUM   3.3/*V*/ / pow(2,10)/*values*/ // Voltage range / Number of possible values
// Voltage devider factor used to convert voltage measured by Teensy to actual sensor output voltage.
#define VOLT_DIV_FACTOR   2.87/*kOhms*/ / (2.87/*kOhms*/ + 1/*kOhms*/) // R1 / (R1 + R2) 


void setup() {
  
  Serial.begin(9600);
}


void loop() {
  // Get measured voltage on voltage divider
  float analogVoltage = analogRead(33) * ADC_QUANTUM;
  // Compute the actual sensor voltage
  float sensorVoltage = analogVoltage / (VOLT_DIV_FACTOR);
  // Compute turbidity value with turbidity/voltage sensor curve
  float turb = -1120.4*sensorVoltage*sensorVoltage + 5742.3*sensorVoltage - 4352.9;
  // Print results
  Serial.println("Analog : " + String(analogVoltage, 2) + "V \t Sensor : " + String(sensorVoltage, 3) + "V \t Turbidity : " + String(turb)); // print out the value you read:
  // 1s delay
  delay(1000);
}
