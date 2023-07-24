/* 
 *****************************************************
 *   GRAVITY ELECTRICAL CONDUCTIVITY SENSOR MODULE   *
 *****************************************************
 * @brief:
 *      This module is loaded to handle Gravity EC10 sensor setup and 
 *      conductivity acquisition.
 * @note:
 *      The sensor output is 3.2V top. No voltage divider requiered.
 *
 * @ADC config (Default):
 *      AREF = 3.3V
 *      10bit resolution
 */
/*
 **************************
 *   GLOBAL DEFINITIONS   *
 **************************
 */
// ADC resolution
#define ADC_RESOLUTION  10/*bits*/
// ADC quantum
// Represents the conversion factor between ADC value and measured voltage
// Quantum = Voltage range / Number of possible values
#define ADC_QUANTUM     3.3/*V*/ / pow(2,ADC_RESOLUTION)/*values*/
// Minimum EC values
#define MIN_EC_VOLT  0.0/*mV*/
#define EC_NO_VALUE   0.0/*mS/cm*/

/*
 *********************
 *   SENSOR CONFIG   *
 *********************
 */
// Teensy analog pin wired to sensor output
#define EC_PIN  A13

/*
 *****************
 *   LIBRARIES   *
 *****************
 */
#include "DFRobot_EC10.h"
#include <EEPROM.h>

/*
 ************************
 *   STATIC VARIABLES   *
 ************************
 */
static DFRobot_EC10 sensor;
static float ec_voltage = 0.0/*mV*/;

/*
 ***************************
 *   FUNCTION PROTOTYPES   *
 ***************************
 */
void setupCondSensor(volatile bool& deviceConnected);
float readRawConductivity(volatile bool& deviceConnected);
float computeConductivity();

/*
 ****************************
 *   FUNCTION DEFINITIONS   *
 ****************************
 */
/*
 * @brief: 
 *      checks if Gravity sensor is connected. If not, waits for reboot.
 * @params:
 *      deviceConnected: bool to store if sensor is connected or not.
 */
void setupCondSensor(volatile bool& deviceConnected)  {

    sensor.begin();
    readRawConductivity(deviceConnected);
    if (!deviceConnected)
        waitForReboot("No Gravity EC sensor detected...");
    
    deviceConnected = true;
}

/*
 * @brief: 
 *      returns Gravity EC output voltage.
 * @params:
 *      deviceConnected: bool to store if sensor is connected or not.
 * @retrun:
 *      sensor voltage output.
 */
float readRawConductivity(volatile bool& deviceConnected) {

    // Get measured voltage on voltage divider
    ec_voltage = analogRead(EC_PIN) * ADC_QUANTUM * 1000;

    // Checking if device still connected
    if (ec_voltage == MIN_EC_VOLT)
        deviceConnected = false;
    else
        deviceConnected = true;
    
    return ec_voltage;
}

/*
 * @brief: 
 *      returns EC computed using ec_voltage sensor module static variable.
 * @params:
 *      temperature : value for temperature compensation.
 * @retrun:
 *      computed EC.
 */
float computeConductivity(const float& temperature)    {

    // Compute EC value using DFRobot_EC10 library
    return sensor.readEC(ec_voltage, temperature);
}