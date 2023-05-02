/* --------------------------
 * @inspiration:
 *    https://www.aranacorp.com/fr/votre-arduino-communique-avec-le-module-hc-05/
 *    
 * @brief:
 *    This program configures an HC-05 Bluetooth module 
 *    to operate with name Cyclopee and baudrate 115200.
 *    The listens for AT commands in loop().
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
 *      Serial3 (34800 baud for HC-05 conf mode)
 * --------------------------
 */
/* ###########################
 * #   GLOBALS DEFINITIONS   #
 * ###########################
 */
// Pin used to set bluetooth module in config mode
#define KEY_PIN 6
// Baudrates
#define DEFAULT_COMM_BAUDRATE 9600
#define COMM_BAUDRATE         115200
#define CONF_BAUDRATE         38400

/* ###################
 * #    LIBRARIES    #
 * ###################
 */
/* No libraries requiered */

/* ##################
 * #    PROGRAM     #
 * ##################
 */
void setup() {
  /// String to store bluetooth module answers
  String ans = "";
  // Pin setup
  pinMode(KEY_PIN, OUTPUT);
  digitalWrite(KEY_PIN, HIGH);
  // Serial ports setup
  Serial.begin(115200);
  Serial3.begin(CONF_BAUDRATE);

  // Checking for bluetooth module presence
  Serial3.println("AT");
  delay(100);
  while (Serial3.available())
   ans.concat((char)Serial3.read()); 
  if (ans != "OK\r\n")  {
    Serial.println("Bluetooth module : No module detected, check wiring...");
    Serial.println("Waiting for reboot...");
    return;
    while(1);
  }
  Serial.println("Bluetooth module : Module detected.");
  ans = "";
  
  /* Current Module Config */
  Serial.println("Current module config :");
  // Getting current bluetooth module name
  Serial3.println("AT+NAME?");
  delay(100);
  while (Serial3.available() && ans != '\n')  {
    ans = Serial3.read();
    Serial.print(ans);
  }
  ans = "";
  // Flush Serial3 buffer
  while(Serial3.available())
    Serial3.read();
  // Getting current bluetooth module communication baudrate
  Serial3.println("AT+UART?");
  delay(100);
  while (Serial3.available() && ans != '\n')  {
    ans = Serial3.read();
    Serial.print(ans);
  }
  ans = "";
  // Flush Serial3 buffer
  while(Serial3.available())
    Serial3.read();
  Serial.println();

  /* Setting New Conf */
  Serial.println("Writing new config...");
  // Setting device bluetooth name
  Serial3.println("AT+NAME=Cyclopee");
  delay(100);
  while (Serial3.available())
    ans.concat((char)Serial3.read());
  if (ans != "OK\r\n")  {
    Serial.println("Bluetooth module : Could not set device name...");
    Serial.println("Waiting for reboot...");
    return;
    while(1);
  }
  Serial.println("Bluetooth module : Device name set to 'Cyclopee'.");
  ans = "";
  // Setting bluetooth communication baudrate
  Serial3.println("AT+UART=115200,1,0");
  delay(100);
  while (Serial3.available())
    ans.concat((char)Serial3.read());
  if (ans != "OK\r\n")  {
    Serial.println("Bluetooth module : Could not set communication baudrate...");
    Serial.println("Waiting for reboot...");
    return;
    while(1);
  }
  Serial.println("Bluetooth module : Communication baudrate set to 115200 baud.");
  ans = "";

  // Configuration end
  Serial.println("\nWrite AT commands :");
}

/* Listen fot AT commands */
void loop()
{
  if (Serial.available()) {
    char c = Serial.read();
    Serial.print(c);
    Serial3.print(c);
  }
  
  if (Serial3.available()) {
    Serial.print((char)Serial3.read());
  } 
}
