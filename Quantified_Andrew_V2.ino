#include <SD.h>
#include <Wire.h>
#include "RTClib.h"


#define LOG_INTERVAL 1000 // mills between entries (reduce to take more/faster data)

#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL 1 // echo data to serial port
#define WAIT_TO_START 0 // Wait for serial input in setup()

// the digital pins that connect to the LEDs
#define redLEDpin 2
#define greenLEDpin 3

// The analog pins that connect to the sensors
#define photocellPin 0 // analog 0
#define tempPin 1 // analog 1
#define BANDGAPREF 14 // special indicator that we want to measure the bandgap

#define aref_voltage 3.3 // we tie 3.3V to ARef and measure it with a multimeter!
#define bandgap_voltage 1.1 // this is not super guaranteed but its not -too- off

RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

const int howPin = 4;
const int wherePin = 6;
const int whyPin = 8;
int howNumber = 0, whereNumber = 0, whyNumber = 0;
int how, where, why;

// the logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);

  // red LED indicates error
  digitalWrite(redLEDpin, HIGH);

  while(1);
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println();

  // use debugging LEDs
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);
  pinMode(howPin, INPUT);
  pinMode(wherePin, INPUT);
  pinMode(whyPin, INPUT);

#if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");

  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break; // leave the loop!
    }
  }

  if (! logfile) {
    error("couldnt create file");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif //ECHO_TO_SERIAL
  }


  logfile.println("millis,stamp,datetime,how,where,why");
#if ECHO_TO_SERIAL
  Serial.println("millis,stamp,datetime,how,where,why");
#endif //ECHO_TO_SERIAL

  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}

void loop(void)
{
  DateTime now;
  readSwitches();
  if(how == LOW || why == LOW || where == LOW){
    digitalWrite(greenLEDpin, HIGH);
    if(how == LOW) howNumber++;
    if(why == LOW) whyNumber++;
    if(where == LOW) whereNumber++;
    // log milliseconds since starting
    uint32_t m = millis();
    logfile.print(m); // milliseconds since start
    logfile.print(", ");
#if ECHO_TO_SERIAL
    Serial.print(m); // milliseconds since start
    Serial.print(", ");
#endif

    // fetch the time
    now = RTC.now();
    // log time
    logfile.print(now.unixtime()); // seconds since 1/1/1970
    logfile.print(", ");
    logfile.print('"');
    logfile.print(now.year(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.day(), DEC);
    logfile.print(" ");
    logfile.print(now.hour(), DEC);
    logfile.print(":");
    logfile.print(now.minute(), DEC);
    logfile.print(":");
    logfile.print(now.second(), DEC);
    logfile.print('"');
#if ECHO_TO_SERIAL
    Serial.print(now.unixtime()); // seconds since 1/1/1970
    Serial.print(", ");
    Serial.print('"');
    Serial.print(now.year(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(":");
    Serial.print(now.minute(), DEC);
    Serial.print(":");
    Serial.print(now.second(), DEC);
    Serial.print('"');
#endif //ECHO_TO_SERIAL
    logfile.print(", ");
    logfile.print(howNumber);
    logfile.print(", ");
    logfile.print(whereNumber);
    logfile.print(", ");
    logfile.print(whyNumber);


#if ECHO_TO_SERIAL
    Serial.print(", ");
    Serial.print(howNumber);
    Serial.print(", ");
    Serial.print(whereNumber);
    Serial.print(", ");
    Serial.print(whyNumber);

#endif //ECHO_TO_SERIAL

    logfile.print(", ");

#if ECHO_TO_SERIAL
    Serial.print(", ");

#endif // ECHO_TO_SERIAL

    logfile.println();
#if ECHO_TO_SERIAL
    Serial.println();
#endif // ECHO_TO_SERIAL
    delay(700);
    digitalWrite(greenLEDpin, LOW);
  }
  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();

  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  digitalWrite(redLEDpin, LOW);
  delay(20);
}

void readSwitches(){
  why = digitalRead(whyPin);
  how = digitalRead(howPin);
  where = digitalRead(wherePin);
}

