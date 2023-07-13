/* --------------------------
 * @inspiration:
 *    clock_logger
 *    GNSS_test
 *   
 * @brief:
 *    This program logs distance and temperature readings into a log file on the SD card 
 *    using a mix of GNSS time and Teensy clock.
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
 *      Teensy RX5       -> DP0601 UART1 B3 (TX)
 *      Teensy Vin (5V)  -> DP0601 UART1 B1 (5V)
 *      Teensy GND       -> DP0601 UART1 B6 (GND)
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
 *      DS18B20 Yellow -> Teensy 21
 *      
 * @ports:
 *      Serial (115200 baud)
 *      Serial5 (115200 baud for DP0601)
 *      Serial4 (9600 baud for URM14)
 * --------------------------
 */
/* ###########################
 * #   GLOBALS DEFINITIONS   #
 * ###########################
 */

/* Serial Ports */
#define URM14_SERIAL  Serial4
#define GNSS_SERIAL   Serial5

/* Sensor acquisition interval */
// min 1s
// max 2¹⁶ms
#define READ_INTERVAL_HOURS 0
#define READ_INTERVAL_MINUTES 0
#define READ_INTERVAL_SECONDS 2
/* GNSS refresh interval */
// Minimal refresh rate to get 20ms GNSS time resolution
#define GNSS_REFRESH_INTERVAL   5000/*µs*/
/* Logging segmentation interval */
// max 2³² - 1
#define LOG_SEG_INTERVAL   6000/*s*/ * 1000/*ms/s*/

/* Teensy pins */
// Logging LED
#define LOG_LED       13
// Modbus
#define DE_PIN        30 // RE = ~DE => Wired to pin 30 as well
// OneWire
#define ONE_WIRE_BUS  21 // Teensy temperature data wire pin
// Disable logging button
// Careful to choose a pin that Snooze can use to wake up the Teensy
#define BUTTON_PIN    2

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
#define   MEASURE_TRIG_BIT      ((uint16_t)0x00 << 3) // Request mesure in passive mode. Unused in auto mode
// URM14 read value when sensor disconnected
#define URM14_DISCONNECTED  UINT16_MAX

/* Dallas temperature sensor */
#define DS18B20_ID   0

/* GNSS Module */
// Time value if GNSS module disconnected
#define NO_GNSS_TIME      24606099 // HH:MM:SS.CC
// Longitude/latitude value if GNSS module disconnected
#define NO_GNSS_LOCATION  91 //°
// Altitude value if GNSS module disconnected
#define NO_GNSS_ALTITUDE  INT32_MAX

/* Devices enum */
enum Devices : uint8_t  {

  SD_CARD = 0,
  DS18B20,
  URM14,
  DP0601,
};

/* Debug */
// Serial debug
// Set to 1 to see debug on Serial port
#if 1
#define SERIAL_DBG(...) {usb.print(__VA_ARGS__);}//;Serial.print("["); Serial.print(__FUNCTION__); Serial.print("(): "); Serial.print(__LINE__); Serial.print(" ] ");}
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
#include <Snooze.h>
#include <TinyGPSPlus.h>
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
void setupSDCard(bool& deviceConnected);
// Log file setup
void handleLogFile(File& file, String& dirName, String& fileName, TinyGPSPlus& gps, Metro& logSegCountdown,   volatile bool& SDConnected);
bool logToSD(File& file, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const int32_t& alt_cm, const uint16_t& dist_mm, const float& temp_C);
void dumpFileToSerial(File& file);
// OneWire communication with DS18B20
void setupDS18B20(DallasTemperature& sensorNetwork, bool& deviceConnected);
// Modbus communication with URM14
void preTrans()  {  digitalWrite(DE_PIN, HIGH); }
void postTrans() {  digitalWrite(DE_PIN, LOW);  }
void setupURM14(ModbusMaster& sensor, const uint16_t& sensorID, const long& sensorBaudrate, void (*preTransCbk)(), void (*postTransCbk)(),  volatile bool& deviceConnected);
// GNSS setup
void setupGNSS(TinyGPSPlus& gps, bool& deviceConnected);
void gnssRefresh(TinyGPSPlus& gps, bool& deviceConnected);
// Sensor reading interrupt
void readSensors();
// Button update
void handleDigitalIO(bool& enLog, const bool* connectedDevices);

/* ##################
 * #    PROGRAM     #
 * ##################
 */
/**** Globals ****/
/* Snooze instances to handle sleep modes */
SnoozeAlarm wakeUpAlarm;
SnoozeDigital logButton;
SnoozeUSBSerial usb;
SnoozeSPI sdCard;
SnoozeBlock config(wakeUpAlarm, logButton, usb, sdCard);
/* Array to store devices connection state */
bool connectedDevices[4] = {false, false, false, false};
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

/* GNSS */
// TinyGPSPlus object to parse NMEA and store location and time
TinyGPSPlus gps;


/* LED timers */
Metro logLEDCountdown = Metro(1500);
Metro noLogLEDCountdown = Metro(600);
Metro errorLEDCountdown = Metro(150);

/* Ohter timers */
// Metro object to manage log segmentation time
Metro logSegCountdown = Metro(LOG_SEG_INTERVAL);

/*
   @brief:
      Sets up SD card, GNSS DP06001, URM14 and DS18B20 sensors
*/
void setup() {

  /* Pin setup */
  pinMode(LOG_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  logButton.pinMode(BUTTON_PIN, INPUT_PULLUP, RISING);
  sdCard.setClockPin(BUILTIN_SDCARD);
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  
  // Turn LED on during setup
  digitalWrite(LOG_LED, HIGH);
  
  /* Serial ports */
  // USB debug Serial port
  Serial.begin(115200);
  // GPS Serial port
  GNSS_SERIAL.begin(115200);

  SERIAL_DBG("#### SETUP ####\n\n")
  
  /* SD card init */
  setupSDCard(connectedDevices[SD_CARD]);
  SERIAL_DBG('\n')
  
  /* Setting up DS18B20 */
  setupDS18B20(sensors, connectedDevices[DS18B20]);
  SERIAL_DBG('\n')

  /* Setting up URM14 */
  setupURM14(urm14, URM14_ID, URM14_BAUDRATE, preTrans, postTrans, connectedDevices[URM14]);
  SERIAL_DBG('\n')

  /* GNSS set up */
  setupGNSS(gps, connectedDevices[DP0601]);
  SERIAL_DBG('\n')

  /* Sensor reading interval config */
  wakeUpAlarm.setRtcTimer(READ_INTERVAL_HOURS, READ_INTERVAL_MINUTES, READ_INTERVAL_SECONDS); 
  
  SERIAL_DBG("\n\n")

  // Turn LED off after setup
  digitalWrite(LOG_LED, LOW);
}

/**** Global variables for loop() ****/
// Variables to store snesor readings
uint32_t time_ms;
double lng_deg, lat_deg;
int32_t alt_cm;
float extTemp_C;
uint16_t dist_mm;

long loopDuration;

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
  
  loopDuration = millis();
  
  handleDigitalIO(enLog, connectedDevices);

  SERIAL_DBG("#### LOOP FUNCTION ####\n\n")

  /* If logging enabled */
  SERIAL_DBG("### DEVICES CONNECTED\n\n")
  SERIAL_DBG("SD CARD :\t")
  SERIAL_DBG(connectedDevices[SD_CARD])
  SERIAL_DBG('\n')
  SERIAL_DBG("DS18B20 :\t")
  SERIAL_DBG(connectedDevices[DS18B20])
  SERIAL_DBG('\n')
  SERIAL_DBG("URM14 :\t\t")
  SERIAL_DBG(connectedDevices[URM14])
  SERIAL_DBG('\n')
  SERIAL_DBG("DP0601 :\t")
  SERIAL_DBG(connectedDevices[DP0601])
  SERIAL_DBG("\n\n")
  
  if (enLog) {
    // Turn off logging LED
    digitalWrite(LOG_LED, LOW);
    
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

    /* Read sensors */
    gnssRefresh(gps, connectedDevices[DP0601]);
    readSensors(time_ms, lng_deg, lat_deg, alt_cm, extTemp_C, dist_mm);

    /* Handling log file management */
    handleLogFile(logFile, logDir, logFileName, gps, logSegCountdown, connectedDevices[SD_CARD]);

    /* Logging into file */
    if ( !logToSD(logFile, time_ms, lng_deg, lat_deg, alt_cm, dist_mm, extTemp_C) )
      SERIAL_DBG("Logging failed...\n") 
        
    /* Dumping log file to Serial */
    if (FILE_DUMP)
      dumpFileToSerial(logFile);

    /* Going to sleep */
    delay(50);
    Snooze.deepSleep(config);
  }
  else  {
    if (logFile)
        logFile.close();
    SERIAL_DBG("Logging disabled...\n")
    /* LED blink if logging disabled */
    digitalWrite(LOG_LED, !digitalRead(LOG_LED));
    delay(500);
  }
  SERIAL_DBG("\n\n")
}

/* ############################
 * #   FUNCTION DEFINITIONS   #
 * ############################
 */
/* ##############   TIMER INTERRUPT    ################ */
/*
 * @brief: Reads sensor values
 * @exec time : long and depends on DS18B20 resolution config
 */
void readSensors(uint32_t& gnssTime, double& gnssLng, double& gnssLat, int32_t& gnssAlt, float& extTemp, uint16_t& dist)  {
  // Interrupt duration
  //long t = millis();
  
  // Modbus communication errors
  uint8_t mbError;
  
  // If logging enabled and logFile open
  if (enLog && logFile) {
    // If buffer not full

    // Interrupt safe GNSS data push into buffers
    //if (gps.time.isUpdated() && gps.location.isUpdated() && gps.altitude.isUpdated()) {
      gnssTime = gps.time.value();
      gnssLng = gps.location.lng();
      gnssLat = gps.location.lat();
      gnssAlt = gps.altitude.value();
      connectedDevices[DP0601] = true;
    /*}
     * TODO : fix gnssRefresh() ->  isUpdated() forced to false
    else {
      gnssTime = NO_GNSS_TIME;
      gnssLng = NO_GNSS_LOCATION;
      gnssLat = NO_GNSS_LOCATION;
      gnssAlt = NO_GNSS_ALTITUDE;
      connectedDevices[DP0601] = false;
    }*/
 
// DS18B20 convertion takes time (depends on sensor resolution config)
    // Read DS18B20 temperature
    sensors.requestTemperatures();
    extTemp = sensors.getTempC(ds18b20_addr);
    // Check for OneWire errors
    if (extTemp == DEVICE_DISCONNECTED_C) {
      SERIAL_DBG("OneWire : DS18B20 disconnected...")
      connectedDevices[DS18B20] = false;
    }
    else
      connectedDevices[DS18B20] = true;
// -------------

    // External compensation : Updade external URM14 temperature register
    if (!TEMP_CPT_ENABLE_BIT && TEMP_CPT_SEL_BIT)  {
      mbError = urm14.writeSingleRegister(URM14_EXT_TEMP_REG, (uint16_t)(extTemp * 10.0));
      // Check for Modbus errors
      if (mbError != ModbusMaster::ku8MBSuccess)  {
        dist = URM14_DISCONNECTED;
        connectedDevices[URM14] = false;
      }
      else
        connectedDevices[URM14] = true;
      }

      // Trigger mode : Set trigger bit to request one measurement
    if (MEASURE_MODE_BIT) {
      mbError = urm14.writeSingleRegister(URM14_CONTROL_REG, urm14_controlBits); //Writes the setting value to the control register
      if (mbError != ModbusMaster::ku8MBSuccess)  {
        dist = URM14_DISCONNECTED;
        connectedDevices[URM14] = false;
      }
      else
        connectedDevices[URM14] = true;
    }
    // Readng distance input register at 0x05
    // Should use readInputRegisters() but somehow doesn't work
    // Trhow ku8MBIllegalDataAddress error (0x02)
    // ToDo : understand error (might be manufacturer who did not follow Modbus standard)
    mbError = urm14.readHoldingRegisters(URM14_DISTANCE_REG, 1);
    // Check for Modbus errors
    if (mbError != ModbusMaster::ku8MBSuccess)  {
      dist = URM14_DISCONNECTED;
      connectedDevices[URM14] = false;
    }
    else  {
      dist = urm14.getResponseBuffer(0);
      connectedDevices[URM14] = true;
    }
  }
    //Serial.println(millis() - t);
}

/* ##############   GNSS    ################ */

void gnssRefresh(TinyGPSPlus& gps, bool& deviceConnected) {

  uint8_t watchdog = millis();
  GNSS_SERIAL.flush();
  while (!gps.time.isUpdated() || !gps.date.isUpdated() || !gps.location.isUpdated() || !gps.altitude.isUpdated())  {
    // If could not update gnsss data in a while    
    if (millis() - watchdog > 700) {
      deviceConnected = false;
      return;
    }
    // Wait for NMEA data
    while(!GNSS_SERIAL.available());
    // Read data
    while (GNSS_SERIAL.available())
       gps.encode(GNSS_SERIAL.read());
  }
  deviceConnected = true;
}

void setupGNSS(TinyGPSPlus& gps, bool& deviceConnected) {

  SERIAL_DBG("Waiting for GNSS signal...\n")
  while (!GNSS_SERIAL.available())  {
    if (millis() > 7000)  {
      SERIAL_DBG("No signal, check GNSS receiver wiring.\n")
      SERIAL_DBG("Waiting for reboot...\n")
      while (1);
    }
  }
  
  SERIAL_DBG("Acquiring GNSS date and time...\n")
  while (gps.date.value() == 0 || gps.time.value() == 0)
    gnssRefresh(gps, deviceConnected);
  SERIAL_DBG("Done.\n")
}

/* ##############   SD CARD    ################ */
/*
   @brief: sets up sd card
*/
void setupSDCard( volatile bool& deviceConnected)  {

  SERIAL_DBG("SD card setup... ")
  // Try to open SD card
  if (!SD.begin(BUILTIN_SDCARD))  {
    SERIAL_DBG("Failed, waiting for reboot...\n");
    while (1);
  }
  else
    SERIAL_DBG("Done.\n")
  deviceConnected = true;
}

/* ##############   FILE MANAGEMENT    ################ */
/*
 * @brief:
 *    convert and write TinyGPSDate date obj into a string.      
 * @params:
 *    gnssDate : TinyGPSDate obj to convert and write
 *    str : String to write date into
 */
void date_to_str(TinyGPSDate& gnssDate, String& str) {
  
  str = "";
  str.concat(gnssDate.year());
  str.concat('_');
  str.concat(gnssDate.month());
  str.concat('_');
  str.concat(gnssDate.day());
}
  
/*
 * @brief:
 *    convert and write TinyGPSTime time obj into a string.      
 * @params:
 *    gnssTime : TinyGPSTime obj to convert and write
 *    str : String to write time into
 */
void time_to_str(TinyGPSTime& gnssTime, String& str) {

  str = "";
  str.concat(gnssTime.hour());
  str.concat('_');
  str.concat(gnssTime.minute());
  str.concat('_');
  str.concat(gnssTime.second());
}

/*
 * @brief:
 *    convert and write time value from TinyGPSPlus obj into a string.    
 * @params:
 *    timeVal : time value to convert and write
 *    str : String to write time into
 */
void timeValToStr(const uint32_t& timeVal, String& str) {

  uint32_t tmp = timeVal;

  str = "";
  str.concat(tmp/1000000);
  str.concat(':');
  tmp %= 1000000;
  str.concat(tmp/10000);
  str.concat(':');
  tmp %= 10000;
  str.concat(tmp/100);
  str.concat('.');
  tmp %= 100;
  str.concat(tmp);
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
  file.print("Date:,"); file.println(dirName);
  file.println("Time (HH:MM:SS.CC),Longitude (°),Latitude (°),Altitude (cm),Distance (mm),External temperature (°C)");
  return true;
}

/*
 * @brief: handle log file segmentation and dir/file creation every day
 * @params:
 *    File : log file object
 *    dirName : log dir name
 *    fileName : log file name
 *    gps : TinyGPSPlus obj to get current date from
 *    logSegCountdown : timer for log segmentation
 */
void handleLogFile(File& file, String& dirName, String& fileName, TinyGPSPlus& gps, Metro& logSegCountdown, volatile bool& SDConnected)  {

  SERIAL_DBG("---> handleLogFile()\n")

  if (SD.mediaPresent()) {
    String currDate;
    date_to_str(gps.date, currDate);
  
    if ( !file || dirName != currDate)  {
      file.close();
      dirName = currDate;
      time_to_str(gps.time, fileName);
      fileName.concat(".csv");
      logSegCountdown.reset();
    }
    // Create new log segment
    else if (logSegCountdown.check()) {
      file.close();
      time_to_str(gps.time, fileName);
      fileName.concat(".csv");
      logSegCountdown.reset();
    }
    SERIAL_DBG("Creating new log dir '")
    SERIAL_DBG(dirName)
    SERIAL_DBG("'...\n")
    new_logDir(dirName);
  
    SERIAL_DBG("Done.\n")
    SERIAL_DBG("Creating and opening new log file '")
    SERIAL_DBG(dirName)
    SERIAL_DBG('/')
    SERIAL_DBG(fileName)
    SERIAL_DBG('\n')
    if (new_logFile(file, dirName, fileName))  {
      logSegCountdown.reset();
      SERIAL_DBG("Done.\n")
    }
    SDConnected = true;
  }
  else  {
    SERIAL_DBG("No SD card detected...\n")
    SDConnected = false;
    file.close();
  }
}

/*
 * @brief: generates a string to log into SD card
 * @params:
 *    log_str : string to store the log
 *    timeVal : time value to log
 *    lng_deg : Longitude in ° to log
 *    lat_deg : Latitude in ° to log
 *    alt_cm : Longitude in cm to log
 *    dist_mm : distance in mm to log
 *    temp_C : temperature in °C to log
 */
void csv_log_string(String& log_str, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const int32_t& alt_cm, const uint16_t& dist_mm, const float& temp_C)  {

  SERIAL_DBG("\n---> csv_log_string()\n") 
  
  // Inserting GNSS time into log string
  if (timeVal != NO_GNSS_TIME)
    timeValToStr(timeVal, log_str);
  else  {
    SERIAL_DBG("No GNSS time response...\n")
    log_str.concat("Nan");
  }
  log_str.concat(',');
  // Inserting GNSS longitude into log string
  if (lng_deg != NO_GNSS_LOCATION)
    log_str.concat(lng_deg);
  else  {
    SERIAL_DBG("No GNSS location response, check wiring...\n")
    log_str.concat("Nan");
  }
  log_str.concat(',');
  // Inserting GNSS latitude into log string
  if (lat_deg != NO_GNSS_LOCATION)
    log_str.concat(lat_deg);
  else  {
    SERIAL_DBG("No GNSS location response, check wiring...\n")
    log_str.concat("Nan");
  }
  log_str.concat(',');
  // Inserting GNSS altitude into log string
  if (alt_cm != NO_GNSS_ALTITUDE)
    log_str.concat(alt_cm);
  else  {
    SERIAL_DBG("No GNSS location response, check wiring...\n")
    log_str.concat("Nan");
  }
  log_str.concat(',');
  // Inserting distance into log string
  if (dist_mm != URM14_DISCONNECTED)
    log_str.concat(dist_mm/10.0);
  else  {
    SERIAL_DBG("No URM14 response, check wiring...\n")
    log_str.concat("Nan");
  }
  log_str.concat(',');
  // Inserting external temperature into log string
  if (extTemp_C != DEVICE_DISCONNECTED_C)
    log_str.concat(extTemp_C);
  else  {
    SERIAL_DBG("No DS18B20 response, check wiring...\n")
    log_str.concat("Nan");
  }
}

/*
 * @brief: logs a log string into a file
 * @params:
 *    fileName: log file name
 *    timeVal : time value to log
 *    lng_deg : Longitude in ° to log
 *    lat_deg : Latitude in ° to log
 *    alt_cm : Longitude in cm to log
 *    dist_mm : distance in mm to log
 *    temp_C : temperature in °C to log
 */
bool logToSD(File& file, const uint32_t& timeVal, const double& lng_deg, const double& lat_deg, const int32_t& alt_cm, const uint16_t& dist_mm, const float& temp_C) {

  String log_str;
  csv_log_string(log_str, timeVal, lng_deg, lat_deg, alt_cm, dist_mm, temp_C);
  // Check if log file is open
  if (!file)
    return false;
  // Log into log file
  file.println(log_str);

  return true;
}

/*
 *  @brief: dumps file content on Serial port
 * @params:
 *    file : file object
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
 * @brief: Sets up the URM14 ultrasoic sensor
 * @params:
 *    sensor: ModbusMaster instance that represents the senter on the bus
 *    sensorID: sensor id on the bus
 *    sensorBaudrate: baudrate to communication with the sensor
 *    preTransCbk: callback called before modbus tansmission to set RS485 interface in transmission mode.
 *    postTransCbk: callback called after modbus tansmission to set RS485 interface back in reception mode.
 *    deviceConnected: bool to store if urm14 is connected or not
 */
void setupURM14(ModbusMaster& sensor, const uint16_t& sensorID, const long& sensorBaudrate, void (*preTransCbk)(), void (*postTransCbk)(), bool& deviceConnected)  {

  // URM14 sensor binary config
  uint16_t fileDumpCountdown = MEASURE_TRIG_BIT | MEASURE_MODE_BIT | TEMP_CPT_ENABLE_BIT | TEMP_CPT_SEL_BIT;
  // Modbus communication errors
  uint8_t mbError;

  // Set Modbus communication
  URM14_SERIAL.begin(sensorBaudrate);
  // Modbus slave id 0x11 on serial port URM14_SERIAL
  sensor.begin(sensorID, URM14_SERIAL);
  // Set pre/post transmision callbacks in ModbusMaster object
  sensor.preTransmission(preTransCbk);
  sensor.postTransmission(postTransCbk);

  // Writing config
  mbError = sensor.writeSingleRegister(URM14_CONTROL_REG, fileDumpCountdown); //Writes the setting value to the control register
  if (mbError != ModbusMaster::ku8MBSuccess) {
    SERIAL_DBG("Modbus : Config could not be written to UMR14 sensor... Check wiring.\n")
    SERIAL_DBG("Waiting for reboot...\n")
    while (1);
  }
  else
    SERIAL_DBG("Modbus : UMR14 sensor found and configured!\n")
  deviceConnected = true;
}

/* ##############   DS18B20   ################ */
/*
 * @brief: watches button state to update enLog
 * @params:
 *    enLog : boolean that stores if logging is enabled or not
 */
void setupDS18B20(DallasTemperature& sensorNetwork, bool& deviceConnected) {

  sensorNetwork.begin();
  // Setting resolution for temperature (the lower, the quicker the sensor responds)
  sensorNetwork.setResolution(11);
  // Store DS18B20 OneWire adress for fast data acquitsition
  sensorNetwork.getAddress(ds18b20_addr, DS18B20_ID);
  // Check for OneWire errors
  if (sensorNetwork.getTempC(ds18b20_addr) == DEVICE_DISCONNECTED_C)  {
    SERIAL_DBG("OneWire : No DS18B20 connected...\n")
    SERIAL_DBG("No external temperature compensation possible.\n")
    if (!TEMP_CPT_ENABLE_BIT && TEMP_CPT_SEL_BIT) {
      SERIAL_DBG("Waiting for reboot...\n")
      while (1);
    }
  }
  else
    SERIAL_DBG("OneWire : DS18B20 found!\n")
  deviceConnected = true;
}

/* ##############   DIGITAL IO  ################ */
/*
 * @brief: watches button state to update enLog
 * @params:
 *    enLog : boolean that stores if logging is enabled or not
 *    connectedDevices : boolean array to store the connection state of devices
 */
void handleDigitalIO(bool& enLog, const bool* connectedDevices)  {
/*
  bool deviceDisconnected =  false;//!connectedDevices[SD_CARD];
  for (uint8_t i = SD_CARD; i <= DP0601; i++) {
    deviceDisconnected |= !connectedDevices[i];
  }
  // Logging LED
  if (enLog) {
    if (deviceDisconnected && errorLEDCountdown.check())
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
    else if (logLEDCountdown.check())
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
  }
  else if (noLogLEDCountdown.check())
      digitalWrite(LOG_LED, !digitalRead(LOG_LED));
*/
  // Logging button
  if (digitalRead(BUTTON_PIN) == 0)
    enLog = true;
  else
    enLog = false;
}
