/* ------------------------------------------------------
 *
 * @insipration:
 *     http://arduiniana.org/libraries/tinygpsplus/
 * 
 * @brief:  This program feeds TinyGPSPlus object with Serial5 buffer data
 *          and dumps updated GNSS time on USB (Serial) port.
 *          
 * @board:
 *    Teensy 3.5
 * 
 * @GNSS module:
 *    Drotek DP0601 RTK GNSS (XL F9P)  
 *
 * @wiring:
 *      Teensy RX2      ->  DP0601 UART1 B3 (TX) 
 *      Teensy Vin (5V) ->  DP0601 UART1 B1 (5V)
 *      Teensy GND      ->  DP0601 UART1 B6 (GND)
 *      
 * @ports:
 *      Serial (115200 baud)
 *      Serial5 (115200 baud for DP0601)
 *      
 * ------------------------------------------------------
 */
/* ############################
 * #    GLOBAL DEFINITIONS    #
 * ############################
 */
/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <TinyGPSPlus.h>

/* #################
 * #    PROGRAM    #
 * #################
 */
// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;
 
void setup() {
  // Setup Serial ports for commmunication
  Serial.begin(115200);
  Serial5.begin(115200);
}

void loop() {
  // While Serial5 buffer is not empty
  while (Serial5.available())
    // Read buffer
    gps.encode(Serial5.read());

  // Dump GNSS time (UTC)
  Serial.print(gps.time.hour());
  Serial.print(':');
  Serial.print(gps.time.minute());
  Serial.print(':');
  Serial.print(gps.time.second());
  Serial.print('.');
  Serial.print(gps.time.centisecond());
  Serial.println();
}
