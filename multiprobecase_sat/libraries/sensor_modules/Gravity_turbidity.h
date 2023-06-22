/* 
 ***************************************
 *   GRAVITY TURBIDITY SENSOR MODULE   *
 ***************************************
 * @brief:
 *		This module is loaded to handle Gravity turbidity sensor setup and 
 *		turbidity acquisition.
 * @note:
 *		As the senor output range is 0-5V and the Teensy's input analog maximum 
 *		range is 0-3.3V, a voltage devider must be wired between the two devices.
 *
 * @ADC config (Default):
 *		AREF = 3.3V
 *		10bit resolution
 */
/*
 **************************
 *   GLOBAL DEFINITIONS   *
 **************************
 */
// ADC resolution
#define ADC_RESOLUTION	10/*bits*/
// ADC quantum
// Represents the conversion factor between ADC value and measured voltage
// Quantum = Voltage range / Number of possible values
#define ADC_QUANTUM 	3.3/*V*/ / pow(2,ADC_RESOLUTION)/*values*/
// Voltage devider factor used to convert voltage measured by Teensy to actual sensor output voltage.
// F = R2 / (R1 + R2)
#define VOLT_DIV_FACTOR	2.87/*kOhms*/ / (2.87/*kOhms*/ + 1/*kOhms*/)
// Minimum turbidity value
#define MIN_TURB_VALUE  0.0/*NTU*/
// Turbidiy value if sensor is out of range or disconnected
#define TURB_NO_VALUE	-1

/*
 *********************
 *   SENSOR CONFIG   *
 *********************
 */
// Teensy analog pin wired to voltage divided sensor output
#define TURBIDITY_PIN  33

/*
 *****************
 *   LIBRARIES   *
 *****************
 */
// None

/*
 ***************************
 *   FUNCTION PROTOTYPES   *
 ***************************
 */
void setupTurbSensor(volatile bool& deviceConnected);
float readTurbidity(const unsigned int& analog_pin, volatile bool& deviceConnected);

/*
 ****************************
 *   FUNCTION DEFINITIONS   *
 ****************************
 */
/*
 * @brief: 
 * 		checks if Gravity sensor is connected. If not, waits for reboot.
 * @params:
 * 		deviceConnected: bool to store if sensor is connected or not.
 */
void setupTurbSensor(volatile bool& deviceConnected)	{

	readTurbidity(TURBIDITY_PIN, deviceConnected);
	if (!deviceConnected)
		waitForReboot("No Gravity turbidity sensor detected...");

    deviceConnected = true;
}

/*
 * @brief: 
 *		returns computed turbidity from measured sensor output voltage.
 * @params:
 *		deviceConnected: bool to store if sensor is connected or not.
 * @retrun:
 *		turb: computed turbidity .
 */
float readTurbidity(const unsigned int& analog_pin, volatile bool& deviceConnected)	{

	// Get measured voltage on voltage divider
	float analogVoltage = analogRead(analog_pin) * ADC_QUANTUM;
    
	// Compute the actual sensor voltage
	float sensorVoltage = analogVoltage / (VOLT_DIV_FACTOR);
	
	// Compute turbidity value with turbidity/voltage sensor curve
	float turb = (-1120.4*(sensorVoltage*sensorVoltage)) + (5742.3*sensorVoltage) - 4352.9;

	// Checking if device still connected
	if (turb < MIN_TURB_VALUE) {
        turb = TURB_NO_VALUE;
		deviceConnected = false;
    }
	else
		deviceConnected = true;
	return turb;
}
