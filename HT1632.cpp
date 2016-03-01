/*
  HT1632.cpp - Library for communicating JY-MCU 3208 PRO
  This library provides higher-level access (including
  text drawing) for these modules.
  
  Created by Gaurav Manek, April 8, 2011.
  Released into the public domain.

  Modified to work only with the JY-MCU 3208 PRO
  32x8 red LED matrix
  by Ken Willmott, February 17, 2016

/* code between:

// ********************************
// JY-MCU modifications

and

// end of JY-MCU mods
//***********************************

is subject to the following license:
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Ken Willmott
 * ----------------------------------------------------------------------------


*/

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

#include "HT1632.h"

#if PIXELS_PER_BYTE != 8
	#error "The current drawImage implementation requires PIXELS_PER_BYTE == 8"
#endif

/*
 * HIGH LEVEL FUNCTIONS
 * Functions that perform advanced tasks using lower-level
 * functions go here:
 */

// *********************
// begin(CS, WR, DATA)
// Initialize HT1632, assign Arduino pins

void HT1632Class::begin(uint8_t pinCS1, uint8_t pinWR, uint8_t pinDATA) {
	_numActivePins = 1;
	_pinCS[0] = pinCS1;
	initialize(pinWR, pinDATA);
}

// Low level initialize

void HT1632Class::initialize(uint8_t pinWR, uint8_t pinDATA) {
	_pinWR = pinWR;
	_pinDATA = pinDATA;
	
	for (uint8_t i = 0; i < _numActivePins; ++i){
		pinMode(_pinCS[i], OUTPUT);
	}

	pinMode(_pinWR, OUTPUT);
	pinMode(_pinDATA, OUTPUT);
	
	select();
	
	for (uint8_t i = 0; i < NUM_CHANNEL; ++i) {
		// Allocate new memory for each channel
		mem[i] = (byte *)malloc(ADDR_SPACE_SIZE);
	}
	// Clear all memory
	clear();

	// Send configuration to chip:
	// This configuration is from the HT1632 datasheet, with one modification:
	//   The RC_MASTER_MODE command is not sent to the master. Since acting as
	//   the RC Master is the default behaviour, this is not needed. Sending
	//   this command causes problems in HT1632C (note the C at the end) chips. 
	
	// Send Master commands
	
	select(0b1111); // Assume that board 1 is the master.
	writeData(HT1632_ID_CMD, HT1632_ID_LEN);    // Command mode
	
	writeCommand(HT1632_CMD_SYSDIS); // Turn off system oscillator
	
	writeCommand(HT1632_CMD_COMS00);

	writeCommand(HT1632_CMD_SYSEN); //Turn on system
	writeCommand(HT1632_CMD_LEDON); // Turn on LED duty cycle generator
	writeCommand(HT1632_CMD_PWM(16)); // PWM 16/16 duty
	
	select();
	
	// Clear all screens by default:
	for(uint8_t i = 0; i < _numActivePins; ++i) {
		renderTarget(i);
		render();
	}
	// Set renderTarget to the first board.
	renderTarget(0);
}

void HT1632Class::renderTarget(uint8_t target) {
	if(target < _numActivePins) {
		_tgtRender = target;
	}
}


// ********************************
// JY-MCU modifications
//

// ************************
// drawImage(*source_image, width, height, x, y, offset)
//
// bit blit from memory to the display
//

void HT1632Class::drawImage(const byte * img, uint8_t width, uint8_t height, int8_t x, int8_t y, int img_offset)
{
  // Sanity checks
  if ( y + height < 0 or x + width<0 or y>COM_SIZE or x > OUT_SIZE )
    return;
  // Perform the copy

  for (int8_t xIndex = 0; xIndex < width; xIndex++)
  {
    for (int8_t yIndex = 0; yIndex < height; yIndex++)
    {
      uint8_t sourceData = pgm_read_byte(&img[img_offset
       + yIndex]);

      if (sourceData & (1<<xIndex))
      {
      setPixel(x+xIndex, y+yIndex);
      }
      else
      {
      clearPixel(x+xIndex, y+yIndex);
      }
    }
  }
}
// end drawImage

// ****************************************
// set and clear pixels
// slightly more efficient than write
//
// setPixel(x, y)
// clearPixel(x, y)
//

void HT1632Class::setPixel(uint8_t x, uint8_t y) {
	if( x < 0 || x > OUT_SIZE || y < 0 || y > COM_SIZE )
		return;
	mem[_tgtChannel][y+(x&~7)]
     |= (0b1 << PIXELS_PER_BYTE-1) >> (x % PIXELS_PER_BYTE);
}

void HT1632Class::clearPixel(uint8_t x, uint8_t y) {
	if( x < 0 || x > OUT_SIZE || y < 0 || y > COM_SIZE )
		return;
	mem[_tgtChannel][y+(x&~7)]
     &= ~((0b1 << PIXELS_PER_BYTE-1) >> (x % PIXELS_PER_BYTE));
}


// *****************************
// JY-MCU added methods
//

// ********************************
// writePixel(x, y, value)
//

void HT1632Class::writePixel(uint8_t x, uint8_t y, uint8_t val)
{
	if( x < 0 || x > OUT_SIZE || y < 0 || y > COM_SIZE )
		return;
      byte selectedByte = y+(x&~7);
	mem[_tgtChannel][selectedByte] =
      mem[_tgtChannel][selectedByte] &
     (~((0b1 << PIXELS_PER_BYTE-1) >> (x % PIXELS_PER_BYTE))) |
     ((val << PIXELS_PER_BYTE-1) >> (x % PIXELS_PER_BYTE));
}

// ******************************************************
// writeChar(column_position, ascii_value, column_width)
//
// raw character write allows fine control of position
// and width

void HT1632Class::writeChar(byte pos, byte val, byte cols)
{
  for (int x = 0; x < cols; x++)
  {

    byte fontColumn = pgm_read_byte (&working_font [val] [x]);
    setDisplayColumn(pos+x, fontColumn);
  }
}

// ****************************************************
// printChar(character_position, ascii_value)
//
// print a character in position 0-3
// of the display using standard font (cp437)

void HT1632Class::printChar(byte pos, byte val)
{
  writeChar(pos*8, val, 7);
  setDisplayColumn(pos*8+7, 0);
}

// *************************************
// printString(*string)
//
// print a standard null terminated string
// to the display

void HT1632Class::printString(const char* str)
{
  for (byte i=0; i<4 and str[i]!='\0'; i++)
  {
      printChar(i, str[i]);
  }
  render();
}

// *****************************************
// setDisplayColumn(position, value)
//
// set the column at <position> to <value>
// low bits are at the top, high bits at the bottom

void HT1632Class::setDisplayColumn(byte pos, byte val)
{
	for (byte y=0; y <8; y++)
      {
       if (val & 1<<y)
       {
         setPixel(pos, y);
       }
       else
       {
         clearPixel(pos, y);
       }
      }
}

// ********************************************
// printDigit(byte pos, byte val)
//
// print a numeric digit of
void HT1632Class::printDigit(byte pos, byte val)
{
  if (val <= 9)
  {
    printChar(pos, val+'0');  // digits to print
  }
  else
  {
    printChar(pos, val+'A'-10);  // hex and up
  }
}

// ************************************************
// printNum(long value, byte base)
//
// Print the number given by <value>, right justified.
// Print a minus sign if the number is negative.
// Numeric base given by <base>
// Leading zeroes if <leading> is set

void HT1632Class::printNum(long val, byte base, bool leading)
{
  const byte numDigits = 4;

  long maxNegativeDigits = base;
  for (int i = 0; i < numDigits - 2; i++)
    {
      maxNegativeDigits *= base;
    }

  // validate number size for digits:

  if (val >= maxNegativeDigits * base)
  {
    printString("HIGH");
  }
  else if (val <= -maxNegativeDigits)
  {
    printString("LOW ");
  }

  //negative number
  else if (val < 0)
  {
    // prepare first symbol to print when digits are exhausted
    boolean needsMinusSign = true;
    int posval = -val;  //use the absolute value of the number
    for (int i = numDigits - 1; i >= 0; i--)
    {
      if (posval > 0)
        printDigit(i, (posval % base));  // digits to print
      else if (needsMinusSign)
      {
        printChar(i, '-');  // print one minus sign
        needsMinusSign = false;
      }
      else
        printChar(i, ' ');  // the rest are spaces
      posval /= base;
    }
  }
  else  // it is a positive number
  {
    for (int i = numDigits - 1; i >= 0; i--)
    {
      if (val > 0 || i == numDigits - 1)
        printDigit(i, (val % base));  // digits to print
      else
        if (leading)
        {
          printChar(i, '0');  // the rest are zeroes
        }
        else
        {
          printChar(i, ' ');  // the rest are spaces
        }
      val /= base;
    }
  }

  render();
}

//
// end of JY-MCU mods
//***********************************

// ****************************
// fill() turns on all pixels
// clear() turns off all pixels

void HT1632Class::fill() {
	for(uint8_t i = 0; i < ADDR_SPACE_SIZE; ++i) {
		mem[_tgtChannel][i] = 0xFF;
	}
}

void HT1632Class::clear(){
	for(uint8_t c = 0; c < NUM_CHANNEL; ++c) {
		for(uint8_t i = 0; i < ADDR_SPACE_SIZE; ++i) {
			mem[c][i] = 0x00; // Needs to be redrawn
		}
	}
}

//*******************************************
// render()
//
// Draw the contents of memory buffer.
// Low level graphics operations don't write
// directly to the screen, but to a buffer.
// render() must be called to display the buffer.

void HT1632Class::render() {
	if(_tgtRender >= _numActivePins) {
		return;
	}
	
	select(0b0001 << _tgtRender); // Selecting the chip
	
	writeData(HT1632_ID_WR, HT1632_ID_LEN);
	writeData(0, HT1632_ADDR_LEN); // Selecting the memory address

	// Write the channels in order
	for(uint8_t c = 0; c < NUM_CHANNEL; ++c) {
		for(uint8_t i = 0; i < ADDR_SPACE_SIZE; ++i) {
			// Write the higher bits before the the lower bits.
			writeData(mem[c][i] >> HT1632_WORD_LEN, HT1632_WORD_LEN); // Write the data in reverse.
			writeData(mem[c][i], HT1632_WORD_LEN); // Write the data in reverse.
		}
	}

	select(); // Close the stream at the end
}

// ***********************************************

// Set the brightness to an integer level
// between 1 and 16 (inclusive).
// Uses the PWM feature to set the brightness.
//
// setBrightness(brightness)

void HT1632Class::setBrightness(char brightness, char selectionmask) {
	if(selectionmask == 0b00010000) {
		if(_tgtRender < _numActivePins) {
			selectionmask = 0b0001 << _tgtRender;
		} else {
			return;
		}
	}
	
	select(selectionmask); 
	writeData(HT1632_ID_CMD, HT1632_ID_LEN);    // Command mode
	writeCommand(HT1632_CMD_PWM(brightness));   // Set brightness
	select();
}

// *******************************************
// end of anything the user would need to know
// *******************************************

/*
 * LOWER LEVEL FUNCTIONS
 * Functions that directly talk to hardware go here:
 */
 
void HT1632Class::writeCommand(char data) {
	writeData(data, HT1632_CMD_LEN);
	writeSingleBit();
}
 
// Integer write to display. Used to write commands/addresses.
// PRECONDITION: WR is LOW

void HT1632Class::writeData(byte data, uint8_t len) {
	for(int j = len - 1, t = 1 << (len - 1); j >= 0; --j, t >>= 1){
		// Set the DATA pin to the correct state
		digitalWrite(_pinDATA, ((data & t) == 0)?LOW:HIGH);
		NOP(); // Delay 
		// Raise the WR momentarily to allow the device to capture the data
		digitalWrite(_pinWR, HIGH);
		NOP(); // Delay
		// Lower it again, in preparation for the next cycle.
		digitalWrite(_pinWR, LOW);
	}
}

// Write single bit to display, used as padding between commands.
// PRECONDITION: WR is LOW

void HT1632Class::writeSingleBit() {
	// Set the DATA pin to the correct state
	digitalWrite(_pinDATA, LOW);
	NOP(); // Delay
	// Raise the WR momentarily to allow the device to capture the data
	digitalWrite(_pinWR, HIGH);
	NOP(); // Delay
	// Lower it again, in preparation for the next cycle.
	digitalWrite(_pinWR, LOW);
}

void HT1632Class::setCLK(uint8_t pinCLK) {
	_pinCLK = pinCLK;
	pinMode(_pinCLK, OUTPUT);
	digitalWrite(_pinCLK, LOW);
}

inline void HT1632Class::pulseCLK() {
	digitalWrite(_pinCLK, HIGH);
	NOP();
	digitalWrite(_pinCLK, LOW);
}


// Choose a chip. This function sets the correct CS line to LOW, and the rest to HIGH
// Call the function with no arguments to deselect all chips.
// Call the function with a bitmask (0b4321) to select specific chips. 0b1111 selects all.
//
 
void HT1632Class::select(uint8_t mask) {
	for(uint8_t i=0, t=1; i<_numActivePins; ++i, t <<= 1){
		digitalWrite(_pinCS[i], (t & mask)?LOW:HIGH);
	}
}

void HT1632Class::select() {
	select(0);
}


// end class definitions
