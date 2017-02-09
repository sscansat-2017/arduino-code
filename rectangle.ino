#include <i2c_t3.h>
#include <TinyGPS++.h>
#include <teenkSeries.h>
#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>

File myFile;
File count_file;
TinyGPSPlus gps;
kSeries K_30;

const int chipSelect = BUILTIN_SDCARD;

int led = 13;
time_t last_updated = 0;
String current_run = "nil";

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

void setup()
{
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);

  // Open serial communications to computer.
  Serial.begin(9600);

  // K30 uses serial 1.

  // Set up hardware serial for GPS.
  Serial2.begin(9600);

  // Set up hardware serial for temperature/pressure/transmitter.
  Serial4.begin(9600);

  // set the Time library to use Teensy's RTC to keep time
  setSyncProvider(getTeensy3Time);
  Teensy3Clock.set(0);
  setTime(0);
  Serial.println("time init");

  if (!SD.begin(chipSelect)) {
    Serial.println("sd fail");
    critical_error(50);
    return;
  }

  // Extract current run for use with SD card saving.
  count_file = SD.open("runcount");
  String line = "";
  if (count_file) {
    while (count_file.available()) {
      line = count_file.readStringUntil('\n');
    }
  }
  else {
    Serial.println("count file fail");
    critical_error(50);
  }
  count_file.close();
  current_run = String(line.toInt() + 1);
  bool err = SD.remove("runcount");
  if (err != true) {
    Serial.println("count file fail");
    critical_error(50);
  }
  count_file = SD.open("runcount", FILE_WRITE);
  count_file.print(current_run);
  count_file.close();
  Serial.println("sd init");
  Serial.print("run "); Serial.println(current_run);
}

void loop()
{
  // Feed the GPS parser.
  while (Serial2.available() > 0)
    gps.encode(Serial2.read());

  // Circle Board Data.
  String tmp, prs;
  while (Serial4.available() > 0)
  {
    String prov = "";
    while (prov != "PROV")
    {
      prov = Serial4.readStringUntil(',');
      prov.trim();
    }
    tmp = Serial4.readStringUntil(',');
    prs = Serial4.readStringUntil(','); 
  }

  // Open file for writing.
  myFile = SD.open(current_run.c_str(), FILE_WRITE);
  if (!myFile) {
    Serial.print("error opening "); Serial.println(current_run.c_str());
  }

  // Time since start of experiment.
  //time_t rtc = getTeensy3Time();
  int rtc = millis();

  // GPS time.
  int gpstime = gps.time.value();

  // Other GPS data.
  int gpssat = gps.satellites.value();
  float gpslat = gps.location.lat();
  float gpslng = gps.location.lng();
  float gpsalt = gps.altitude.meters();
  char latstring[9], lngstring[9], altstring[9];
  dtostrf(gpslat, 8, 5, latstring);
  dtostrf(gpslng, 8, 5, lngstring);
  dtostrf(gpsalt, 6, 2, altstring);
  // If sec % 2 = 0, Collect CO2 data.
  //if ((rtc % 2) == 0) {
  int co2 = K_30.getCO2('p');
  //}

  char payload[80];
  // RUNID, RTC, GPSTIME, GPSSAT, GPSLAT, GPSLNG, GPSALT, K30CO2, TMP, PRS
  sprintf(payload, "%s,%d,%d,%d,%s,%s,%s,%d,%s,%s,", current_run.c_str(), (int)rtc, gpstime, gpssat, latstring, lngstring, altstring, co2, tmp.c_str(), prs.c_str());
  Serial4.println(payload);
  Serial.println(payload);
  myFile.println(payload);

  // Write data out to SD card.
  myFile.close();

  // Blink LED to show data has been updated.
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(50);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
}

void critical_error(int ms) {
  while (true) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(ms);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(ms);
  }
}
