// Default ADC config:
//	AREF = 3.3V
// 	10bit resolution

// ADC quantum that represents the increment in voltage for every bit set to 1
// Quantum = Voltage range / Number of possible values
#define ADC_QUANTUM   3.3/*V*/ / pow(2,10)/*values*/

// Voltage devider factor used to convert voltage measured by Teensy to actual sensor output voltage.
// F = R2 / (R1 + R2)
#define VOLT_DIV_FACTOR   2.87/*kOhms*/ / (2.87/*kOhms*/ + 1/*kOhms*/)


float readTurbidity(const unsigned int analog_pin)	{

	// Get measured voltage on voltage divider
	float analogVoltage = analogRead(analog_pin) * ADC_QUANTUM;
	
	// Compute the actual sensor voltage
	float sensorVoltage = analogVoltage / (VOLT_DIV_FACTOR);
	
	// Compute turbidity value with turbidity/voltage sensor curve
	return -1120.4*sensorVoltage*sensorVoltage + 5742.3*sensorVoltage - 4352.9;
}
