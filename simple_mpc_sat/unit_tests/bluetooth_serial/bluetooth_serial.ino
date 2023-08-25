/* --------------------------
 * @inspiration:
 *    https://www.aranacorp.com/fr/votre-arduino-communique-avec-le-module-hc-05/
 *    
 * @brief:
 *    This program configures an HC-05 Bluetooth module 
 *    to operate with name Cyclopee and baudrate 115200.
 *    Then listens for AT commands in loop().
 *   
 * @boards :
 *    Teensy 3.5
 *    HC-05 Bluetooth module
 *
 * @wiring:
 *      Teensy RX3      -> HC-05 TX
 *      Teensy TX3      -> HC-05 RX
 *      Teensy 6 Pin    -> HC-05 KEY Pin
 *      Teensy Vin (5V) -> HC-05 VCC
 *      Teensy GND      -> HC-05 GND
 *      
 * @ports:
 *      Serial (115200 baud)
 *      BLUETOOTH_SERIAL (38400 baud for HC-05 conf mode)
 * --------------------------
 */
/* ###########################
 * #   GLOBALS DEFINITIONS   #
 * ###########################
 */
// Serial port for communication with Bluetooth module
#define BLUETOOTH_SERIAL Serial3
// Pin used to set bluetooth module in config mode
#define KEY_PIN 6
// Baudrates
#define INITIAL_COMM_BAUDRATE 9600
#define NEW_COMM_BAUDRATE     9600
#define CONF_BAUDRATE         38400
// Bluetooth config
#define BLUETOOTH_NAME      "Cyclopee"
#define BLUETOOTH_UART_CONF String(NEW_COMM_BAUDRATE) + ",1,0"

/* ###################
 * #    LIBRARIES    #
 * ###################
 */
/* No libraries requiered */

/* ###########################
 * #   FUNCTION PROTOTYPES   #
 * ###########################
 */

bool sendATCommand(const String& cmd, String* pAns = NULL);
bool readBTModuleConfig(String& btName, String& macAddr, String& UARTConf);
bool configureBTModule(const String& btName, const String& UARTConf);

void waitForReboot(const String& msg = "");


/* ##################
 * #    PROGRAM     #
 * ##################
 */

String btName, macAddr, UARTConf;
 
void setup() {
  /// String to store bluetooth module answers
  String ans = "";
  // Pin setup
  pinMode(KEY_PIN, OUTPUT);
  // Serial ports setup
  Serial.begin(115200);
  BLUETOOTH_SERIAL.begin(INITIAL_COMM_BAUDRATE);
  delay(1000);
  digitalWrite(KEY_PIN, HIGH);
  // Checking for bluetooth module presence
  if (!sendATCommand("AT"))
    waitForReboot("No module detected, check wiring...");
  Serial.println("Module detected.");
  
  // Current module config
  Serial.println("Module initial config :");
  if (!readBTModuleConfig(btName, macAddr, UARTConf))
    waitForReboot("Could not read Bluetooth module config.");
  Serial.println("BT name :\t" + btName);
  Serial.println("MAC adress :\t" + macAddr);
  Serial.println("UART config :\t" + UARTConf);
  Serial.println();

   // Configuring Bluetooth module if need be
  Serial.print("Configuring Bluetooth module... ");
  if (!configureBTModule(BLUETOOTH_NAME, BLUETOOTH_UART_CONF))
    waitForReboot("Could not configure Bluetooth module.");
  Serial.println("Done.");
  // Reading module config
  Serial.println("New module config :");
  btName = macAddr = UARTConf = "";
  if (!readBTModuleConfig(btName, macAddr, UARTConf))
    waitForReboot("Could not read Bluetooth module config.");
  Serial.println("BT name :\t" + btName);
  Serial.println("MAC adress :\t" + macAddr);
  Serial.println("UART config :\t" + UARTConf);
  Serial.println();

  digitalWrite(KEY_PIN, LOW);
  sendATCommand("AT+RESET");
  delay(1000);
  digitalWrite(KEY_PIN, HIGH);
  BLUETOOTH_SERIAL.begin(NEW_COMM_BAUDRATE);

  Serial.println("## Waiting for AT commands :");
}

/* Listen fot AT commands */
void loop()
{
  if (Serial.available()) {
    char c = Serial.read();
    Serial.print(c);
    BLUETOOTH_SERIAL.print(c);
  }
  
  if (BLUETOOTH_SERIAL.available()) {
    Serial.print((char)BLUETOOTH_SERIAL.read());
  } 
}

/* ############################
 * #   FUNCTION DEFINITIONS   #
 * ############################
 */

bool sendATCommand(const String& cmd, String* pAns = NULL) {

  char c;
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
    Serial.println("An error occured when writing command '" + cmd + "' : " + ((tmp.length() == 0) ? "No response" : tmp ) + '.');
    return false;
  }
  // Affecting tmp to answer string if provided
  if (pAns)
    *pAns = tmp;

  return true;
}

bool readBTModuleConfig(String& btName, String& macAddr, String& UARTConf) {

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

bool configureBTModule(const String& btName, const String& UARTConf)  {

  if (!sendATCommand("AT+NAME=" + btName))
    return false;
  if (!sendATCommand("AT+UART=" + UARTConf))
    return false;
 
  return true;
}

void waitForReboot(const String& msg = "")  {
  if (msg != "")
    Serial.println(msg);
  Serial.println("Waiting for reboot...");
  while(1);
}
