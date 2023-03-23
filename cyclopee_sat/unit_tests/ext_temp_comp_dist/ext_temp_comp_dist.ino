/* ------------------------------------------------------
 *
 * @insipration:
 *     ModbusMaster library half-duplex example.
 *     DallasTemperature library Simple example.
 * 
 * @brief:  This program reads the externaly temperature (DS18B20)
 *          compensated distance measured by an URM14 DFRobot
 *          ultrasonic sensor interfaced with a Mikroe RS485 click 2
 *          to a Teensy 3.5 board.
 *          
 * @board :
 *    Teensy 3.5
 *
 *  @RS485 interface :
 *    Mikroe RS485 click 2 (MAX3471)
 *
 * @wiring:
 *      Teensy TX4  |  ESP32 D2   -> RS485 RX
 *      Teensy RX4  |  ESP32 D4   -> RS485 TX 
 *      Teensy 30   |  ESP32 D5   -> RS485 DE 
 *      Teensy 3V3  |  ESP32 3V3  -> RS485 3V3
 *      Teensy GND  |  ESP32 GND  -> RS485 GND
 *      URM14 Blue  -> RS485 B
 *      URM14 White -> RS485 A
 *      URM14 Brown -> 12V                    
 *      URM14 Black -> 0V
 *      DS18B20 Red -> Teensy 3V3   |  ESP32 3V3
 *      DS18B20 Black -> Teensy GND |  ESP32 GND
 *      DS18B20 Yellow -> Teensy 21 |  ESP32 D18
 *      
 * @ports:
 *      Serial (115200 baud)
 *      Serial4 (9600 baud configured in URM14)
 *      
 * ------------------------------------------------------
 */
/* ############################
 * #    GLOBAL DEFINITIONS    #
 * ############################
 */
/* Board selection */
// Comment the board you're not using
//#define ESP32
#define TEENSY

/* ESP32 pins */
#ifdef ESP32
  // Modbus
  #define RX_PIN 4
  #define TX_PIN 2
  #define DE_PIN 5  // RE = ~DE => Wired to pin 30 as well
  // OneWire
  #define ONE_WIRE_BUS  18 // ESP32 temperature data wire pin
#endif
  /* TEENSY pins */
  #ifdef TEENSY
  // Modbus
  #define DE_PIN  30 // RE = ~DE => Wired to pin 30 as well
  // OneWire
  #define ONE_WIRE_BUS  21 // Teensy temperature data wire pin
#endif

/* Dallas temperature sensor */
#define DS_ID   0

/*  URM14 sensor */
// Sensor baudrate
#define URM14_BAUDRATE 9600
// Sensor id
#define UMR14_ID  0x11
// Sensor registers
#define URM14_ID_REG        0x02
#define URM14_DISTANCE_REG  0x05
#define URM14_EXT_TEMP_REG  0x07
#define URM14_CONTROL_REG   0x08
// Sensor congig register bit values
#define   TEMP_CPT_SEL_BIT      ((uint16_t)0x01)      // Use custom temperature compensation
#define   TEMP_CPT_ENABLE_BIT   ((uint16_t)0x00 << 1) // Enable temperature compensation
#define   MEASURE_MODE_BIT      ((uint16_t)0x01 << 2) // Passive measure mode
#define   MEASURE_TRIG_BIT      ((uint16_t)0x01 << 3) // Request mesure in passive mode

/* Debug */
// Uncomment to see debug output on Serail port
//#define DEBUG

/* ###################
 * #    LIBRARIES    #
 * ###################
 */
// Modbus
#include <ModbusMaster.h>
// OneWire bus & Dallas temperature
#include <OneWire.h>
#include <DallasTemperature.h>


/* #################
 * #    PROGRAM    #
 * #################
 */
/* Buses instanciation */
/* Modbus */
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

/* OneWire */
// Instantiate OneWire object
OneWire oneWire(ONE_WIRE_BUS);
//Instanciate Dallas temperature sensor network (1 sensor)
DallasTemperature sensors(&oneWire);

/* Global variables */
// Modbus
uint16_t urm14_control_bits = 0;  // Holds URM14 sensor binary config
uint8_t mb_error;   // Stores Modbus communication errors
// Sensor values
float dist_mm, ext_temp_C;

/*
 * @brief: Setup ports, buses and sensors
 * @Local variables:  Serial  -> USB debug (115200 baud)
 *                    Serial4 -> Modbus port (9600 baud)
 */
void setup()
{
  // USB Debug
  Serial.begin(115200);

  /* Modbus */
  // Set DE_PIN as output and init in receive mode
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);

#ifdef ESP32
  // Modbus communication  (9600 baud)
  Serial1.begin(URM14_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  // Modbus slave id 0x11 on serial port Serial4
  node.begin(UMR14_ID, Serial1);
#endif
#ifdef TEENSY
  // Modbus communication  (9600 baud)
  Serial4.begin(URM14_BAUDRATE);
  // Modbus slave id 0x11 on serial port Serial4
  node.begin(UMR14_ID, Serial4);
#endif
  // Set pre/post transmision callbacks in ModbusMaster object
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  /* URM14 sensor*/
  // Initilaising config register
  urm14_control_bits |= (uint16_t)TEMP_CPT_ENABLE_BIT;  // Enable temperature compensation
  urm14_control_bits |= (uint16_t)TEMP_CPT_SEL_BIT;// Select internal temperature compensation
  urm14_control_bits |= (uint16_t)MEASURE_MODE_BIT;// Set to trigger mode
#ifdef DEBUG
  Serial.print("Config bits for URM14 : ");
  Serial.println(urm14_control_bits, BIN);
#endif

  // Writing config
  mb_error = node.writeSingleRegister(URM14_CONTROL_REG, urm14_control_bits); //Writes the setting value to the control register
  if (mb_error != ModbusMaster::ku8MBSuccess) {
    Serial.println("Modbus : Config could not be written to UMR14 sensor...");
    Serial.println("Check wiring");
    Serial.println("Reboot required");
    while(1);
  }
  else
    Serial.println("Modbus : UMR14 sensor found and configured!");

  /* Dallas sensor */
  sensors.begin();
}

/*
 * @brief: Reads sensor values and writes them on Serial port.
 * @steps : 
 *    UMR14 sensor :
 *      - Set trigger bit to 1 (request measurement).
 *      - Ask for sensor value (readInputRegister).
 *      - Get sensor response in responseBuffer.
 *    Dallas sensor :
 *      - Request temperature.
 *      - Read response.
 */
void loop()
{
  /* Dallas sensor */
  sensors.requestTemperatures();
  ext_temp_C = sensors.getTempCByIndex(0);
  // Check for OneWire errors
  if (ext_temp_C == DEVICE_DISCONNECTED_C)
    Serial.println("OneWire : DS18B20 disconnected...");
  
  /* URM14 sensor */
  // Update external temperature register
  mb_error = node.writeSingleRegister(URM14_EXT_TEMP_REG, (uint16_t)(ext_temp_C*10)); //Writes the setting value to the control register
  if (mb_error != ModbusMaster::ku8MBSuccess)
    Serial.println("Modbus : Could update external temperature on URM14 sensor...");
  // Set trigger bit to request one measurement
  urm14_control_bits |= MEASURE_TRIG_BIT;//Set trig bit
  mb_error = node.writeSingleRegister(URM14_CONTROL_REG, urm14_control_bits); //Writes the setting value to the control register
  if (mb_error != ModbusMaster::ku8MBSuccess)
    Serial.println("Modbus : Could set trigger bit on URM14 sensor...");
  // Readng distance input register at 0x05
  // Should use readInputRegisters() but somehow doesn't work
  // Trhow ku8MBIllegalDataAddress error (0x02)
  /* ToDo : understand error (might be manufacturer who did not follow Modbus standard) */
  mb_error = node.readHoldingRegisters(URM14_DISTANCE_REG, 1);
  dist_mm = node.getResponseBuffer(0)/10.0;

#ifdef DEBUG
  // Check external temperature register value
  node.readHoldingRegisters(URM14_EXT_TEMP_REG, 1);
  Serial.print("External temperature register value = ");
  Serial.print(node.getResponseBuffer(0)/10.0);
  Serial.print("°C");
  Serial.println();
  // Check control register value
  node.readHoldingRegisters(URM14_CONTROL_REG, 1);
  Serial.print("Control register : ");
  Serial.print(node.getResponseBuffer(0), BIN);
  Serial.println("\n");
#endif

  /* Write data to Serial */
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
    Serial.print("Distance = ");
    Serial.print(dist_mm);
    Serial.print("mm");
    Serial.print("\tTemperature = ");
    Serial.print(ext_temp_C);
    Serial.print("°C");
    Serial.println();
  }
  
  delay(1000); 
}
