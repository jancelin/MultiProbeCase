
#include <HardwareSerial.h>
#include <ArduinoModbus.h>
#include <ArduinoRS485.h>

// Global definitions 
// ESP32 Pins
#define RX_PIN  4 // Plugged to RS485 interface TX
#define TX_PIN  15 // Plugged to RS485 interface RX
#define DE_PIN  5 // RE = \DE => Wired to pin D5 as well
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


/*
 *@brief Read data from holding register of client
 *
 *@param addr ： Address of Client
 *@param reg: Reg index
 *@return data if execute successfully, false oxffff.
 */
uint16_t readData(uint16_t addr, uint16_t reg)
{
  uint16_t data;
  
  if (!ModbusRTUClient.requestFrom(addr, HOLDING_REGISTERS, reg, 1))  {
    
    Serial.print("failed to read registers! ");
    Serial.println(ModbusRTUClient.lastError());
    data = 0xffff;
    
  }
  else
    data =  ModbusRTUClient.read();
    
  return data;
}

/*
 *@brief write data to holding register of client 
 *
 *@param addr ： Address of Client
 *@param reg: Reg index
 *@param data: The data to be written
 *@return 1 if execute successfully, false 0.
 */
uint16_t writeData(uint16_t addr, uint16_t reg, uint16_t data)
{
  if (!ModbusRTUClient.holdingRegisterWrite(addr, reg, data)) {
    
    Serial.print("Failed to write coil! ");
    Serial.println(ModbusRTUClient.lastError());
    return 0;
    
  }
  else
    return 1;
}


float  dist;
volatile uint16_t control_reg_value = 0;

void setup() {

  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); 
  ModbusRTUClient.begin(9600, Serial1);
  Serial.begin(9600);
  
  control_reg_value |= MEASURE_MODE_BIT; //Set bit2 , Set to trigger mode
  control_reg_value &= ~(uint16_t)TEMP_CPT_SEL_BIT;  //Select internal temperature compensation
  control_reg_value &= ~(uint16_t)TEMP_CPT_ENABLE_BIT; //enable temperature compensation
  writeData(SLAVE_ID, CONTROL_REG, control_reg_value);  //Writes the setting value to the control register
  
  delay(100);
}

void loop() {
  
  control_reg_value |= MEASURE_TRIG_BIT; //Set trig bit
  writeData(SLAVE_ID, CONTROL_REG, control_reg_value); //Write the value to the control register and trigger a ranging
  
  delay(300); //Delay of 300ms(minimum delay should be greater than 30ms) is to wait for the completion of ranging
  
  dist = (float)readData(SLAVE_ID, DISTANCE_REG) / 10;  //Read distance register, one LSB is 0.1mm

  Serial.print("distance = ");
  Serial.print(dist, 1);
  Serial.println("mm");
  
}
