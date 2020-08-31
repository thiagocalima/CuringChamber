// Thiago Lima
/*
TODO List

Adjust the write function to register the set values for temperature and humidity (make it store a string instead?)
Function to write logs (reboot, errors, etc)
Function to parse parameters from SDCard (https://forum.arduino.cc/index.php?topic=210904.0)
Solve problem of seconds not being sent properly when calling functions
Adjust the error message onto the debugSDcard function
Publish the code on GitHub

*/



#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

#define DHTPIN1 A1
#define DHTPIN2 A2
#define DHTTYPE DHT11
#define SDCSPin 53

DHT sensor1(DHTPIN1, DHTTYPE);
DHT sensor2(DHTPIN2, DHTTYPE);

RTC_DS1307 rtc;

const int Device1 = 6;
const int Device2 = 7;
const int Device3 = 8;
const int Device4 = 9;

const int Pot = A15;

int sampleRate = 1000;
unsigned long timer = 0;

float minTemperature = 14.0;
float maxTemperature = 15.0;

float minHumidity = 80.0;
float maxHumidity = 90.0;

String probesFileName;

File myFile;
const int chipSelect = SDCSPin;

//Variables for de SDCard debug
Sd2Card card;
SdVolume volume;
SdFile root;


//Mapping functino for sensor calibration
//double map(double x, double in_min, double in_max, double out_min, double out_max) {
//  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
//}


void throwError(DateTime now, String message) {
  message = getTimestamp(now) + " " + message;
  Serial.println(message);
}


String getTimestamp(DateTime timestamp) {
  char message[120];

  int Year = timestamp.year();
  int Month = timestamp.month();
  int Day = timestamp.day();
  int Hour = timestamp.hour();
  int Minute = timestamp.minute();
  int Second= timestamp.second();

  sprintf(message, "%d-%d-%d %02d:%02d:%02d", Day,Month,Year,Hour,Minute,Second);
  
  return message;
}


String getDateTimeString(DateTime timestamp) {
  char message[120];

  int Year = timestamp.year();
  int Month = timestamp.month();
  int Day = timestamp.day();
  int Hour = timestamp.hour();
  int Minute = timestamp.minute();
  int Second= timestamp.second();

  sprintf(message, "%d%d%d%d%d%d", Day,Month,Year,Hour,Minute,Second);
  
  return message;
}


void setRelayState(bool relayState) {
  //Set the Relay State for ALL of the channels
  digitalWrite(Device1, !relayState);
  digitalWrite(Device2, !relayState);
  digitalWrite(Device3, !relayState);
  digitalWrite(Device4, !relayState);
}


void setRelayState(int device, bool relayState) {
  //Set the Relay State for a specific channel
  //LOW is ON and HIGH is OFF (Because of some twisted reason)

    switch (device) {
      case 1:
        digitalWrite(Device1, !relayState);
        break;
      case 2:
        digitalWrite(Device2, !relayState);
        break;
      case 3:
        digitalWrite(Device3, !relayState);
        break;
      case 4:
        digitalWrite(Device4, !relayState);
        break;
    }
}


String parseData(String timeStamp, 
                float humidity1,
                float humidity2,
                float temperature1,
                float temperature2,
                int relayState1,
                int relayState2,
                int relayState3,
                int relayState4) {

  String message = timeStamp + ";"
                    + humidity1 + ";"
                    + humidity2 + ";"
                    + temperature1 + ";"
                    + temperature2 + ";"
                    + relayState1 + ";"
                    + relayState2 + ";"
                    + relayState3 + ";"
                    + relayState4 + ";"
                    + "\n";

  return message;
}


void rtcAdjustTime() {
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}


void writeDataToSD(String message, String fileName) {
  File file;
  file = SD.open(fileName, FILE_WRITE);

  if (file) {
    file.print(message);
    file.close();
  } else {
    // if the file didn't open, print an error:
    throwError(rtc.now(), "ERROR: Failing to write to " + fileName);
  }
}


void setupPinMode() {
  pinMode(Device1, OUTPUT);
  pinMode(Device2, OUTPUT);
  pinMode(Device3, OUTPUT);
  pinMode(Device4, OUTPUT);
  
  pinMode(Pot, INPUT);
}


void getSettings() {
  float minTemperature = 14.0;
  float maxTemperature = 16.0;
  
  float minHumidity = 80.0;
  float maxHumidity = 90.0;
}


void initializeSDCard(bool debug) {

  DateTime now = rtc.now();

  if (debug) {
    debugSDCard();
  } 
  
  throwError(now, "INFO: Initializing SD Card...");
  
  if (!SD.begin(SDCSPin)) {
    throwError(now, "ERROR: SD Card initialization failed!");
    while (1);
  }
  throwError(now, "INFO: SD Card initialized successfully.");

  //probesFileName = "probes"+ getDateTimeString(now) + ".txt";
  probesFileName = "probes.txt";
  
}


void debugSDCard () {
  Serial.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
  root.close();
}


void setup() {
  setupPinMode();

  Serial.begin(9600);
  
  sensor1.begin();
  sensor2.begin();

  if (! rtc.begin()) {
    Serial.println("ERROR: RTC not found");
    Serial.flush();
    abort();
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //Logging a message about the reboot of the device
  throwError(rtc.now(), "ALERT: The device has been rebooted!");

  //Initialize the SDCard and prepare files
  initializeSDCard(false);

  //Getting settings from the config file
  getSettings();

  //Turning off all the relays
  setRelayState(LOW);
 
  //Waiting a while for the DHT11 to initialize properly
  delay(1000);
}


void loop() {
  DateTime now = rtc.now();

  //float correction = map(analogRead(Pot),0,1023,2100,2500);

  if ((unsigned long)(millis() - timer) > sampleRate) {
    timer = millis();  

    float humidity1 = sensor1.readHumidity();
    float humidity2 = sensor2.readHumidity();
    float temperature1 = sensor1.readTemperature();
    float temperature2 = sensor2.readTemperature();

    //float temperature1 = correction/100.0;
    //float temperature2 = correction/100.0;
    
  
    // Throw error message if measurements of temperature or humidity are not valid
    if ((isnan(temperature1) || isnan(humidity1)) || (isnan(temperature2) || isnan(humidity2))) {
    //if (isnan(temperature2) || isnan(humidity2)) {
      throwError(now, "ERROR: Reading from DHT11 failed.");
    } else {
  
      if (temperature1 > maxTemperature) {
        setRelayState(4, HIGH);
      }
  
      if (temperature1 < minTemperature) {
        setRelayState(4, LOW);
      }

      //Print results to serial and store to SDCard
      Serial.print(parseData(getTimestamp(now), humidity1, humidity2, temperature1, temperature2, !digitalRead(Device1), !digitalRead(Device2), !digitalRead(Device3), !digitalRead(Device4)));
      writeDataToSD(parseData(getTimestamp(now), humidity1, humidity2, temperature1, temperature2, !digitalRead(Device1), !digitalRead(Device2), !digitalRead(Device3), !digitalRead(Device4)), probesFileName);
  
    }
  }
}
