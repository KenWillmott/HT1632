// Test Program for the JY-MCU 3208 PRO
//
// 2016-02-14 by Ken Willmott
// 2016-02-25 test all library functions

#include <HT1632.h>
HT1632Class LEDmatrix;

// active array size:
const byte xSize = 32;
const byte ySize = 8;

const int maxIterations = 200;

unsigned long displayInterval = 100; //0.1s

void setup() {
  //initialize the display
  LEDmatrix.begin(7, 8, 6);  // CS, WR, DATA

}

void loop()
{
  LEDmatrix.setBrightness(2);
  LEDmatrix.clear();

  LEDmatrix.printString("Test");
  delay(1000);

  LEDmatrix.printString("ing");
  delay(1000);

  LEDmatrix.printString("****");
  delay(1000);

  LEDmatrix.fill();

  for (int i = 0; i < 4; i++)
  {
    LEDmatrix.printChar(i, 'a' + i);
    LEDmatrix.render();
    delay(500);
  }
  delay(1000);

  for (int j = 0; j < (256 - 4); j += 4)
  {
    for (int i = 0; i < 4; i++)
    {
      LEDmatrix.printChar(i, j + i);
      delay(50);
    }
    LEDmatrix.render();
  }

  for (long i = -50; i < 50; i++)
  {
    LEDmatrix.printNum(i, 10);
    delay(100);
  }

  for (byte xIndex = 0; xIndex < xSize; xIndex++)
  {
    for (byte yIndex = 0; yIndex < ySize; yIndex++)
    {
      LEDmatrix.setPixel(xIndex, yIndex);
      LEDmatrix.render(); // This updates the display on the screen.
    }
  }

  for (byte xIndex = 0; xIndex < xSize; xIndex++)
  {
    for (byte yIndex = 0; yIndex < ySize; yIndex++)
    {
      LEDmatrix.clearPixel(xIndex, yIndex);
      LEDmatrix.render(); // This updates the display on the screen.
    }
  }

  for (byte xIndex = 0; xIndex < xSize; xIndex++)
  {
    for (byte yIndex = 0; yIndex < ySize; yIndex++)
    {
      LEDmatrix.writePixel(xIndex, yIndex, yIndex & xIndex);
    }
  }
  LEDmatrix.render(); // This updates the display on the screen.
  delay(1000);
  
  LEDmatrix.fill();
  LEDmatrix.render(); // This updates the display on the screen.

  for (int brightness = 1; brightness < 16; brightness++)
  {
    LEDmatrix.setBrightness(brightness);
    LEDmatrix.printNum(brightness, 10);
    delay(500);
    LEDmatrix.fill();
    LEDmatrix.render(); // This updates the display on the screen.
    delay(500);

  }
}



