/* --------------------------
 * @inspiration:
 *    SD library :
 *      Example ReadWrite.ino
 *      
 *  @brief:
 *    This program opens, writes and reads what was written in 
 *    a file on an SD card plugged in the built in SD card slot.
 *    
 *  @board:
 *    Teensy 3.5
 * --------------------------
 */
/* ################
 * #  LIBRARIES   #
 * ################
 */
#include <SD.h>

/* ################
 * #  PROGRAM     #
 * ################
 */
 
File file;
char c;
int i;

void setup() {

  Serial.begin(115200);

  Serial.println("#### SD card test #####");
  
  /* SD card init */
  Serial.print("SD card init... ");
  // Try to open SD card
  if (!SD.begin(BUILTIN_SDCARD))  {
    Serial.println("Failed, please reboot.");
    while(1);
  }
  else
    Serial.println("Done.");
    
  /* Create/Try to open 'test.txt' file for writing */ 
  Serial.print("Openning file 'test.txt' for writing... ");
  file = SD.open("test.txt", FILE_WRITE);
  if (!file) {
   Serial.println("Failed, please reboot."); 
   while(1);
  }
  else
    Serial.println("Open.");
    
  /* Writing to file */
  Serial.print("Writing to 'test.txt'... ");
  if (file)
    file.println("You made it here, congrats!");
  Serial.println("Done.");
  
  /* Closing and reopening file for reading */
  Serial.print("Reopening 'test.txt' for reading... ");
  file.close();
  file = SD.open("test.txt", FILE_READ);
  if (!file) {
   Serial.println("Failed, please reboot."); 
   while(1);
  }
  else
    Serial.println("Done.");
    
  /* Read file */
  Serial.println("Reading 'test.txt'...");
  Serial.print("test.txt :\n\t");
  while (file.available())  {
    c = file.read();
    Serial.write(c);
    if (c == '\n')
      Serial.print('\t');
      
  }
  Serial.println("EOF");
  
  /* Closing file */
  Serial.print("Closing test.txt'... ");
  file.close();
  Serial.println("Done, exiting program...");
  Serial.println("END");
}

void loop() {
  // Not used here
} 
