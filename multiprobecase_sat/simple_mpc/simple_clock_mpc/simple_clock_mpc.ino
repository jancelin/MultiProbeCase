/* --------------------------
 * @inspiration:
 *    ext_temp_comp_test
 *    SD_test
 *   
 * @brief:z
 *    This program logs distance and temperature readings into a log file on the SD card.
 *    Log file segmentation and new day file creation are handled.
 *   
 * @board :
 *    Teensy 3.5
 *
 * @RS485 interface :
 *    Mikroe RS485 click 2 (MAX3471)
 *
 * @wiring:
 *      Teensy TX4  -> RS485 RX
 *      Teensy RX4  -> RS485 TX 
 *      Teensy 30   -> RS485 DE 
 *      Teensy 3V3  -> RS485 3V3
 *      Teensy GND  -> RS485 GND
 *      URM14 Blue  -> RS485 B
 *      URM14 White -> RS485 A
 *      URM14 Brown -> 12V                    
 *      URM14 Black -> 0V
 *      DS18B20 Red -> Teensy 3V3
 *      DS18B20 Black -> Teensy GND
 *      DS18B20 Yellow -> Teensy 21
 *      
 * @ports:
 *      Serial (115200 baud)
 *      Serial4 (9600 baud configured in URM14)
 * --------------------------
 */
/* ##########################
 * #   GLOBAL DEFINITIONS   #
 * ##########################
 */
/************** SERIAL PORTS *****************/

/* Current date & time */
#define YEAR      2023
#define MONTH     7
#define DAY       20
#define HOURS     00
#define MINUTES   00
#define SECONDS   00

/************** TIMER INTERRUPTS INTERVALS *****************/
// Sensor acquisition interval
// Check sensor reading interrupt duration before setting the value
// 71s maximum
#define READ_INTERVAL       1/*s*/ * 1000000/*µs/s*/
// Logging segmentation interval
#define LOG_SEG_INTERVAL    1000/*ms/s*/ * 10/*s*///* 60/*s/min*/ * 60/*min/h*/
// Digital I.O. refresh interval
#define IO_REFRESH_INTERVAL 50/*ms*/ * 1000/*µs/ms*/

/************** TEENSY PINS *****************/
// Logging LED
#define LOG_LED       13
// Disable logging button
#define BUTTON_PIN    39

/************** DATA NUMBER OF DECIMALS *****************/
// Temperature
#define TEMP_DECIMALS 3
// Turbidity
#define TURB_DECIMALS 1
// Conductivity
#define COND_DECIMALS 2

/************** BUFFERS *****************/
// Maximum buffer size
#define MAX_BUFFER_SIZE  100

/************** ENUMS *****************/
// Connected devices
enum Devices : uint8_t  {

  SD_CARD = 0,
  TEMPERATURE,
  TURBIDITY,
  CONDUCTIVITY
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
#include <TimeLib.h>
#include <SD.h>
#include <Metro.h>

/* ###########################
 * #   FUNCTION PROTOTYPES   #
 * ###########################
 */
// Error handling
void waitForReboot(const String& msg);
// Sd card setu
void setupSDCard(volatile bool& deviceConnected);
// Log file setup
void handleLogFile(File& file, String& dirName, String& fileName, Metro& logSegCountdown, volatile bool& deviceConnected);
bool logToSD(File& file, const long& timestamp, const float& rawTurb, const float& turb, const float& rawCond, const float& cond, const float& temp_C);
void dumpFileToSerial(File& file);
// Sensor reading interrupt
void readSensors();
// Digital IO update interrupt
void handleDigitalIO();

/* ######################
 * #   SENSOR MODULES   #
 * ######################
 */
#include "DS18B20_temperature.h"
#include "Gravity_turbidity.h"
#include "Gravity_EC10_conductivity.h"

/* ##################
 * #    PROGRAM     #
 * ##################
 */
/************** GLOBALS *****************/
// Array to store devices connection state
volatile bool connectedDevices[4] = {false, false, false};

// LOGGING
// Log file
String logDir = "";
String logFileName = "";
File logFile;
// Log state (enabled/disabled)
volatile bool enLog = false;

// Timer interrputs
IntervalTimer sensorRead_timer, ioRefresh_timer;

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
 *    Sets up SD card, distance and temperature sensors.
 */
void setup() {

  // Pin setup
  pinMode(LOG_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Turn logging LED on during setup
  digitalWrite(LOG_LED, HIGH);

  // USB debug Serial port
  Serial.begin(115200);
  
  SERIAL_DBG("#### SETUP ####\n\n")
  
  // SD card init
  setupSDCard(connectedDevices[SD_CARD]);
  SERIAL_DBG('\n')
  // Setting up temperature sensor
  setupTempSensor(connectedDevices[TEMPERATURE]);
  SERIAL_DBG('\n')
  // Setting up turbidity sensor
  setupTurbSensor(connectedDevices[TURBIDITY]);
  SERIAL_DBG('\n')
  // Setting up conductivity sensor
  setupCondSensor(connectedDevices[CONDUCTIVITY]);
  SERIAL_DBG('\n')
  
  // Setting time
  setTime(HOURS, MINUTES, SECONDS, DAY, MONTH, YEAR);
  // Setting up timer interrupts
  sensorRead_timer.begin(readSensors, READ_INTERVAL);
  sensorRead_timer.priority(200);
  ioRefresh_timer.begin(handleDigitalIO, IO_REFRESH_INTERVAL);
  ioRefresh_timer.priority(190);
  
  SERIAL_DBG("\n\n")

  // Turn log LED off after setup
  digitalWrite(LOG_LED, LOW);
}

/************** LOOP() GLOBAL VARS *****************/
// Buffers to store values to log
RingBuf <long, MAX_BUFFER_SIZE> timestamp_buf;
RingBuf <float, MAX_BUFFER_SIZE> temp_buf, rawTurb_buf, turb_buf, rawCond_buf, cond_buf;

// Variables to store buffer readings
long timestamp;
float temp_C, rawTurb, turb, rawCond, cond;

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
  //long t = millis();

  SERIAL_DBG("#### LOOP FUNCTION ####\n\n")

  // Print conected devices state
  SERIAL_DBG("### DEVICES CONNECTED\n\n")
  SERIAL_DBG("SD CARD :\t")
  SERIAL_DBG(connectedDevices[SD_CARD])
  SERIAL_DBG('\n')
  SERIAL_DBG("TEMPERATURE :\t")
  SERIAL_DBG(connectedDevices[TEMPERATURE])
  SERIAL_DBG('\n')
  SERIAL_DBG("TURBIDITY :\t")
  SERIAL_DBG(connectedDevices[TURBIDITY])
  SERIAL_DBG('\n')
  SERIAL_DBG("CONDUCTIVITY :\t")
  SERIAL_DBG(connectedDevices[CONDUCTIVITY])
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
    }
    else
      SERIAL_DBG("No log file open.\n")
    SERIAL_DBG("\n###\n")

    // Handling log file management
    handleLogFile(logFile, logDir, logFileName, logSegCountdown, connectedDevices[SD_CARD]);

    // Logging into file
    if (logFile && !timestamp_buf.isEmpty()) {
      // Create function for this
      timestamp_buf.pop(timestamp);
      temp_buf.pop(temp_C);
      rawTurb_buf.pop(rawTurb);
      turb_buf.pop(turb);
      rawCond_buf.pop(rawCond);
      cond_buf.pop(cond);
      // -----------------
      if ( !logToSD(logFile, timestamp, rawTurb, turb, rawCond, cond, temp_C) )
        SERIAL_DBG("Logging failed...\n")
    }    
    // Dumping log file to Serial
    if (FILE_DUMP && fileDumpCountdown.check())
      dumpFileToSerial(logFile);
  }
  else  {
    // If log file open then empty buffers into it before closing
    if (logFile)  {
      SERIAL_DBG("Writing remaining buffer values...\n")
      // Create function for this
      timestamp_buf.pop(timestamp);
      temp_buf.pop(temp_C);
      rawTurb_buf.pop(rawTurb);
      turb_buf.pop(turb);
      rawCond_buf.pop(rawCond);
      cond_buf.pop(cond);
      // ---------------------
      if ( !timestamp_buf.isEmpty() && !logToSD(logFile, timestamp, rawTurb, turb, rawCond, cond, temp_C) )
        SERIAL_DBG("Logging failed...\n")
      else
        logFile.close();
    }
    else
      SERIAL_DBG("Logging disabled...\n")
  }
  SERIAL_DBG("\n\n")

  // Loop execution time
  //Serial.println(millis() - t);
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
  if (enLog && logFile) {
    // If buffer not full
    if ( !timestamp_buf.isFull() ) {

      timestamp_buf.lockedPush(millis());

      // Acquire temperature
      temp_buf.push(readTemperature(connectedDevices[TEMPERATURE]));
      // Acquire raw turbidity
      rawTurb_buf.push(readRawTurbidity(connectedDevices[TURBIDITY]));
      // Acquire turbidity
      turb_buf.push(computeTurbidity());
      // Acquire raw conductivity
      rawCond_buf.push(readRawConductivity(connectedDevices[CONDUCTIVITY]));
      // Acquire conductivity
      cond_buf.push(computeConductivity(temp_buf[temp_buf.size()-1]));
    }
    else
      SERIAL_DBG("Buffer is full!\n")
  }

  // Interrupt execution time
  //Serial.println(millis() - t);
}

/* ##############   SD CARD    ################ */
/*
 * @brief: 
 *    Sets up sd card.
 * @params:
 *    deviceConnected: Boolean to store device connection state.
 */
void setupSDCard( volatile bool& deviceConnected)  {

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
 *    Convert and write timestamp into a string.
 * @params:
 *    timestamp : Timestamp to convert and write.
 *    str : String to write time into.
 *    add_ms : Should milliseconds be considered in time ?
 */
void timestampToStr(const long& timestamp , String& str, bool add_ms) {

  uint16_t h, m, s, ms;

  // Convert timestamp to different time units
  ms = timestamp%1000;
  s = SECONDS + timestamp/1000;
  m = MINUTES + s/60;
  h = HOURS + m/60;
  // Get hours, minutes, seconds and miliseconds values from converted values
  s%=60;
  m%=60;
  h%=24;
  // Generate the time String
  str =  ((h < 10) ? '0' + String(h) : String(h)) + ':' +
          ((m < 10) ? '0' + String(m) : String(m)) + ':' +
          ((s < 10) ? '0' + String(s) : String(s));
  // Add miliseconds
  if (add_ms) {
    str += '.';
    if (ms < 10)
      str += "00";
    else if (ms < 100)
      str += '0';
    str += String(ms);
  }
}

/*
 * @brief: 
 *    Convert and write date into a string.
 * @params:
 *    str : String to write time into.
 */
void dateToStr(String& str) {
  
  str = String(year()) + '_' + 
        ( (month() < 10) ? '0' + String(month()) : String(month()) ) + '_' +
        ( (day() < 10) ? '0' + String(day()) : String(day()) );
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
  file.println("Time (HH:MM:SS.CCC),Raw Turbidity (V),Turbidity (NTU),Raw Conductivity (mV),Conductivity (mS/cm),Temperature (°C)");
  return true;
}


/*
 * @brief: 
 *    Handle log file segmentation and dir/file creation every day.
 * @params:
 *    File : Log file object.
 *    dirName : Log dir name.
 *    fileName : Log file name.
 *    logSegCountdown : Timer for log segmentation.
 */
void handleLogFile(File& file, String& dirName, String& fileName, Metro& logSegCountdown, volatile bool& deviceConnected)  {

  SERIAL_DBG("---> handleLogFile()\n")

  if (SD.mediaPresent()) {
    String currDate;
    dateToStr(currDate);
  
    if ( !file || dirName != currDate)  {
      file.close();
      dirName = currDate;
      timestampToStr(millis(), fileName, false);
      fileName.replace(':', '_');
      fileName += ".csv";
    }
    // Create new log segment
    else if (logSegCountdown.check()) {
      file.close();
      timestampToStr(millis(), fileName, false);
      fileName.replace(':', '_');
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
 *    Generates a csv string to log into SD card.
 * @params:
 *    log_str : String to store the log.
 *    timestamp : Timestamp to log.
 *    turb : Distance in mm to log.
 *    temp_C : Temperature in °C to log.
 */
void csv_logString(String& log_str, const long& timestamp, const float& rawTurb, const float& turb, const float& rawCond, const float& cond, const float& temp_C)  {
  
  // Inserting timestamp into log string
  timestampToStr(timestamp, log_str, true);
  log_str += ',';
  // Inserting raw turbidity into log string
  log_str += String(rawTurb, 3) + ',';
  // Inserting turbidity into log string
  if (turb != TURB_NO_VALUE)
    log_str += String(turb, TURB_DECIMALS);
  else  {
    SERIAL_DBG("No turbidity response, check wiring...\n")
    log_str += "NaN";
  }
  log_str += ',';
  // Inserting raw conductivity into log string
  log_str += String(rawCond, 3) + ',';
  // Inserting turbidity into log string
  if (cond != EC_NO_VALUE)
    log_str += String(cond, COND_DECIMALS);
  else  {
    SERIAL_DBG("No conductivity response, check wiring...\n")
    log_str += "NaN";
  }
  log_str += ',';
  
  // Inserting external temperature into log string
  if (temp_C != TEMP_NO_VALUE)
    log_str += String(temp_C, TEMP_DECIMALS);
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
 *    timestamp : Timestamp to log.
 *    turb : Distance in mm to log.
 *    temp_C : Temperature in °C to log.
 */
bool logToSD(File& file, const long& timestamp, const float& rawTurb, const float& turb, const float& rawCond, const float& cond, const float& temp_C) {

  String log_str;
  csv_logString(log_str, timestamp, rawTurb, turb, rawCond, cond, temp_C);
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
  for (uint8_t i = SD_CARD; i <= CONDUCTIVITY; i++) {
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
