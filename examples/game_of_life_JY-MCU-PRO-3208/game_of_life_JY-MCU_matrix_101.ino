// Conway's "game of life" cellular automaton
//
// Demo for the JY-MCU 3208 PRO
//
// 2016-02-14 adapted from anon by Ken Willmott
// 2016-02-16 check for dead field

#include <HT1632.h>
HT1632Class LEDmatrix;

// active array size:
const byte xSize = 32;
const byte ySize = 8;

const int maxIterations = 200;
const bool IDENTICAL = false;

int iteration;
unsigned long displayInterval = 100; //0.1s
unsigned long timeStamp;

byte playingField[xSize + 2][ySize + 2]; // field, plus a border

void setup() {
  //initialize the display
  LEDmatrix.begin(7, 8, 6);  // CS, WR, DATA
  LEDmatrix.setBrightness(2);
  LEDmatrix.clear();

  LEDmatrix.printString("Game");
  delay(1000);
  LEDmatrix.clear();

  LEDmatrix.printString(" of ");
  delay(1000);
  LEDmatrix.clear();

  LEDmatrix.printString("Life");
  delay(1000);

  initField(playingField);
}

void loop()
{
  if (millis() - timeStamp >= displayInterval)
  {
    updateDisplay(playingField);
    bool fieldStatus = life(playingField);
    iteration++;
    if (iteration > maxIterations or fieldStatus == IDENTICAL)
    {
      initField(playingField);
      iteration = 0;
    }
    timeStamp += displayInterval;
  }
}

// fill field with random values
//
void initField(byte array[xSize + 2][ySize + 2])
{
  for (byte xIndex = 1; xIndex < xSize + 1; xIndex++)
  {
    for (byte yIndex = 1; yIndex < ySize + 1; yIndex++)
    {
      array[xIndex][yIndex] = (byte)random(2);
    }
  }
}

bool life(byte array[xSize + 2][ySize + 2])
{
  //Copies the main array to a temp array so changes can be entered into a grid
  //without effecting the other cells and the calculations being performed on them.
  byte temp[xSize + 2][ySize + 2];
  copy(array, temp);

  for (byte xIndex = 1; xIndex < xSize + 1; xIndex++)
  {
    for (byte yIndex = 1; yIndex < ySize + 1; yIndex++)
    {
      //The Moore neighborhood checks all 8 cells surrounding the current cell in the array.
      byte count = 0;
      count = array[xIndex - 1][yIndex] +
              array[xIndex - 1][yIndex - 1] +
              array[xIndex][yIndex - 1] +
              array[xIndex + 1][yIndex - 1] +
              array[xIndex + 1][yIndex] +
              array[xIndex + 1][yIndex + 1] +
              array[xIndex][yIndex + 1] +
              array[xIndex - 1][yIndex + 1];
      //The cell dies.
      if (count < 2 || count > 3)
        temp[xIndex][yIndex] = 0;
      //The cell stays the same.
      if (count == 2)
        temp[xIndex][yIndex] = array[xIndex][yIndex];
      //The cell either stays alive, or is "born".
      if (count == 3)
        temp[xIndex][yIndex] = 1;
    }
  }
  //Copies the completed temp array back to the main array.
  return copy(temp, array);
}

// Copies one array to another.
// Returns "true" if they are identical
//
bool copy(byte array1[xSize + 2][ySize + 2], byte array2[xSize + 2][ySize + 2])
{
  bool comparison = IDENTICAL;
  for (byte xIndex = 1; xIndex < xSize + 1; xIndex++)
  {
    for (byte yIndex = 1; yIndex < ySize + 1; yIndex++)
    {
      if (array2[xIndex][yIndex] != array1[xIndex][yIndex])
      {
        array2[xIndex][yIndex] = array1[xIndex][yIndex];
        comparison =  not IDENTICAL;
      }
    }
  }
  return comparison;
}

void updateDisplay(byte array[xSize + 2][ySize + 2])
{
  for (byte xIndex = 1; xIndex < xSize + 1; xIndex++)
  {
    for (byte yIndex = 1; yIndex < ySize + 1; yIndex++)
    {
      LEDmatrix.writePixel(xIndex - 1, yIndex - 1, array[xIndex][yIndex]);
    }
  }

  LEDmatrix.render(); // This updates the display on the screen.
}



