/*
  HT1632.h - Library for communicating JY-MCU 3208 PRO
  This library provides higher-level access (including
  text drawing) for these modules.
  
  Modified to work only with the JY-MCU 3208 PRO
  32x8 red LED matrix
  by Ken Willmott, February 17, 2016

  Created by Gaurav Manek, April 8, 2011.
  Released into the public domain.

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

#ifndef HT1632_h
#define HT1632_h

#include <Arduino.h>
#ifdef __AVR__
 #include <avr/pgmspace.h>
#elif defined(ESP8266)
 #include <pgmspace.h>
#else
 #define PROGMEM
#endif

//*************************
// custom font for JY-MCU

#include "cp437alt_font.h"
#define BIGNUM_OFFSET (0xC0)


/*
 * USER OPTIONS
 * Change these options
 */

  #define COM_SIZE 8
  #define OUT_SIZE 32
  #define NUM_CHANNEL 1
  #define USE_NMOS 1

/*
 * END USER OPTIONS
 * Don't edit anything below unless you know what you are doing!
 */


 // Pixels in a single byte of the internal image representation:
#define PIXELS_PER_BYTE 8

// Address space size (number of 4-bit words in HT1632 memory)
// Exactly equal to the number of 4-bit address spaces available.
#define ADDR_SPACE_SIZE (COM_SIZE * OUT_SIZE / PIXELS_PER_BYTE)

// NO-OP Definition
#define NOP(); __asm__("nop\n\t"); 
// The HT1632 requires at least 50 ns between the change in data and the rising
// edge of the WR signal. On a 16MHz processor, this provides 62.5ns per NOP. 

// Standard command list.
// This list is modified from original code by Bill Westfield

#define HT1632_ID_CMD 0b100	/* ID = 100 - Commands */
#define HT1632_ID_RD  0b110	/* ID = 110 - Read RAM */
#define HT1632_ID_WR  0b101	/* ID = 101 - Write RAM */
#define HT1632_ID_LEN 3         /* IDs are 3 bits */

#define HT1632_CMD_SYSDIS 0x00	/* CMD= 0000-0000-x Turn off oscil */
#define HT1632_CMD_SYSEN  0x01	/* CMD= 0000-0001-x Enable system oscil */
#define HT1632_CMD_LEDOFF 0x02	/* CMD= 0000-0010-x LED duty cycle gen off */
#define HT1632_CMD_LEDON  0x03	/* CMD= 0000-0011-x LEDs ON */
#define HT1632_CMD_BLOFF  0x08	/* CMD= 0000-1000-x Blink ON */
#define HT1632_CMD_BLON   0x09	/* CMD= 0000-1001-x Blink Off */
#define HT1632_CMD_SLVMD  0x10	/* CMD= 0001-00xx-x Slave Mode */
#define HT1632_CMD_MSTMD  0x14 /* CMD= 0001-01xx-x Master Mode, on-chip clock */
#define HT1632_CMD_RCCLK  0x18  /* CMD= 0001-10xx-x Master Mode, external clock */
#define HT1632_CMD_EXTCLK 0x1C	/* CMD= 0001-11xx-x Use external clock */
#define HT1632_CMD_COMS00 0x20	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS01 0x24	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS10 0x28	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS11 0x2C	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_PWM_T  0xA0	/* CMD= 101x-PPPP-x PWM duty cycle - template*/
#define HT1632_CMD_PWM(lvl) (HT1632_CMD_PWM_T | (lvl-1))
  /* Produces the correct command from the given value of lvl. lvl = [0..15] */
#define HT1632_CMD_LEN    8	/* Commands are 8 bits long, excluding the trailing bit */
#define HT1632_ADDR_LEN   7	/* Addresses are 7 bits long */
#define HT1632_WORD_LEN   4     /* Words are 4 bits long */

class HT1632Class
{
  private:  
    uint8_t _pinCS [4];
    uint8_t _numActivePins;
    uint8_t _pinWR;
    uint8_t _pinDATA;
    uint8_t _pinCLK;
    uint8_t _currSelectionMask;
    uint8_t _tgtRender;
    uint8_t _tgtChannel;
    byte * mem [5];
    void writeCommand(char);
    void writeData(byte, uint8_t);
    void writeDataRev(byte, uint8_t);
    void writeSingleBit();
    void initialize(uint8_t, uint8_t);
    void select();
    void select(uint8_t mask);
    inline void pulseCLK();
    void setCLK(uint8_t pinCLK);
    void sendCommand(uint8_t command);
    void renderTarget(uint8_t targetScreen);
    
  public:
    void begin(uint8_t pinCS1, uint8_t pinWR,  uint8_t pinDATA);
    void render();
    void clear();
    void fill();
    void setBrightness(char brightness, char selectionmask = 0b00010000);
    
	// modified for JY-MCU
	// lower level support:
    void setPixel(uint8_t x, uint8_t y);
    void clearPixel(uint8_t x, uint8_t y);

	// backwards compatibility:
    void drawImage(const byte img [], uint8_t width, uint8_t height, int8_t x, int8_t y, int offset = 0);

	// added for JY-MCU
    void writePixel(uint8_t x, uint8_t y, uint8_t val);
    void writeChar(byte pos, byte val, byte cols);
    void printChar(byte pos, byte val);
    void printString(const char* str);
    void printNum(long val, byte base);
    void setDisplayColumn(byte pos, byte val);

};

// end class prototypes

#else
//#error "HT1632.h" already defined!
#endif
