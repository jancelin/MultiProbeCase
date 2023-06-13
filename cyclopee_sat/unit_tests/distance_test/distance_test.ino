/* ----------------------------------------------
* @inspiration :
* 
*   ModbusMaster library :
*     Example RS485_HalfDuplex.pde
*
* @brief:
*    This program loads config and retrieves distance measured by an 
*    URM14 DFRobots ultrasonic sensor over Modbus data  bus.
*
* @board :
*    Teensy 3.5
* 
* @RS485 interface :
*    Mikroe RS485 2 click (MAX3471)
*    Sparkfun RS485 Breakout
*
* @wiring :
*    RS485 interface RX      -> Teensy TX4
*    RS485 interface TX      -> Teensy RX4
*    RS485 interface DE & RE -> Teensy 30
*    RS485 interface 3V3     -> Teensy 3V3
*    RS485 interface GND     -> Teensy GND
*    Sensor white wire       -> RS485 interface A
*    Sensor blue wire        -> RS485 interface B
*
* @baudrates :
*    Serial (USB Debug) : 115200
*    URM14_SERIAL (Modbus communication) : Baudrate configured in sensor
* ---------------------------------------------------    
*/
/* ############################
 * #    GLOBAL DEFINITIONS    #
 * ############################
 */

// Modbus
#define URM14_SERIAL Serial5
#define DE_PIN  35 // RE = \DE => Wired to pin 30 as well

// URM14 sensor
// Sensor baudrate
#define URM14_INITIAL_BAUDRATE  115200
#define URM14_NEW_BAUDRATE      9600
// Sensor id
#define URM14_ID      0x11
// Sensor registers
#define URM14_ID_REG        0x02
#define URM14_BAUDRATE_REG  0x03
#define URM14_DISTANCE_REG  0x05
#define URM14_CONTROL_REG   0x08
// Sensor baudrate config
#define URM14_BAUDRATE_CONFIG 0x03  // 9600 baud
// Sensor congig register bit values
#define   TEMP_CPT_SEL_BIT          ((uint16_t)0x01)      // Use custom temperature compensation
#define   TEMP_CPT_ENABLE_BIT       ((uint16_t)0x01 << 1) // Enable temperature compensation
#define   MEASURE_MODE_BIT          ((uint16_t)0x00 << 2) // Passive measure mode
#define   MEASURE_TRIG_BIT          ((uint16_t)0x01 << 3) // Request mesure in passive mode

// Debug
// Uncomment to see debug output on Serial port
//#define DEBUG

/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <ModbusMaster.h>

/* #################
 * #    PROGRAM    #
 * #################
 */
// Modbus
// Instantiate ModbusMaster object
ModbusMaster node;
// Callbacks for DE_PIN
void preTransmission()
{
  digitalWrite(DE_PIN, HIGH);
}
void postTransmission()
{
  digitalWrite(DE_PIN, LOW);
}

// Global variables
// Modbus
uint16_t urm14_control_bits = 0;  // Holds URM14 sensor binary config
uint8_t mb_error;  // Stores Modbus communication errors
// Sensor values
float dist_mm;

/*
 * @brief: setup ports, bus and sensor
 * @Local variables:  Serial  -> USB debug (115200 baud)
 *                    URM14_SERIAL -> Modbus port (9600 baud)
 */

void setup()
{
  // USB Debug
  Serial.begin(115200);

  // Modbus
  // DE_PIN as output and init in receive mode
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  
  // Modbus communication  (9600 baud)
  URM14_SERIAL.begin(URM14_INITIAL_BAUDRATE, SERIAL_8N1);
  // Modbus slave id 0x11 on serial port URM14_SERIAL
  node.begin(URM14_ID, URM14_SERIAL);
  
  // Set callbacks in ModbusMaster object
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // URM14 sensor
  // Initilaising config register
  urm14_control_bits |= (uint16_t)TEMP_CPT_ENABLE_BIT;
  urm14_control_bits |= (uint16_t)TEMP_CPT_SEL_BIT;
  urm14_control_bits |= (uint16_t)MEASURE_MODE_BIT;
#ifdef DEBUG
  Serial.print("Config bits for URM14 : ");
  Serial.println(urm14_control_bits, BIN);
#endif

  // Configuring URM14 baudrate
  mb_error = node.writeSingleRegister(URM14_BAUDRATE_REG,); //Writes the setting value to the control register
  if (mb_error != ModbusMaster::ku8MBSuccess) {
    Serial.println("Modbus : Baudrate could not be written to UMR14 sensor...");
    Serial.println("Check wiring.");
    Serial.println("Reboot required.");
    while(1);
  }
  else
    Serial.println("Modbus : UMR14 baudrate configured!");
  URM14_SERIAL.begin(URM14_NEW_BAUDRATE, SERIAL_8N1);

  // Writing config
  mb_error = node.writeSingleRegister(URM14_CONTROL_REG, urm14_control_bits); //Writes the setting value to the control register
  if (mb_error != ModbusMaster::ku8MBSuccess) {
    Serial.println("Modbus : Config could not be written to UMR14 sensor...");
    Serial.println("Check wiring.");
    Serial.println("Reboot required.");
    while(1);
  }
  else
    Serial.println("Modbus : UMR14 sensor configured!");
  
}

/*
 * @brief: Reads sensor values and writes them on Serial port.
 * @steps : 
 *    UMR14 sensor :
 *      - Set trigger bit to 1 (request measurement).
 *      - Ask for sensor value (readInputRegister).
 *      - Get sensor response in responseBuffer.
 */

void loop()
{
  // URM14
  // Config to read dist_mmance once
  urm14_control_bits |= MEASURE_TRIG_BIT;//Set trig bit
  mb_error = node.writeSingleRegister(URM14_CONTROL_REG, urm14_control_bits); //Writes the setting value to the control register
  if (mb_error != ModbusMaster::ku8MBSuccess)
    Serial.println("Modbus : Could set trigger bit on URM14 sensor...");
  // Readng distance input register at 0x05
  // Should use readInputRegisters() but somehow doesn't work
  // Trhow ku8MBIllegalDataAddress error (0x02)
  // ToDo : understand error (might be manufacturer who did not follow Modbus standard)
  mb_error = node.readHoldingRegisters(URM14_DISTANCE_REG, 1);
  dist_mm = node.getResponseBuffer(0)/10.0;
  
#ifdef DEBUG
  // Check control register value
  node.readHoldingRegisters(URM14_CONTROL_REG, 1);
  Serial.print("Control register : ");
  Serial.print(node.getResponseBuffer(0), BIN);
  Serial.println("\n");
#endif

  // Write data to Serial
  // Checking for Modbus errors
  if (mb_error != ModbusMaster::ku8MBSuccess) {
    Serial.println("Modbus : URM14 sensor lost...");
    Serial.print("Error : ");
    
    if (mb_error > 15)
      Serial.print("0x");
    else
      Serial.print("0x0");
      
    Serial.println(mb_error, HEX);
  }
  else  {
    Serial.print("Disance = ");
    Serial.print(dist_mm);
    Serial.println("mm");
  }
  
  delay(1000);
}
