// Digiclock matrix 1.00

// for Arduino and JY-MCU PRO 3208 8x32 matrix display
// and DS3231 RTC
//
//
// clock with no PPS from RTC
//

// 2016-02-11 port
// 2016-02-16 add auto brightness
// 2016-02-19 extend clock functions to remove them from the base library
// 2016-02-22 eliminate PPS

// Hardware:
//
// JY-MCU PRO 3208 module:
//
// Note, this is for the PRO module. It is not tested on the earlier version.
//
// For this application, the onboard AVR on the display is disabled
// by connecting a jumper from the <reset> pin on the display ICSP
// connector to an adjacent ground (on the display power connector).
// This enables a connection to the HT1632 display driver on the
// same connector as follows:
//
//   Arduino  display
//   -------  -------
//   GND      GND
//   VCC      VCC
//   "CS"     MOSI
//   "WR"     MISO
//   "DATA"   SCK
//
// The CS, WR, and DATA pins can be assigned to any digital pins
// or analog pins configured for digital I/O
// on the Arduino. Example: CS=D7, WR=D8, DATA=D6
//

// RTC on I2C pins SCA, SCL, VCC, GND
// DS3231 library handles the interface.
//

#include <HT1632.h>

#include "ClockMatrix.h"
ClockMatrix LEDpanel;
const byte LEDpanelCS = 7;
const byte LEDpanelWR = 8;
const byte LEDpanelDATA = 6;

#include <EEPROM.h>
#include <Wire.h>

#include <Time.h>                  //https://github.com/PaulStoffregen/Time
#include <DS3232RTC.h>             //https://github.com/JChristensen/DS3232RTC
#include <Timezone.h>              //https://github.com/JChristensen/Timezone


// time handling variables:
//
#define RESET_OSC_STOPPED_FLAG true

// RTC time is UTC:

time_t utc = SECS_YR_2000;  // universal coordinated time (GMT)
time_t localTime;  //  display time

signed char timeZoneRule;  // current time zone
signed char defaultTimeZoneRule = 3;  // set to EST time by default

int utcAddrEEPROM = 0x40;  // where to store offset in EEPROM
int blinkModeEEPROM = 0x41;
int hour24TimeModeEEPROM = 0x42;

bool localTimeMode = true;
byte hour24TimeMode;
bool useEEPROM = true;

// RTC polling
time_t lastTimeRTC;
unsigned long timeCheck;
const int timeCheckInterval = 100;

unsigned long blinkStartTime;
byte blinkMode;
const int blinkDuration = 75;
int blinkCounter;
const int blinkPeriod = 5; // number of seconds between blinks
bool blinkIsOn = false;

// auto brightness settings control the brightness levels and intervals:
//
const byte DAY_BR = 3;
const byte TWEEN_BR = 2;
const byte NIGHT_BR = 1;

byte brightnessHours[24] = {NIGHT_BR, NIGHT_BR, NIGHT_BR, NIGHT_BR, NIGHT_BR, NIGHT_BR,
                            TWEEN_BR, TWEEN_BR, DAY_BR, DAY_BR, DAY_BR, DAY_BR,
                            DAY_BR, DAY_BR, DAY_BR, DAY_BR, DAY_BR, DAY_BR,
                            TWEEN_BR, TWEEN_BR, TWEEN_BR, NIGHT_BR, NIGHT_BR, NIGHT_BR
                           };
// Time Zone Rules:
// range can be from +14h (Line Islands Time) to -11h (Samoa Standard Time)

//China Standard Time Zone (Beijing, Chengdu)
TimeChangeRule zhCST = {"CST", Second, Sun, Mar, 2, 480};  //China Standard Time = UTC + 8 hours
Timezone zhST(zhCST, zhCST);

//Canada Newfoundland Time Zone (St. Johns)
TimeChangeRule caNDT = {"NDT", Second, Sun, Mar, 2, -150};  //Newfoundland Daylight Time = UTC - 2.5 hours
TimeChangeRule caNST = {"NST", First, Sun, Nov, 2, -210};   //Newfoundland Standard Time = UTC - 3.5 hours
Timezone caNT(caNDT, caNST);

//Canada Atlantic Time Zone (Halifax)
TimeChangeRule caADT = {"ADT", Second, Sun, Mar, 2, -180};  //Atlantic Daylight Time = UTC - 3 hours
TimeChangeRule caAST = {"AST", First, Sun, Nov, 2, -240};   //Atlantic Standard Time = UTC - 4 hours
Timezone caAT(caADT, caAST);

//US Eastern Time Zone (Toronto, New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);

//US Central Time Zone (Winnipeg, Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, dowSunday, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, dowSunday, Nov, 2, -360};
Timezone usCT(usCDT, usCST);

//US Mountain Time Zone (Edmonton, Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDT", Second, dowSunday, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, dowSunday, Nov, 2, -420};
Timezone usMT(usMDT, usMST);

//Arizona is US Mountain Time Zone but does not use DST
Timezone usAZ(usMST, usMST);

//US Pacific Time Zone (Vancouver, Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, dowSunday, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, dowSunday, Nov, 2, -480};
Timezone usPT(usPDT, usPST);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get abbreviations

Timezone* zoneTable[] = {&zhST, &caNT, &caAT, &usET, &usCT, &usMT, &usAZ, &usPT};
const byte zoneTableSize = 8;

//*******************************************************
// code begins
//*******************************************************
//

void setup()
{

  Serial.begin(9600);

  // load time zone offset from EEPROM
  if (useEEPROM == true)
  {
    timeZoneRule = EEPROM.read(utcAddrEEPROM);  // current time zone
    if (timeZoneRule >= zoneTableSize or timeZoneRule < 0) // if EEPROM is uninitialized set to default.
    {
      EEPROM.write(utcAddrEEPROM, defaultTimeZoneRule);
      timeZoneRule = defaultTimeZoneRule;
    }
    hour24TimeMode = EEPROM.read(hour24TimeModeEEPROM);
    blinkMode = EEPROM.read(blinkModeEEPROM);
  }
  else
  {
    timeZoneRule = defaultTimeZoneRule;
    hour24TimeMode = true;
    blinkMode = false;
  }

  //initialize the display
  LEDpanel.begin(LEDpanelCS, LEDpanelWR, LEDpanelDATA);  // CS, WR, DATA
  LEDpanel.setBrightness(DAY_BR);
  LEDpanel.clear();

  printCommandList();
}

void loop()
{

  // realtime input loop
  //
  // everything inside this must be non-blocking
  //

  // Check RTC time every timeCheckInterval. Should be about 100ms.
  if (millis() - timeCheck >= timeCheckInterval)
  {
    timeCheck += timeCheckInterval;

    //*****************************************
    // and all the once per second functions:

    // provided that the RTC has valid time
    if (not RTC.oscStopped(not RESET_OSC_STOPPED_FLAG))
    {
      // one second elapsed test
      utc = RTC.get(); //Get RTC time
      if (utc != lastTimeRTC) // check RTC
      {
        lastTimeRTC = utc;
        display_time();

        // Update blink timer.
        // The on time is always synchronized with the time update
        // but the off time is variable so it is placed outside the
        // once per second code.

        if (blinkMode != 0)
        {
          ++blinkCounter;
          if (blinkCounter % blinkPeriod == 0)
          {
            blinkStartTime = millis();
            blinkIsOn = true;
            dotOn();
          }
        }
      }
    }
    else  // RTC is not set
    {
      LEDpanel.printString("!set");
    }
  }

  //********************* end once per second code


  //********************
  // Periodic functions:
  //
  // test for the end of the blink interval
  if (blinkMode)
  {
    if (millis() - blinkStartTime >= blinkDuration and blinkIsOn == true)
    {
      blinkIsOn = false;
      dotOff();
    }
  }

  // Handle input from the serial monitor:
  //
  getSerial();
}
//********************* end loop


//*********************************************************
//
// Time functions:
//
//*********************************************************

// main time display routine - send time to the display
//
// called once per second by main program

void display_time()
{

  //  global variable utc is updated in loop()
  if (localTimeMode)
  {
    localTime = zoneTable[timeZoneRule]->toLocal(utc);
  }
  else
  {
    localTime = utc;
  }

  // auto brightness
  static byte lastHour = 0;
  if (hour(localTime) != lastHour)
  {
    LEDpanel.setBrightness(brightnessHours[hour(localTime)]);
    lastHour = hour(localTime);
  }

  // write to display
  //
  if (hour24TimeMode == true)
  {
    LEDpanel.print2digitsUpper(hour(localTime), true);
  }
  else // 12 hour time
  {
    LEDpanel.print2digitsUpper(hourFormat12(localTime), false);
  }
  LEDpanel.print2digitsLower(minute(localTime), true);
  LEDpanel.setColon();

  LEDpanel.render(); // This updates the display on the screen.
}

//********************************************
//
// Display Support functions:
//
//********************************************

void dotOn()
{
  LEDpanel.setPixel(0, 0);
  LEDpanel.render(); // This updates the display on the screen.
}

void dotOff()
{
  LEDpanel.clearPixel(0, 0);
  LEDpanel.render(); // This updates the display on the screen.
}

//********************************************
// Serial monitor

void getSerial()
{
  // serial command buffer
#define BUFF_MAX 80
  static char recv[BUFF_MAX];
  static unsigned int charsReceived = 0;


  char in;
  if (Serial.available() > 0) {
    in = Serial.read();

    if ((in == 10 || in == 13) && (charsReceived >= 0))
    {
      parse_cmd(recv, charsReceived);
      charsReceived = 0;
      recv[0] = 0;
    }
    else if (in < 32 || in > 122)
    {
      // ignore ~[" "-9A-Za-z]
    }
    else if (charsReceived > BUFF_MAX - 2)
    { // drop lines that are too long
      // drop
      charsReceived = 0;
      recv[0] = 0;
    }
    else if (charsReceived < BUFF_MAX - 2)
    {
      recv[charsReceived] = in;
      recv[charsReceived + 1] = 0;
      charsReceived += 1;
    }
  }
}

// serial monitor simple command interpreter

void parse_cmd(char *cmd, int cmdsize)
{
  uint8_t i;
  uint8_t reg_val;
  char *nextValue;
  tmElements_t setTimeElements;
  time_t unixTime;

  uint8_t param[8];

  if (cmdsize == 0) {
    printCommandList();

    // st <YYYY M D h m s> aka set time
  } else if (cmd[0] == 's' && cmd [1] == 't') {
    getParams(&cmd[2], param, 6);
    setTimeElements.Year = CalendarYrToTm(param[0]);
    setTimeElements.Month = param[1];
    setTimeElements.Day = param[2];
    setTimeElements.Hour = param[3];
    setTimeElements.Minute = param[4];
    setTimeElements.Second = param[5];

    unixTime = makeTime(setTimeElements);
    Serial.println(F("Press enter key to set time..."));
    while (Serial.available() == 0);
    /*
        while (Serial.available() > 0)
        {
          Serial.read();
        }
    */
    //    RTC.set(unixTime);
    Serial.println(F("Time is set to"));
    printTime();

  } else if (cmd[0] == 'b') {  // "b" - set blink mode
    byte temp = EEPROM.read(blinkModeEEPROM);
    blinkMode = !temp;
    EEPROM.write(blinkModeEEPROM, blinkMode);
    Serial.print(F("Blink mode set to "));
    Serial.println(boolString(blinkMode));

  } else if (cmd[0] == 'h') {  // "h" - set 24 hr mode
    byte temp = EEPROM.read(hour24TimeModeEEPROM);
    hour24TimeMode = !temp;
    EEPROM.write(hour24TimeModeEEPROM, hour24TimeMode);
    Serial.print(F("24 hour mode set to "));
    Serial.println(boolString(hour24TimeMode));

  } else if (cmd[0] == 'z') {  // "z" - set time zone rule

    byte cmdOffset = strtol(&cmd[1], NULL, 0);
    if (cmdOffset >= 0 && cmdOffset < zoneTableSize)
    {
      timeZoneRule = cmdOffset;
      EEPROM.write(utcAddrEEPROM, timeZoneRule);
      Serial.print(F("time zone rule set to "));
      Serial.println(timeZoneRule, DEC);
    }
    else
      Serial.println(F("time zone rule out of range"));

  } else if (cmd[0] == 'r' && cmdsize == 1) {  // "r" - get registers and config info
    Serial.println(F("----registers-----"));
    Serial.print(F("control register = 0x"));
    Serial.println(RTC.readRTC(RTC_CONTROL), HEX);
    Serial.print(F("status register =  0x"));
    Serial.println(RTC.readRTC(RTC_STATUS), HEX);
    Serial.print(F("aging register = "));
    Serial.println((int8_t)RTC.readRTC(RTC_AGING));

  } else if (cmd[0] == 't') {  // "t" - show time
    printTime();
  } else {
    Serial.print(F("unknown command syntax "));
    Serial.println(cmd[0]);
    Serial.println(cmd[0], DEC);
  }
}

void printTime()
{
  // provided that the RTC has valid time
  if (not RTC.oscStopped(not RESET_OSC_STOPPED_FLAG))
  {
    tmElements_t tm;
    time_t unixTime = RTC.get(); //Get time

    if (localTimeMode)
    {
      unixTime = zoneTable[timeZoneRule]->toLocal(unixTime);
    }
    breakTime(unixTime, tm);

    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(F("-"));
    Serial.print(tm.Month);
    Serial.print(F("-"));
    Serial.print(tm.Day);
    Serial.print(F(" d:"));
    Serial.print(tm.Wday);

    Serial.print(F(" ("));
    Serial.print(dayStr(tm.Wday));
    Serial.print(F(", "));
    Serial.print( monthStr(tm.Month) );
    Serial.print(F(" "));
    Serial.print(tm.Day);
    Serial.print(F(", "));
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(F(")"));

    Serial.print(F(" "));
    Serial.print(tm.Hour);
    if (tm.Minute < 10)
    {
      Serial.print(F(":0"));
    }
    else
    {
      Serial.print(F(":"));
    }
    Serial.print(tm.Minute);
    if (tm.Second < 10)
    {
      Serial.print(F(":0"));
    }
    else
    {
      Serial.print(F(":"));
    }
    Serial.print(tm.Second);
    Serial.println();
  }
  else  // RTC is not set
  {
    Serial.println(F("The time has not been set!"));
  }

}
void printStatus()
{
  Serial.print(F("RTC temperature is "));
  Serial.print(RTC.temperature() / 4.0, 2);
  Serial.println(F(" degrees Celsius"));

  Serial.print(F("Time zone currently set to "));
  zoneTable[timeZoneRule]->toLocal(now(), &tcr);
  Serial.print(tcr->abbrev);
  Serial.print(F(", UTC "));
  int temp = zoneTable[timeZoneRule]->toLocal(utc) - utc;
  if (temp > 0)
  {
    Serial.print('+');
  }
  Serial.print(temp / 3600.0,  1);
  Serial.println(F(" hours."));


  Serial.print(F("Blink mode set to "));
  Serial.println(boolString(blinkMode));

  Serial.print(F("24 hour mode set to "));
  Serial.println(boolString(hour24TimeMode));
}

void printCommandList()
{
  Serial.println(F("**********************************************"));
  Serial.println(F("*** Digiclock JY-MCU configuration utility ***"));
  Serial.println(F("Status:"));
  printStatus();
  printTime();
  Serial.println(F("Commands:"));
  Serial.println(F("<enter> - show this"));
  Serial.println(F("r - show config register info"));
  Serial.println(F("b - toggle blink mode"));
  Serial.println(F("h - toggle 12/24 hour display mode"));
  Serial.println(F("z <val> - set time zone"));
  Serial.println(F("  Available time zone rules:"));
  for (int i = 0; i < zoneTableSize; i++)
  {
    if (i % 4 == 0) Serial.print(F("  "));
    Serial.print(i);
    Serial.print(F("="));
    zoneTable[i]->toLocal(RTC.get(), &tcr);
    Serial.print(tcr->abbrev);
    Serial.print(F(" "));
    if (i % 4 == 3) Serial.println(F("  "));
  }
  Serial.println(F("t - read time"));
  Serial.println(F("st <YYYY M D h m s> - set time"));
  Serial.println(F("Enter the UTC time as: YYYY M D h m s"));
}

void getParams(char *input, uint8_t *p, int n)
{
  int i;
  for (i = 0; i < n; i++)
  {
    p[i] = strtol(input, &input, 0);
  }
}

char * boolString(byte val)
{
  if (val == true)
    return "on";
  else
    return "off";
}

