HT1632 - Library for communicating JY-MCU 3208 PRO
LED display module

  This library provides higher-level access (including
  text drawing) for these modules.
  
  Modified to work only with the JY-MCU 3208 PRO
  32x8 red LED matrix
  by Ken Willmott, February 17, 2016

  Portions created by Gaurav Manek, April 8, 2011.
  Released into the public domain.



Hardware:

JY-MCU PRO 3208 module:

Note, this is for the PRO module. It is not tested on the earlier version.

For this application, the onboard AVR on the display is disabled
by connecting a jumper from the <reset> pin on the display ICSP
connector to an adjacent ground (on the display power connector).
This enables a connection to the HT1632 display driver on the
same connector as follows:

Arduino | display
------- | -------
   GND   |   GND
   VCC   |   VCC
   "CS"  |   MOSI
   "WR"  |   MISO
   "DATA" |  SCK

The CS, WR, and DATA pins can be assigned to any digital pins
or analog pins configured for digital I/O
on the Arduino. Example: CS=D7, WR=D8, DATA=D6 are
convenient because they are all on the ICSP connector of the UNO.
