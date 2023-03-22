/*
  Inspiration :
  
    ModbusMaster library :
      RS485_HalfDuplex.pde - example using ModbusMaster library to communicate
      with EPSolar LS2024B controller using a half-duplex RS485 transceiver.


  Description :
  
    This program loads config and retrieves distance measured by an 
    URM14 DFRobots ultrasonic sensor.

  Board :
    Teensy 3.5

  RS485 interface :
    Mikroe RS485 click 2 (MAX3471)

  Wiring :
    RS485 interface RX      -> Teensy TX2
    RS485 interface TX      -> Teensy RX2
    RS485 interface DE & RE -> Teensy 11
    RS485 interface 3V3     -> Teensy 3V3
    RS485 interface GND     -> Teensy GND
    Sensor white wire       -> RS485 interface A
    Sensor blue wire        -> RS485 interface B
    Sensor red wire         -> RS485 interface Vcc
    Sensor black wire       -> RS485 interface Gnd

  Baudrates :
    Serial (USB Debug) : 115200
    Serial2 (Modbus communication) : 9600 (Baudrate configured in sensor)
    
*/

// Global definitions 
// Teensy Pins
#define DE_PIN  11 // RE = \DE => Wired to pin D5 as well
// Sensor id
#define SLAVE_ID      0x11
//Sensor registers
#define SLAVE_ID_REG  0x02
#define DISTANCE_REG  0x05
#define CONTROL_REG   0x08
//Sensor congig register bit values
#define   TEMP_CPT_SEL_BIT          ((uint16_t)0x01)      // Use custom temperature compensation
#define   TEMP_CPT_ENABLE_BIT       ((uint16_t)0x01 << 1) // Enable temperature compensation
#define   MEASURE_MODE_BIT          ((uint16_t)0x01 << 2) // Passive measure mode
#define   MEASURE_TRIG_BIT          ((uint16_t)0x01 << 3) // Request mesure in passive mode


/* --------------------------- */

#include <ModbusMaster.h>

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

// Globals
uint16_t control_reg_value = 0;
uint8_t comError;
float distance;


void setup()
{
   // DE_PIN as output and init in receive mode
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);

  // USB Debug
  Serial.begin(115200);
  
  // Modbus communication  (9600 baud)
  
  Serial2.begin(9600, SERIAL_8N1);
  // Modbus slave id 0x11 on serial port Serial2
  node.begin(SLAVE_ID, Serial2);
  // Load callbacks in ModbusMaster object
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // Initilaising config register
  control_reg_value |= MEASURE_MODE_BIT;//Set bit2 , Set to trigger mode
  control_reg_value &= ~(uint16_t)TEMP_CPT_SEL_BIT;//Select internal temperature compensation
  control_reg_value &= ~(uint16_t)TEMP_CPT_ENABLE_BIT;//enable temperature compensation
  // Writing config
  comError = node.writeSingleRegister(CONTROL_REG, control_reg_value); //Writes the setting value to the control register
  if (comError != ModbusMaster::ku8MBSuccess)
    Serial.println("Config could not be written to sensor...");
  else
    Serial.println("Sensor found and configured!");
  
}

void loop()
{
  // Config for reading distance once
  control_reg_value |= MEASURE_TRIG_BIT;//Set trig bit
  comError = node.writeSingleRegister(CONTROL_REG, control_reg_value); //Writes the setting value to the control register
  if (comError != ModbusMaster::ku8MBSuccess)
    Serial.println("Config could not be written to sensor...");
  
  // Readng distance input register at 0x05
  comError = node.readHoldingRegisters(DISTANCE_REG, 1);
  distance = node.getResponseBuffer(0)/10.0;

  if (comError != ModbusMaster::ku8MBSuccess)
    Serial.println("Sensor lost...");
  else
  {
    Serial.print("\tDistance = ");
    Serial.print(distance);
    Serial.println("mm");
  }
  
  delay(1000);
}
