/* --------------------------
 * @inspiration:
 *    ext_temp_comp_test
 *    SD_test
 *   
 * @brief:
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
/* Current date & time */
#define YEAR      2023
#define MONTH     04
#define DAY       06
#define HOURS     23
#define MINUTES   59
#define SECONDS   45

/* Sensor acquisition interval */
// Check sensor reading interrupt duration before setting the value
// 71s maximum
#define READ_INTERVAL  (uint32_t) 1/*s*/ * 1000000/*µs/s*/
/* Logging segmentation interval */
#define LOG_SEG_INTERVAL  (uint32_t) 120/*s*/ * 1000/*ms/s*/

/* Teensy pins */
// Logging LED
#define LOG_LED       13
// Modbus
#define DE_PIN        30 // RE = ~DE => Wired to pin 30 as well
// OneWire
#define ONE_WIRE_BUS  21 // Teensy temperature data wire pin
// Disable logging button
#define BUTTON_PIN    0

/*  URM14 sensor */
// Sensor baudrate
#define URM14_BAUDRATE 115200
// Sensor id
#define URM14_ID  (uint16_t)0x11
// Sensor registers
#define URM14_ID_REG        (uint16_t)0x02
#define URM14_DISTANCE_REG  (uint16_t)0x05
#define URM14_EXT_TEMP_REG  (uint16_t)0x07
#define URM14_CONTROL_REG   (uint16_t)0x08
// Sensor config register bit values
#define   TEMP_CPT_SEL_BIT      ((uint16_t)0x01)      // Use custom temperature compensation
#define   TEMP_CPT_ENABLE_BIT   ((uint16_t)0x00 << 1) // Enable temperature compensation
#define   MEASURE_MODE_BIT      ((uint16_t)0x00 << 2) // Passive(1)/auto(0) measure mode
#define   MEASURE_TRIG_BIT      ((uint16_t)0x01 << 3) // Request mesure in passive mode. Unused in auto mode
// URM14 read value when sensor disconnected
#define URM14_DISCONNECTED  65535
/* Dallas temperature sensor */
#define DS18B20_ID   0

/* Maximum buffer size */
#define MAX_BUFFER_SIZE  100

/* Debug */
// Serial debug
// Set to 1 to see debug on Serial port
#if 1
#define SERIAL_DBG(...) {Serial.print(__VA_ARGS__);}//;Serial.print("["); Serial.print(__FUNCTION__); Serial.print("(): "); Serial.print(__LINE__); Serial.print(" ] ");}
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
#include <ModbusMaster.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Metro.h>

/* ###########################
 * #   FUNCTION PROTOTYPES   #
 * ###########################
 */
// Sd card setup
void setupSDCard();
// Log file setup
void handleLogFile(File& file, String& dirName, String& fileName, Metro& logSegCountdown, bool& errorFlag);
bool logToSD(File& file, const long& timestamp, const uint16_t& dist_mm, const float& temp_C);
void dumpFileToSerial(File& file);
// Modbus communication with URM14
void preTrans()  {  digitalWrite(DE_PIN, HIGH); }
void postTrans() {  digitalWrite(DE_PIN, LOW);  }
void setupURM14(ModbusMaster& sensor, const uint16_t& sensorID, const long& sensorBaudrate, void (*preTransCbk)(), void (*postTransCbk)());
// OneWire communication with DS18B20
void setupDS18B20(DallasTemperature& sensorNetwork);
// gps_update_timer interrupt
void readSensors();
// Button update
void watchButton(bool& enLog);

/* ##################
 * #    PROGRAM     #
 * ##################
 */
/**** Globals ****/

/* Logging */
// Log file
String logDir = "";
String logFileName = "";
File logFile;
// Log state (enabled/disabled)
bool enLog = false;

/* DS18B20 */
// OneWire bus
OneWire oneWire(ONE_WIRE_BUS);
// Dallas sensor bus
DallasTemperature sensors(&oneWire);
// DS18B20 address
byte ds18b20_addr[8];

/* URM14 */
// ModbusMaster sensor object for URM14
ModbusMaster urm14;
// URM14 config
uint16_t urm14_controlBits = MEASURE_TRIG_BIT | MEASURE_MODE_BIT | TEMP_CPT_ENABLE_BIT | TEMP_CPT_SEL_BIT;

/* Timer interrputs */
IntervalTimer sensorRead_timer;

/* LED timers */
Metro logLEDCountdown = Metro(1500);
Metro noLogLEDCountdown = Metro(600);
Metro errorLEDCountdown = Metro(150);

/* Other timers */
// Metro object to manage log segmentation time
Metro logSegCountdown = Metro(30000);
// Metro object to dump log file every 1s
Metro fileDumpCountdown = Metro(2000);

/*
 * @brief:
 *    Sets up SD card, URM14 and DS18B20 sensors
 */
void setup() {

  /* Set time */
  setTime(HOURS, MINUTES, SECONDS, DAY, MONTH, YEAR);
  
  /* Pin setup */
  pinMode(LOG_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);

  // Turn LED on during setup
  digitalWrite(LOG_LED, HIGH);
  
  /* Serial ports */
  // USB debug Serial port
  Serial.begin(115200);
  // GPS Serial port
  Serial5.begin(115200);

  SERIAL_DBG("#### SETUP ####\n\n")
  
  /* SD card init */
  setupSDCard();
  SERIAL_DBG('\n')
  
  /* Setting up URM14 */
  setupURM14(urm14, URM14_ID, URM14_BAUDRATE, preTrans, postTrans);
  SERIAL_DBG('\n')
  
  /* Setting up DS18B20 */
  setupDS18B20(sensors);
  SERIAL_DBG('\n')
  
  /* Setting up timer interrupts */
  sensorRead_timer.begin(readSensors, READ_INTERVAL);
  sensorRead_timer.priority(250);
  
  SERIAL_DBG("\n\n")

  // Turn LED off after setup
  digitalWrite(LOG_LED, LOW);
}

/* Global variables for loop() */
// System status error flag
bool errorFlag = false;
// Interrupt safe buffers to store values to log
RingBuf <long, MAX_BUFFER_SIZE> time_buf;
RingBuf <float, MAX_BUFFER_SIZE> extTemp_buf;
RingBuf <uint16_t, MAX_BUFFER_SIZE>   dist_buf;

long time_ms;
float extTemp_C;
uint16_t dist_mm;


/*
 * @brief:
 *    If logging enabled (enLog) :
 *      - Prints open log file name;
 *      - Handles the log file (segementation, new day);
 *      - Logs data;
 *      - Dumps log file to Serial (if FILE_DUMP).
 *    Else :
 *      - Closes log file if open;
 *      - Prints "Logging disabled...".
 *    Reads button to update enLog.
 */
void loop() {

  //long t = millis();

  SERIAL_DBG("#### LOOP FUNCTION ####\n\n")

  /* If logging enabled */
  if (enLog) {
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

    /* Handling log file management */
    handleLogFile(logFile, logDir, logFileName, logSegCountdown, errorFlag);

    /* Logging into file */
    if (logFile && !time_buf.isEmpty()) {
      time_buf.pop(time_ms);
      extTemp_buf.pop(extTemp_C);
      dist_buf.pop(dist_mm);
      if ( !logToSD(logFile, time_ms, dist_mm, extTemp_C, errorFlag) )
        SERIAL_DBG("Logging failed...\n")
    }
    /* Dumping log file to Serial */
    if (FILE_DUMP && fileDumpCountdown.check())
      dumpFileToSerial(logFile);
  }
  else  {
    // If log file open then empty log buffer before closing it
    if (logFile)  {
      SERIAL_DBG("Writing remaining buffer values...\n")
      time_buf.pop(time_ms);
      extTemp_buf.pop(extTemp_C);
      dist_buf.pop(dist_mm);
      if ( !time_buf.isEmpty() && !logToSD(logFile, time_ms, dist_mm, extTemp_C, errorFlag) )
        SERIAL_DBG("Logging failed...\n")
      else
        logFile.close();
    }
    else
      SERIAL_DBG("Logging disabled...\n")
  }
  SERIAL_DBG("\n\n")

  // Update enLog state according to button state
  handleDigitalIO(enLog);


  //Serial.println(millis() - t);
}

/* ############################
 * #   FUNCTION DEFINITIONS   #
 * ############################
*/
/* ##############   TIMER INTERRUPT    ################ */
/*
 * @brief: interrupts loop() to read sensors
 * @exec time : long and mostly depends on DS18B20 resolution config
 */
void readSensors()  {
  // Interrupt duration
  //long t = millis();
  
  // Modbus communication errors
  uint8_t mbError;

  // If logging enabled and logFile open
  if (enLog && logFile) {
    // If buffer not full
    if ( !time_buf.isFull() ) {
      
      // Safe interrupt GNSS data push into buffers
      time_buf.lockedPush(millis());

// DS18B20 convertion takes time (depends on sensor resolution config)
      // Read DS18B20 temperature
      sensors.requestTemperatures();
      extTemp_buf.push(sensors.getTempC(ds18b20_addr));
      // Check for OneWire errors
      if (extTemp_buf[extTemp_buf.size()] == DEVICE_DISCONNECTED_C)
        SERIAL_DBG("OneWire : DS18B20 disconnected...")
// --------------
      
      // External compensation : Updade external URM14 temperature register
      if (!TEMP_CPT_ENABLE_BIT && TEMP_CPT_SEL_BIT)  {
        mbError = urm14.writeSingleRegister(URM14_EXT_TEMP_REG, (uint16_t)(extTemp_buf[extTemp_buf.size()] * 10.0));
        // Check for Modbus errors
        if (mbError != ModbusMaster::ku8MBSuccess)
          dist_buf.push(URM14_DISCONNECTED);
      }

      // Reading distance input register at 0x05
      // Should use readInputRegisters() but somehow doesn't work
      // Throws ku8MBIllegalDataAddress error (0x02)
      // ToDo : understand error (might be manufacturer who did not follow Modbus standard)
      mbError = urm14.readHoldingRegisters(URM14_DISTANCE_REG, 1);
      // Check for Modbus errors
      if (mbError != ModbusMaster::ku8MBSuccess)
        dist_buf.push(URM14_DISCONNECTED);
      else
        dist_buf.push(urm14.getResponseBuffer(0));
    }
    else
      SERIAL_DBG("Buffer is full!\n")
  }
  //Serial.println(millis() - t);
}

/* ##############   SD CARD    ################ */
/*
 * @brief: sets up sd card
 */
void setupSDCard()  {

  SERIAL_DBG("SD card setup... ")
  // Try to open SD card
  if (!SD.begin(BUILTIN_SDCARD))  {
    SERIAL_DBG("Failed, waiting for reboot...\n");
    while (1);
  }
  else
    SERIAL_DBG("Done.\n")
  }

/* ##############   FILE MANAGEMENT    ################ */
/*
 * @brief: convert and write timestamp into a string
 * @params:
 *    timestamp : timestamp to convert and write
 *    str : String to write time into
 *    add_ms : should milliseconds be considered in time ?
 */
void timestamp_to_string(const long& timestamp , String& str, bool add_ms) {

  long t = timestamp;
  uint8_t h,m,s;
  
  h = HOURS + t / (3600 * 1000);
  t %= 3600 * 1000;
  m = MINUTES + t / (60 * 1000);
  t %= 60 * 1000;
  s = SECONDS + t / 1000;
  if (s >= 60) {
    m++;
    if (m >= 60)
      h++;
  }
  h %= 24;
  m %= 60;
  s %= 60;
  str = "";
  str.concat(h);
  str.concat(':');
  str.concat(m);
  str.concat(':');
  str.concat(s);
  if (add_ms) {
    str.concat('.');
    t %= 1000;
    if (t < 10)
      str.concat("00");
    else if (t < 100)
      str.concat("0");
    str.concat(t);
  }
}

/*
 * @brief: convert and write date into a string
 * @params:
 *    str : String to write time into
 */
void dateToString(String& str) {

  str = "";
  str.concat(day());
  str.concat('_');
  str.concat(month());
  str.concat('_');
  str.concat(year());
}

/*
 * @brief: create new log dir 'dirName' if it does not already exist
 * @params:
 *    dirName : new dir name
 */
void new_logDir(String& dirName)  {

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
 * @brief: create and open a new log file 'dirName/fileName' if it does not already exist
 * @params:
 *    file: File object
 *    dirName: directory name in which file will be created
 *    fileName : new file name
 */
bool new_logFile(File& file, const String& dirName, String& fileName)  {

  String file_path = "";

  file_path.concat(dirName);
  file_path.concat('/');
  file_path.concat(fileName);

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
  file.println("Time (HH:MM:SS.CC),Distance (mm),External temperature (°C),Read status");
  return true;
}

/*
 * @brief: handle log file segmentation and dir/file creation every day
 * @params:
 *    File : log file object
 *    dirName : log dir name
 *    fileName : log file name
 *    logSegCountdown : timer for log segmentation
 */
void handleLogFile(File& file, String& dirName, String& fileName, Metro& logSegCountdown, bool& errorFlag)  {

   SERIAL_DBG("---> handleLogFile()\n")

  if (SD.mediaPresent())  {
    
    String currDate;
    dateToString(currDate);
    // If no log file open or new day
    if ( !file || dirName != currDate )  {
      file.close();
      dirName = currDate;
      timestamp_to_string(millis(), fileName, false);
      fileName.replace(':', '_');
      fileName.concat(".csv");
      logSegCountdown.reset();
    }
    // Create new log segment
    else if (logSegCountdown.check()) {
      file.close();
      timestamp_to_string(millis(), fileName, false);
      fileName.replace(':', '_');
      fileName.concat(".csv");
      logSegCountdown.reset();
    }
    
    SERIAL_DBG("Creating new log dir '")
    SERIAL_DBG(dirName)
    SERIAL_DBG("'...\n")
    // Create new log dir if dirName changed
    new_logDir(dirName);
  
    SERIAL_DBG("Done.\n")
    SERIAL_DBG("Creating and opening new log file '")
    SERIAL_DBG(dirName)
    SERIAL_DBG('/')
    SERIAL_DBG(fileName)
    SERIAL_DBG('\n')
    // Create new log file if fileName changed
    if (new_logFile(file, dirName, fileName))  {
      logSegCountdown.reset();
      SERIAL_DBG("Done.\n")
    }
    errorFlag = false;
  }
  else  {
    SERIAL_DBG("No SD card detected...\n")
    errorFlag = true;
    file.close();
  }
}

/*
   @brief: generates a string to log into SD card
   @params:
      log_str : string to store the log
      timestamp : timestamp to log
      dist_mm : distance in mm to log
      temp_C : temperature in °C to log
*/
void csv_log_string(String& log_str, const long& timestamp, const uint16_t& dist_mm, const float& temp_C, bool& errorFlag)  {

  SERIAL_DBG("\n---> csv_log_string()\n") 
  
  // Inserting timestamp into log string
  timestamp_to_string(timestamp, log_str, true);
  log_str.concat(',');
  // Inserting distance into log string
  if (dist_mm != URM14_DISCONNECTED)  {
    log_str.concat(dist_mm/10.0);
    errorFlag |= false;
  }
  else  {
    SERIAL_DBG("No URM14 response, check wiring...\n")
    log_str.concat("Nan");
    errorFlag |= true;
  }
  log_str.concat(',');
  // Inserting external temperature into log string
  if (extTemp_C != DEVICE_DISCONNECTED_C) {
    log_str.concat(extTemp_C);
    errorFlag |= false;
  }
  else  {
    SERIAL_DBG("No DS18B20 response, check wiring...\n")
    log_str.concat("Nan");
    errorFlag |= true;
  }
}

/*
   @brief: logs a log string into a file
   @params:
      file: File object used to log into log file
      timestamp: timestamp to convert and log
      ditstance_mm: distance to log
      temp_C: external temperature to log
*/
bool logToSD(File& file, const long& timestamp, const uint16_t& dist_mm, const float& temp_C, bool& errorFlag) {

  String log_str;
  csv_log_string(log_str, timestamp, dist_mm, temp_C, errorFlag);
  // Check if log file is open
  if (!file)
    return false;
  // Log into log file
  file.println(log_str);

  return true;
}

/*
   @brief: dumps file content on Serial port
   @params:
      file : file object
      dirName : dir name in which is located the file
      fileName: name of the file to dump
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


/* ##############   URM14  ################ */
/*
   @brief: Sets up the URM14 ultrasoic sensor
   @params:
      sensor: ModbusMaster instance that represents the senter on the bus
      sensorID: sensor id on the bus
      sensorBaudrate: baudrate to communication with the sensor
      preTransCbk: callback called before modbus tansmission to set RS485 interface in transmission mode.
      postTransCbk: callback called after modbus tansmission to set RS485 interface back in reception mode.
*/
void setupURM14(ModbusMaster& sensor, const uint16_t& sensorID, const long& sensorBaudrate, void (*preTransCbk)(), void (*postTransCbk)())  {

  // URM14 sensor binary config
  uint16_t fileDumpCountdown = MEASURE_TRIG_BIT | MEASURE_MODE_BIT | TEMP_CPT_ENABLE_BIT | TEMP_CPT_SEL_BIT;
  // Modbus communication errors
  uint8_t mbError;

  // Set Modbus communication
  Serial4.begin(sensorBaudrate);
  // Modbus slave id 0x11 on serial port Serial4
  sensor.begin(sensorID, Serial4);
  // Set pre/post transmision callbacks in ModbusMaster object
  sensor.preTransmission(preTransCbk);
  sensor.postTransmission(postTransCbk);

  // Writing config
  mbError = sensor.writeSingleRegister(URM14_CONTROL_REG, fileDumpCountdown);
  // Check for Modbus errors
  if (mbError != ModbusMaster::ku8MBSuccess) {
    SERIAL_DBG("Modbus : Config could not be written to UMR14 sensor... Check wiring.\n")
    SERIAL_DBG("Waiting for reboot...\n")
    while (1);
  }
  else
    SERIAL_DBG("Modbus : UMR14 sensor found and configured!\n")
}

/* ##############   DS18B20   ################ */
/*
   @brief: Sets up the DS18B20 temperature sensor
   @params:
      sensorNetwork: DallasTemperature instance that represents a DallasTemperature sensor network
*/
void setupDS18B20(DallasTemperature& sensorNetwork) {

  sensorNetwork.begin();
  // Setting resolution for temperature (the lower, the quicker the sensor responds)
  sensorNetwork.setResolution(10);
  // Store DS18B20 OneWire adress for fast data acquitsition
  sensorNetwork.getAddress(ds18b20_addr, DS18B20_ID);
  // Check for OneWire errors
  if (sensorNetwork.getTempC(ds18b20_addr) == DEVICE_DISCONNECTED_C)  {
    SERIAL_DBG("OneWire : No DS18B20 connected...\n")
    SERIAL_DBG("No external temperature compensation possible.\n")
    // If external compensation activated, request reboot
    if (!TEMP_CPT_ENABLE_BIT && TEMP_CPT_SEL_BIT) {
      SERIAL_DBG("Waiting for reboot...\n")
      while (1);
    }
  }
  else
    SERIAL_DBG("OneWire : DS18B20 found!\n")
}

/* ##############   DIGITAL IO  ################ */
/*
 * @brief: watches button state to update enLog
 * @params:
 *    enLog : boolean that stores if logging is enabled or not
 */
void handleDigitalIO(bool& enLog)  {

  // Logging LED
  if (!errorFlag) { 
    if (!enLog && noLogLEDCountdown.check())
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
    else if (enLog && logLEDCountdown.check())
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
  }
  else
    if (errorLEDCountdown.check())
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
      
  // Logging button
  if (digitalRead(BUTTON_PIN) == 0)
    enLog = true;
  else
    enLog = false;
}
