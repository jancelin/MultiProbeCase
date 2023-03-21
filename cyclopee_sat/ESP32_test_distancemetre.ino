
/*

  RS485_HalfDuplex.pde - example using ModbusMaster library to communicate
  with EPSolar LS2024B controller using a half-duplex RS485 transceiver.

  This example is tested against an EPSolar LS2024B solar charge controller.
  See here for protocol specs:
  http://www.solar-elektro.cz/data/dokumenty/1733_modbus_protocol.pdf

  Library:: ModbusMaster
  Author:: Marius Kintel <marius at kintel dot net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

// Global definitions 
// RS485 Pins
#define RX_PIN  4
#define TX_PIN  2
#define DE_PIN  5 // RE = \DE => Wired to pin 5 as well
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

#define MODBUSMASTER_LIB 1  // Using ModbusMaster library
#define DFROBOTS_LIB     0  // Using DFRobots Modbus RTU library

/* --------------------------- */

#if MODBUSMASTER_LIB

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
  Serial1.begin(9600, SERIAL_8N1);//, RX_PIN, TX_PIN);
  // Modbus slave id 0x11 on serial port Serial1
  node.begin(SLAVE_ID, Serial1);
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

#elif DFROBOTS_LIB

/* ----------------------------------------------- */

/*!
 * @file scanModbusID.ino
 * @brief 扫描modbus总线上，所有串口配置为9600波特率，8位数据位，无校验位，1位停止位的modbus从机的地址。
 * @n modbus从机设备地址范围为1~247(0x01~0xF7),0为广播地址，所有modbus从机接受到广播包
 * @n 都会处理，但不会响应。
 * @n 一个modbus主机可以连多个modbus从机，在运行此demo之前，必须知道modbus从机的波特率，数据位，校
 * @n 验位，停止位等串口配置。
 * @n connected table
 * ---------------------------------------------------------------------------------------------------------------
 * sensor pin |             MCU                | Leonardo/Mega2560/M0 |    UNO    | ESP8266 | ESP32 |  microbit  |
 *     VCC    |            3.3V/5V             |        VCC           |    VCC    |   VCC   |  VCC  |     X      |
 *     GND    |              GND               |        GND           |    GND    |   GND   |  GND  |     X      |
 *     RX     |              TX                |     Serial1 RX1      |     5     |5/D6(TX) |  D2   |     X      |
 *     TX     |              RX                |     Serial1 TX1      |     4     |4/D7(RX) |  D3   |     X      |
 * ---------------------------------------------------------------------------------------------------------------
 * @note: 不支持UNO，Microbit，ESP8266
 *
 * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [Arya](xue.peng@dfrobot.com)
 * @version  V1.0
 * @date  2021-07-16
 * @https://github.com/DFRobot/DFRobot_RTU
 */

#include "DFRobot_RTU.h"

DFRobot_RTU modbus(&Serial1, DE_PIN); // Modbus instance using port Serial1 and pin D5 as DE/RE pin

// Globals
uint16_t* control_reg_value = (uint16_t*) malloc(sizeof(uint16_t));
uint8_t comError;
float distance;

void setup() {
  
  Serial.begin(115200);
  while(!Serial); //Waiting for USB Serial COM port to open.

  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initilaising config register
  *control_reg_value |= MEASURE_MODE_BIT;//Set bit2 , Set to trigger mode
  *control_reg_value &= ~(uint16_t)TEMP_CPT_SEL_BIT;//Select internal temperature compensation
  *control_reg_value &= ~(uint16_t)TEMP_CPT_ENABLE_BIT;//enable temperature compensation
  // Writing config
  comError = modbus.writeHoldingRegister(SLAVE_ID, CONTROL_REG, control_reg_value, 1); //Writes the setting value to the control register
  if (comError != 0)
    Serial.println("Config could not be written to sensor...");
  else
    Serial.println("Sensor found and configured!");
  
  delay(1000);
  
}

void loop() {
  
  // Config for reading distance once
  *control_reg_value |= MEASURE_TRIG_BIT;//Set trig bit
  comError = modbus.writeHoldingRegister(SLAVE_ID, CONTROL_REG, control_reg_value, 1); //Writes the setting value to the control register
  if (comError != 0)
    Serial.println("Config could not be written to sensor...");
  
  // Reading distance input register at 0x05
  distance = modbus.readInputRegister(SLAVE_ID, SLAVE_ID_REG)/10.0;
  //distance = node.getResponseBuffer(0)/10.0;
  Serial.print("\tDistance = ");
  Serial.print(distance);
  Serial.println("mm");
  
  delay(1000);
  
}


#endif
