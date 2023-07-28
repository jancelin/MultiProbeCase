/* --------------------------
 * @inspiration:
 *    clock_logger
 *    GNSS_test
 *   
 * @brief:
 *    This program logs distance and temperature readings into a log file on the SD card 
 *    using GNSS time. GNSS signal quality is logged as well.
 *    Log file segmentation and new day file creation are handled.
 *   
 * @board :
 *    Teensy 3.5
 *
 * @RS485 interface :
 *    Mikroe RS485 click 2 (MAX3471)
 *    
 * @GNSS :
 *    Drotek DP0601 RTK GNSS (XL F9P)
 *
 * @wiring:
 *      Teensy RX2       -> GNSS_MODULE UART1 B3 (TX)
 *      Teensy Vin (5V)  -> GNSS_MODULE UART1 B1 (5V)
 *      Teensy GND       -> GNSS_MODULE UART1 B6 (GND)
 *      Teensy TX4  -> RS485 RX
 *      Teensy RX4  -> RS485 TX 
 *      Teensy 30   -> RS485 DE 
 *      Teensy 3V3  -> RS485 3V3
 *      Teensy GND  -> RS485 GND
 *      URM14 Blue  -> RS485 B
 *      URM14 White -> RS485 A
 *      URM14 Brown -> 12V                    
 *      URM14 Black -> 0V
 *      DS18B20 Red    -> Teensy 3V3
 *      DS18B20 Black  -> Teensy GND
 *      DS18B20 Yellow -> Teensy 14
 *      
 * @ports:
 *      Serial (115200 baud)
 *      URM14_SERIAL (configured baudrate for URM14)
 *      GNSS_SERIAL (configured baudrate for GNSS module)
 * --------------------------
 */
/* ###########################
 * #   GLOBAL DEFINITIONS   #
 * ###########################
 */

/************** SERIAL PORTS *****************/
#define GNSS_SERIAL Serial2
#define BLUETOOTH_SERIAL  Serial1
 
/************** TIMER INTERRUPTS INTERVALS *****************/
// Sensor acquisition interval
// Check sensor reading interrupt duration before setting the value
// 71s maximum
#define READ_INTERVAL 1000/*ms*/ * 1000/*µs/ms*/

// GNSS refresh interval
// Minimal refresh rate to get 20ms GNSS time resolution
#define GNSS_REFRESH_INTERVAL 1/*ms*/ * 1000/*ms/µs*/
// Logging segmentation interval
#define LOG_SEG_INTERVAL  30/*s*/ * 1000/*ms/s*/
// Digital I.O. refresh interval
#define IO_REFRESH_INTERVAL 50/*ms*/ * 1000/*µs/ms*/

/************** TEENSY PINS *****************/
// Logging LED
#define LOG_LED       13
// Modbus DE & RE pins
#define DE_PIN        30 // RE = ~DE => Wired to pin 30 as well
// Disable logging button
#define BUTTON_PIN    16

/************** BLUETOOTH MODULE *****************/
// Bluetooth module key (AT mode) pin
#define BLUETOOTH_KEY 2
// Bluetooth module communication baudrate
#define BLUETOOTH_COMM_BAUDRATE 115200
// BLUETOOTH INFO
#define BLUETOOTH_NAME      "Cyclopee"
#define BLUETOOTH_UART_CONF "115200,1,0"
#define BLUETOOTH_NRG_MODE  ""

/************** GNSS module *****************/
// GNSS commuiation baudrate
#define GNSS_BAUDRATE 115200//bauds
// Time value if GNSS module disconnected
#define NO_GNSS_TIME      24606099 // HH:MM:SS.CC
// Longitude/latitude value if GNSS module disconnected
#define NO_GNSS_LOCATION  91//°
// Altitude value if GNSS module disconnected
#define NO_GNSS_ALTITUDE  INT32_MAX
// NMEA messages inteval
#define GNSS_NMEA_INTERVAL  200//ms

/************** BLUETOOTH *****************/
#define MODULE_NAME "Cyclopee"

/************** DATA NUMBER OF DECIMALS *****************/
// Location
#define LOC_DECIMALS  9
// Elevation
#define ELV_DECIMALS  3
// PDOP
#define PDOP_DECIMALS 1
// Temperature
#define TEMP_DECIMALS 3
// Distance
#define DIST_DECIMALS 1

/************** BUFFERS *****************/
// Maximum buffer size
#define MAX_BUFFER_SIZE  100

/************** ENUMS *****************/
// Connected devices
enum Devices : uint8_t  {

  SD_CARD = 0,
  TEMPERATURE,
  DISTANCE,
  GNSS_MODULE,
  BLUETOOTH
};

/************** DEBUG *****************/
// Serial debug
// Set to 1 to see debug on Serial port
#if 1
#define SERIAL_DBG(...) {Serial.print(__VA_ARGS__);}
#else
#define SERIAL_DBG(...) {}
#endif
// File dump
// Set to 1 to dump open log file to Serial port
// Probably better to set Serial debug to 0
#define FILE_DUMP 0

/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <RingBuf.h>
#include <TinyGPSPlus.h>
#include <TimeLib.h>
#include <SD.h>
#include <Metro.h>

/* ###########################
 * #   FUNCTION PROTOTYPES   #
 * ###########################
 */
// Sd card setup
void setupSDCard(volatile bool& deviceConnected);
// Log file setup
void handleLogFile(File& file, String& dirName, String& fileName, TinyGPSPlus& gnss, Metro& logSegCountdown, volatile bool& deviceConnected);
bool logToSD(File& file, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const double& elv_m, const float& dist_mm, const float& temp_C);
void dumpFileToSerial(File& file);
// GNSS setup
void setupGNSS(TinyGPSPlus& gnss, volatile bool& deviceConnected);
void gnssRefresh();
// Bluetooth communication
void setupBluetooth(String& satelliteID, volatile bool& deviceConnected);
void sendDataToBluetooth(TinyGPSDate& gnssDate, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const double& elv_m, const char* fixMode, const char* pdop, const float& dist_mm, const float& temp_C);
void readBluetoothOrders();
// Sensor reading interrupt
void readSensors();
// Digital IO update interrupt
void handleDigitalIO();
// Handling errors
void waitForReboot(const String& msg);

/* ######################
 * #   SENSOR MODULES   #
 * ######################
 */
#include "DS18B20_temperature.h"
#include "A01NYUB_distance.h"

/* ##################
 * #    PROGRAM     #
 * ##################
 */
/************** GLOBALS *****************/
// Array to store devices connection state
volatile bool connectedDevices[4] = {false, false, false, false};

// LOGGING
// Log file
String logDir = "";
String logFileName = "";
File logFile;
// Log state (enabled/disabled)
volatile bool enLog = false;

// GNSS MODULE
// TinyGPSPlus objects to parse NMEA and store location and time
TinyGPSPlus gnss;
TinyGPSCustom gnssGeoidElv(gnss, "GNGGA", 11);
TinyGPSCustom gnssFixMode(gnss, "GNGGA", 6);
TinyGPSCustom gnssPDOP(gnss, "GNGSA", 15);

// Bluetooth
String satelliteID;

// Timer interrputs
IntervalTimer sensorRead_timer, gnssRefresh_timer, ioRefresh_timer;

// LED timers
Metro logLEDCountdown = Metro(1500);
Metro noLogLEDCountdown = Metro(600);
Metro errorLEDCountdown = Metro(150);

// Metro timers
// Timer to manage log segmentation time
Metro logSegCountdown = Metro(LOG_SEG_INTERVAL);
// Timer to dump log file every 1s
Metro fileDumpCountdown = Metro(1000);

/*
 *  @brief:
 *    Sets up SD card, GNSS module, distance and temperature sensors.
 */
void setup() {

  // Pin setup
  pinMode(LOG_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Turn logging LED on during setup
  digitalWrite(LOG_LED, HIGH);
  delay(50);

  // USB debug Serial port
  Serial.begin(115200);
  
  SERIAL_DBG("#### SETUP ####\n\n")

  // Bluetooth setup
  SERIAL_DBG("## BLUETOOTH\n")
  setupBluetooth(satelliteID, connectedDevices[BLUETOOTH]);
  SERIAL_DBG('\n')
  // SD card init
  SERIAL_DBG("## SD CARD\n")
  setupSDCard(connectedDevices[SD_CARD]);
  SERIAL_DBG('\n')
  // Setting up temperature sensor
  SERIAL_DBG("## TEMPERATURE SENSOR\n")
  //setupTempSensor(connectedDevices[TEMPERATURE]);
  SERIAL_DBG('\n')
  // Setting up distance sensor
  SERIAL_DBG("## DISTANCE SENSOR\n")
  //setupDistSensor(connectedDevices[DISTANCE]);
  SERIAL_DBG('\n')
  // GNSS set up
  SERIAL_DBG("## GNSS MODULE\n")
  setupGNSS(gnss, connectedDevices[GNSS_MODULE]);
  SERIAL_DBG('\n')
  // Setting up timer interrupts
  sensorRead_timer.begin(readSensors, READ_INTERVAL);
  sensorRead_timer.priority(200);
  gnssRefresh_timer.begin(gnssRefresh, GNSS_REFRESH_INTERVAL);
  gnssRefresh_timer.priority(190);
  ioRefresh_timer.begin(handleDigitalIO, IO_REFRESH_INTERVAL);
  ioRefresh_timer.priority(180);
  
  SERIAL_DBG("\n\n")

  // Turn log LED off after setup
  digitalWrite(LOG_LED, LOW);
}

/************** LOOP() GLOBAL VARS *****************/
// Buffers to store values to log
RingBuf <uint32_t, MAX_BUFFER_SIZE> time_buf;
RingBuf <double, MAX_BUFFER_SIZE> lng_buf, lat_buf, elv_buf;
RingBuf <const char*, MAX_BUFFER_SIZE> fixMode_buf, pdop_buf;
RingBuf <float, MAX_BUFFER_SIZE> extTemp_buf, dist_buf;

// Variables to store buffer readings
uint32_t time_ms;
double lng_deg, lat_deg, elv_m;
const char *fixMode, *pdop;
float extTemp_C, dist_mm;

/*
 * @brief:
 *    Prints devices connection state.
 *    If logging enabled (enLog) :
 *      - Prints open log file name;
 *      - Handles the log file (segementation, new day);
 *      - Logs data;
 *      - Dumps log file to Serial (if FILE_DUMP).
 *    Else :
 *      - Empty buffers into log file;
 *      - Closes log file if open;
 *      - Prints "Logging disabled...".
 */

void loop() {
  // Loop execution time
  //long t = micros();
  
  //readBluetoothOrders();

  // File management and data storage
  // If buffers are empty
  if (time_buf.isEmpty()) {
    if (!enLog)
      logFile.close();
  }
  else {
    // Handling log file management
    handleLogFile(logFile, logDir, logFileName, gnss, logSegCountdown, connectedDevices[SD_CARD]);
    // Create function for this
    time_buf.pop(time_ms);
    extTemp_buf.pop(extTemp_C);
    lng_buf.pop(lng_deg);
    lat_buf.pop(lat_deg);
    elv_buf.pop(elv_m);
    fixMode_buf.pop(fixMode);
    pdop_buf.pop(pdop);
    dist_buf.pop(dist_mm);
    // -----------------
    if ( !logToSD(logFile, time_ms, lng_deg, lat_deg, elv_m, fixMode, pdop, dist_mm, extTemp_C) )
      SERIAL_DBG("Logging failed...\n")
  }

  // Debug serial output
  SERIAL_DBG("#### LOOP FUNCTION ####\n\n")

  // Print conected devices state
  SERIAL_DBG("### DEVICES CONNECTED\n\n")
  SERIAL_DBG("SD CARD :\t")
  SERIAL_DBG(connectedDevices[SD_CARD])
  SERIAL_DBG('\n')
  SERIAL_DBG("TEMPERATURE :\t")
  SERIAL_DBG(connectedDevices[TEMPERATURE])
  SERIAL_DBG('\n')
  SERIAL_DBG("DISTANCE :\t")
  SERIAL_DBG(connectedDevices[DISTANCE])
  SERIAL_DBG('\n')
  SERIAL_DBG("GNSS MODULE :\t")
  SERIAL_DBG(connectedDevices[GNSS_MODULE])
  SERIAL_DBG("\n\n")

  // If logging enabled
  if (enLog) {
    // Print log file info
    SERIAL_DBG("### LOG FILE\n\n")
    if (logFile) {
      SERIAL_DBG("File : ")
      SERIAL_DBG(logDir)
      SERIAL_DBG("/")
      SERIAL_DBG(logFileName)
      SERIAL_DBG('\n')
      SERIAL_DBG("\n###\n")
    }
    else
      SERIAL_DBG("No log file open...\n")
  }
  else
    SERIAL_DBG("Logging disabled...\n")

  // Dumping log file to Serial
  if (FILE_DUMP && fileDumpCountdown.check())
    dumpFileToSerial(logFile);

  SERIAL_DBG("\n\n")

  // Loop execution time
  //Serial.println(micros() - t);
}


/* ############################
 * #   FUNCTION DEFINITIONS   #
 * ############################
 */
/* ##############   TIMER INTERRUPT    ################ */
/*
 * @brief: 
 *    Interrupts loop() to read sensors.
 * @exec time:
 *     Long and mostly depends on temperature resolution config.
 */
void readSensors()  {
  // Interrupt execution time
  //long t = millis();
  
  // If logging enabled and logFile open
  if (enLog) {
    // If buffer not full
    if ( !time_buf.isFull() ) {

      // Interrupt safe GNSS data push into buffers
      if (gnss.time.isUpdated())
        time_buf.lockedPush(gnss.time.value());
      else
        time_buf.push(NO_GNSS_TIME);
      if (gnss.location.isUpdated()) {
        lng_buf.lockedPush(gnss.location.lng());
        lat_buf.lockedPush(gnss.location.lat());
      }
      else  {
        lng_buf.push(NO_GNSS_LOCATION);
        lat_buf.push(NO_GNSS_LOCATION);
      }

      if (gnss.altitude.isUpdated())
        elv_buf.lockedPush(gnss.altitude.meters() + strtod(gnssGeoidElv.value(), NULL));
      else
        elv_buf.push(NO_GNSS_ALTITUDE);

      pdop_buf.lockedPush(gnssPDOP.value());
      fixMode_buf.lockedPush(gnssFixMode.value());

      // Acquire temperature
      extTemp_buf.push(readTemperature(connectedDevices[TEMPERATURE]));
      // Acquire distance
      dist_buf.push(readDistance(extTemp_buf[extTemp_buf.size()-1], connectedDevices[DISTANCE]));
    }
    else
      SERIAL_DBG("Buffer is full!\n")
  }

  // Interrupt execution time
  //Serial.println(millis() - t);
}

/* ##############   BLUETOOTH    ################ */

bool sendATCommand(const String& cmd, String* pAns = NULL) {

  char c = '\0';
  String tmp;
  // Writing command to module
  BLUETOOTH_SERIAL.println(cmd);
  // Waitng for data
  delay(150);
  // Reading data
  while (BLUETOOTH_SERIAL.available() && c != '\n') {
    c = BLUETOOTH_SERIAL.read();
    tmp += c;
  }
  // Clearing remaining OK answer
  BLUETOOTH_SERIAL.clear();
  // Checking fo errors
  if (tmp.length() == 0 || tmp.indexOf("ERROR") != -1) {
    tmp.remove(tmp.length()-1);
    SERIAL_DBG("An error occured when writing command '" + cmd + "' : " + ((tmp.length() == 0) ? "No response" : tmp ) + ".\n");
    return false;
  }
  // Affecting tmp to answer string if provided
  if (pAns)
    *pAns = tmp;
  return true;
}

bool readBTModuleConfig(String& btName, String& macAddr, String& UARTConf, String& nrgMode) {

  // Reading module Bluetooth name
  if (!sendATCommand("AT+NAME", &btName))
    return false;
  btName.remove(0,6); // Remove AT prefix
  btName.remove(btName.length()-2,2); // Remove "\r\n" final chars
  // Reading module MAC address
  if (!sendATCommand("AT+ADDR", &macAddr))
    return false;
  macAddr.remove(0,6);
  macAddr.remove(macAddr.length()-2,2);
  // Reading module UART conf
  if (!sendATCommand("AT+UART", &UARTConf))
    return false;
  UARTConf.remove(0,6);
  UARTConf.remove(UARTConf.length()-2,2);

  return true;
}

bool configureBTModule(const String& btName, const String& UARTConf, const String& nrgMode)  {

  if (!sendATCommand("AT+NAME=" + btName))
    return false;
  if (!sendATCommand("AT+UART=" + UARTConf))
    return false;
 
  return true;
}

void setupBluetooth(String& satelliteID, volatile bool& deviceConnected)  {

  String btName, macAddr, UARTConf, nrgMode;

  // Pin setup
  pinMode(BLUETOOTH_KEY, OUTPUT);
  // Wait for Bluetooth module to boot
  delay(600);
  // Set AT mode pin high for module configuration
  digitalWrite(BLUETOOTH_KEY, HIGH);

  // Serial port setup
  BLUETOOTH_SERIAL.begin(BLUETOOTH_COMM_BAUDRATE);

  // Checking for bluetooth module presence
  if (!sendATCommand("AT"))
    waitForReboot("No module detected, check wiring...");
  SERIAL_DBG("Module detected.\n");

  // Configuring Bluetooth module if need be
  SERIAL_DBG("Configuring Bluetooth module... ")
  if (!configureBTModule(BLUETOOTH_NAME, BLUETOOTH_UART_CONF, BLUETOOTH_NRG_MODE))
    waitForReboot("Could not configure Bluetooth module.");
  SERIAL_DBG("Done.\n");
  // Reading module config
  SERIAL_DBG("Module config :\n");
  if (!readBTModuleConfig(btName, macAddr, UARTConf, nrgMode))
    waitForReboot("Could not read Bluetooth module config.");
  SERIAL_DBG("BT name :\t" + btName + '\n');
  SERIAL_DBG("MAC adress :\t" + macAddr + '\n');
  SERIAL_DBG("UART config :\t" + UARTConf + '\n');
  SERIAL_DBG('\n');

  // Generating satellite ID
  SERIAL_DBG("Generating satellite ID :\n");
  satelliteID = btName + ';' + macAddr;
  SERIAL_DBG(satelliteID + '\n');
  if (satelliteID.indexOf("ERROR") != -1)
    waitForReboot("Error generating satellite ID.");

  // Stop module configuration
  digitalWrite(BLUETOOTH_KEY, LOW);
  // Reboot module in Bluetooth mode
  sendATCommand("AT+RESET");

  // Configure Bluetooth for data comunication
  BLUETOOTH_SERIAL.begin(BLUETOOTH_COMM_BAUDRATE);
  
  deviceConnected = true;

  SERIAL_DBG("Done.\n")
} 

void json_logStr(String& str, const String& satelliteID, TinyGPSDate& gnssDate, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const double& elv_m, const char* fixMode, const char* pdop, const float& dist_mm, const float& temp_C) {

  String timeVal_str = "", date_str = "";
  str = "" ;
  timeValToStr(timeVal, timeVal_str);
  dateToStr(gnssDate, date_str);
  
  str += '{';
  // Inserting satellite id
  str += "\"id\":\"" + satelliteID + "\",";
  // Inserting date and time
  str += "\"time\":";
  if (timeVal != NO_GNSS_TIME)
    str += "\"" + date_str.replace('_', '/') + " " + timeVal_str + "\"";
  else
    str += "null";
  str += ',';
  // Inserting longitude
  str += "\"lon\":";
  if (lng_deg != NO_GNSS_LOCATION)
    str += String(lng_deg, LOC_DECIMALS);
  else
    str += "null";
  str += ',';
  // Inserting latitude
  str += "\"lat\":";
  if (lat_deg != NO_GNSS_LOCATION)
    str += String(lat_deg, LOC_DECIMALS);
  else
    str += "null";
  str += ',';
  // Inserting elevation
  str += "\"elv\":";
  if (elv_m != NO_GNSS_ALTITUDE)
    str += String(elv_m, ELV_DECIMALS);
  else
    str += "null";
  str += ',';
  // Inserting GNSS fix mode
  str += "\"fix\":" + String(fixMode) + ',';
  // inserting GNSS PDOP value
  str += "\"pdop\":" + String(pdop) + ',';
  // Inserting distance
  str += "\"dist\":";
  if (dist_mm != DIST_NO_VALUE)
    str += String(dist_mm, DIST_DECIMALS);
  else
    str += "null";
  str += ',';
  // Inserting temperature
  str += "\"temp\":";
  if (temp_C != TEMP_NO_VALUE)
    str += String(temp_C, TEMP_DECIMALS);
  else
    str += "null";
  str += '}';
}

void sendDataToBluetooth(const String& satelliteID, TinyGPSDate& gnssDate, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const double& elv_m, const char* fixMode, const char* pdop, const uint16_t& dist_mm, const float& temp_C)  {

  String str = "";
  json_logStr(str, satelliteID, gnssDate, timeVal, lng_deg, lat_deg, elv_m, fixMode, pdop, dist_mm, temp_C);
  BLUETOOTH_SERIAL.println(str);
  
}

void readBluetoothOrders()  {

  char c = BLUETOOTH_SERIAL.read();
  if (c != (char) -1)
    Serial.print(c);
}

/* ##############   GNSS    ################ */
/*
 * @brief: 
 *    Sets up communication with GNSS module and gets date an time.
 *    Waits 7s for GNSS signal.
 *    Acquires date and time.
 * @params:
 *    gnss: TinyGPSPlus object to update with date and time.
 *    deviceConnected: Boolean to store if GNSS module is connected.
 */
void setupGNSS(TinyGPSPlus& gnss, volatile bool& deviceConnected) {

  // Store start time to detect timeout
  long startTime = millis();

  // GNSS module Serial port
  GNSS_SERIAL.begin(GNSS_BAUDRATE);

  // Wait 7s for GNSS signal before timeout
  SERIAL_DBG("Waiting for GNSS signal... ")
  while (!GNSS_SERIAL.available())  {
    if (millis() - startTime > 7000)
      waitForReboot("No signal, check GNSS receiver wiring.");
  }
  SERIAL_DBG("Done.\n")
  // Acquiring GNSS date and time
  SERIAL_DBG("Acquiring GNSS date and time... ")
  while (gnss.date.value() == 0 && gnss.time.value() == 0)
    gnssRefresh();
  SERIAL_DBG("Done.\n")
  
  deviceConnected = true;
}

/*
 * @brief:  
 *    Interrupts loop() to refresh TinyGPSPlus object with NMEA data.
 *    Checks if GNSS module still connected by checking the number of caraters received during NMEA intervals
 */
void gnssRefresh() {

  // Static varible to store current NMEA interval start time
  static long intervalStart = millis();
  // Static variable to store the number of characters received until previous NMEA interval
  static uint32_t nbCharsProcessed = 0;

  // If NMEA interval is over
  if (millis() - intervalStart > GNSS_NMEA_INTERVAL)  {
    // If not enough characters were received
    if (gnss.charsProcessed() - nbCharsProcessed < 10)
      connectedDevices[GNSS_MODULE] = false;
    else
      connectedDevices[GNSS_MODULE] = true;
    // Update NMEA interval start time to the beginning of the new current NMEA interval.
    intervalStart = millis();
    // Store the total number of characters read until the previously current interval.
    nbCharsProcessed = gnss.charsProcessed();
  }
  // Feed TinyGPSPlus object with NMEA data
  while (GNSS_SERIAL.available())
     gnss.encode(GNSS_SERIAL.read());
}


/* ##############   SD CARD    ################ */
/*
   @brief: 
      Sets up sd card.
   @params:
      deviceConnected: Boolean to store device connection state.
*/
void setupSDCard(volatile bool& deviceConnected)  {

  SERIAL_DBG("SD card setup... ")
  // Try to open SD card
  if (!SD.begin(BUILTIN_SDCARD))
    waitForReboot("Failed.");
  else
    SERIAL_DBG("Done.\n")
  deviceConnected = true;
}

/* ##############   FILE MANAGEMENT    ################ */
/*
 * @brief:
 *    Convert and write TinyGPSDate date obj into a string.      
 * @params:
 *    gnssDate : TinyGPSDate obj to convert and write.
 *    str : String to write date into.
 */
void dateToStr(TinyGPSDate& gnssDate, String& str) {
  
  str = String(gnssDate.year()) + '_' + 
        ( (gnssDate.month() < 10) ? '0' + String(gnssDate.month()) : String(gnssDate.month()) ) + '_' + 
        ( (gnssDate.day() < 10) ? '0' + String(gnssDate.day()) : String(gnssDate.day()) );
}
  
/*
 * @brief:
 *    Convert and write TinyGPSTime time obj into a string.      
 * @params:
 *    gnssTime : TinyGPSTime obj to convert and write.
 *    str : String to write time into.
 */
void timeToStr(TinyGPSTime& gnssTime, String& str) {

  str = ( (gnssTime.hour() < 10) ? '0' + String(gnssTime.hour()) : String(gnssTime.hour()) ) + '_' + 
        ( (gnssTime.minute() < 10) ? '0' + String(gnssTime.minute()) : String(gnssTime.minute()) ) + '_' + 
        ( (gnssTime.second() < 10) ? '0' + String(gnssTime.second()) : String(gnssTime.second()) );
}

/*
 * @brief:
 *    Convert and write time value from TinyGPSPlus obj into a string.    
 * @params:
 *    timeVal : Time value to convert and write.
 *    str : String to write time into.
 */
void timeValToStr(const uint32_t& timeVal, String& str) {

  uint32_t tmp = timeVal;

  int h = tmp/1000000;
  tmp %= 1000000;
  int m = tmp/10000;
  tmp %= 10000;
  int s = tmp/100;
  tmp %= 100;
  int ms = tmp;
  
  str = ( (h < 10) ? '0' + String(h) : String(h) ) + ':' + 
        ( (m < 10) ? '0' + String(m) : String(m) ) + ':' + 
        ( (s < 10) ? '0' + String(s) : String(s) ) + ':';
  if (ms < 100)  {
    str += '0';
    if (ms < 10)
      str += '0'; 
  }
  str += ms;
}
/*
 * @brief: 
 *    Create new log dir 'dirName' if does not already exist.
 * @params:
 *    dirName : New dir name.
 */
void newLogDir(String& dirName)  {

  if (SD.exists(dirName.c_str()))  {
    SERIAL_DBG("Dir '")
    SERIAL_DBG(dirName)
    SERIAL_DBG("' already exists...\n")
  }
  else  {
    if (!SD.mkdir(dirName.c_str()))  {
      SERIAL_DBG("Could not create dir '")
      SERIAL_DBG(dirName)
      SERIAL_DBG("'...\n")
    }
  }
}

/*
 * @brief: 
 *    Create and open a new log file 'dirName/fileName' if does not already exist
 * @params:
 *    file: File object.
 *    dirName: Directory name in which file will be created.
 *    fileName : New file name.
 */
bool newLogFile(File& file, const String& dirName, String& fileName)  {

  String file_path = "";

  file_path += dirName + '/' + fileName;

  if (SD.exists(file_path.c_str()))  {
    SERIAL_DBG("File '")
    SERIAL_DBG(file_path)
    SERIAL_DBG("' already exists...\n")
    return false;
  }

  file = SD.open(file_path.c_str(), FILE_WRITE);
  if (!file)  {
    SERIAL_DBG("Could not create new log file...\n")
    return false;
  }
  file.print("Date:,"); file.println(dirName);
  file.println("Time (HH:MM:SS.CC),Longitude (°),Latitude (°),Altitude (cm),Fix Mode,PDOP,Distance (mm),External temperature (°C)");
  return true;
}

/*
 * @brief: 
 *    Handle log file segmentation and dir/file creation every day.
 * @params:
 *    File : Log file object.
 *    dirName : Log dir name.
 *    fileName : Log file name.
 *    gnss : TinyGPSPlus obj to get current date from.
 *    logSegCountdown : Timer for log segmentation.
 */
void handleLogFile(File& file, String& dirName, String& fileName, TinyGPSPlus& gnss, Metro& logSegCountdown, volatile bool& deviceConnected)  {

  SERIAL_DBG("---> handleLogFile()\n")

  if (SD.mediaPresent()) {
    String currDate;
    dateToStr(gnss.date, currDate);
  
    if ( !file || dirName != currDate)  {
      file.close();
      dirName = currDate;
      timeToStr(gnss.time, fileName);
      fileName += ".csv";
    }
    // Create new log segment
    else if (logSegCountdown.check()) {
      file.close();
      timeToStr(gnss.time, fileName);
      fileName += ".csv";
    }
    SERIAL_DBG("Creating new log dir '")
    SERIAL_DBG(dirName)
    SERIAL_DBG("'...\n")
    newLogDir(dirName);
    SERIAL_DBG("Done.\n")
    
    SERIAL_DBG("Creating and opening new log file '")
    SERIAL_DBG(dirName)
    SERIAL_DBG('/')
    SERIAL_DBG(fileName)
    SERIAL_DBG('\n')
    if (newLogFile(file, dirName, fileName))  {
      logSegCountdown.reset();
      SERIAL_DBG("Done.\n")
    }
    deviceConnected = true;
  }
  else  {
    SERIAL_DBG("No SD card detected...\n")
    deviceConnected = false;
    file.close();
  }
}

/*
 * @brief: 
 *    Generates a string to log into SD card.
 * @params:
 *    log_str : String to store the log.
 *    timeVal : Time value to log.
 *    lng_deg : Longitude in ° to log.
 *    lat_deg : Latitude in ° to log.
 *    elv_m : Longitude in cm to log.
 *    dist_mm : Distance in mm to log.
 *    temp_C : Temperature in °C to log.
 */
void csv_logStr(String& log_str, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const double& elv_m, const char* fixMode, const char* pdop, const float& dist_mm, const float& temp_C)  {

  SERIAL_DBG("\n---> csv_logStr()\n") 
  
  // Inserting GNSS time into log string
  if (timeVal != NO_GNSS_TIME)
    timeValToStr(timeVal, log_str);
  else  {
    SERIAL_DBG("No GNSS time response...\n")
    log_str += "NaN";
  }
  log_str += ',';
  // Inserting GNSS longitude into log string
  if (lng_deg != NO_GNSS_LOCATION)
    log_str += String(lng_deg, LOC_DECIMALS);
  else  {
    SERIAL_DBG("No GNSS location response, check wiring...\n")
    log_str += "NaN";
  }
  log_str += ',';
  // Inserting GNSS latitude into log string
  if (lat_deg != NO_GNSS_LOCATION)
    log_str += String(lat_deg, LOC_DECIMALS);
  else  {
    SERIAL_DBG("No GNSS location response, check wiring...\n")
    log_str += "NaN";
  }
  log_str += ',';
  // Inserting GNSS altitude into log string
  if (elv_m != NO_GNSS_ALTITUDE)
    log_str += String (elv_m, ELV_DECIMALS);
  else  {
    SERIAL_DBG("No GNSS location response, check wiring...\n")
    log_str += "NaN";
  }
  log_str += ',';
  // Inserting GNSS fix mode value
  log_str += String(fixMode);
  log_str += ',';
  // Inserting GNSS PDOP value
  log_str += String(pdop);
  log_str += ',';
  // Inserting distance into log string
  if (dist_mm != DIST_NO_VALUE)
    log_str += String(dist_mm, DIST_DECIMALS);
  else  {
    SERIAL_DBG("No distance response, check wiring...\n")
    log_str += "NaN";
  }
  log_str += ',';
  // Inserting external temperature into log string
  if (extTemp_C != TEMP_NO_VALUE)
    log_str += String(extTemp_C, TEMP_DECIMALS);
  else  {
    SERIAL_DBG("No temperature response, check wiring...\n")
    log_str += "NaN";
  }
}

/*
 * @brief: 
 *    Logs a log string into a file.
 * @params:
 *    fileName: Log file name.
 *    timeVal : Time value to log.
 *    lng_deg : Longitude in ° to log.
 *    lat_deg : Latitude in ° to log.
 *    elv_m : Longitude in cm to log.
 *    dist_mm : Distance in mm to log.
 *    temp_C : Temperature in °C to log.
 */
bool logToSD(File& file, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const double& elv_m, const char* fixMode, const char* pdop, const float& dist_mm, const float& temp_C) {

  String log_str;
  csv_logStr(log_str, timeVal, lng_deg, lat_deg, elv_m, fixMode, pdop, dist_mm, temp_C);
  // Check if log file is open
  if (!file)
    return false;
  // Log into log file
  file.println(log_str);

  return true;
}

/*
 *  @brief: 
 *      Dumps file content on Serial port.
 * @params:
 *    file : File object.
 */
void dumpFileToSerial(File& file) {
  char c;
  // If file is open
  if (file) {
    // Go to file start
    file.seek(0);
    // Read whole file
    SERIAL_DBG("\n------------------\n\n") // Not printed ¯\_(ツ)_/¯
    SERIAL_DBG('\t')
    while (file.available())  {
      c = file.read();
      Serial.print(c);
      if (c == '\n')
        SERIAL_DBG('\t')
    }
    SERIAL_DBG("\n------------\n\n") // Not printed ¯\_(ツ)_/¯
  }
  else
    SERIAL_DBG("---> dumpFileToSerial() : No file open...")  
}

/* ##############   DIGITAL IO  ################ */
/*
 * @brief: 
 *    Watches button state to update enLog.
 * @params:
 *    enLog : Boolean that stores if logging is enabled or not.
 *    connectedDevices : Boolean array to store the connection state of devices.
 */
void handleDigitalIO()  {

  // Check for disconnected devices
  bool deviceDisconnected = false;
  for (uint8_t i = SD_CARD; i <= GNSS_MODULE; i++) {
    deviceDisconnected |= !connectedDevices[i];
  }
  // Logging button
  if (digitalRead(BUTTON_PIN) == LOW)
    enLog = true;
  else
    enLog = false;
    
  // Update logging LED
  if (enLog && logLEDCountdown.check()) {
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
      logLEDCountdown.reset();
  }
  else if (!enLog && noLogLEDCountdown.check()) {
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
      noLogLEDCountdown.reset();
  }
  else if (deviceDisconnected && errorLEDCountdown.check()) {
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
      errorLEDCountdown.reset();
  }
}

/* ##############   ERROR HANDLING  ################ */

void waitForReboot(const String& msg = "")  {

  SERIAL_DBG(msg + '\n');
  SERIAL_DBG("Waiting for reboot...");
  while(1);
}
