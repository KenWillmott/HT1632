#include "ClockMatrix.h"

//************************************************
// extend HT1632Class with clock specific functions
//

// set colon and blank spaces explicitly
//
void ClockMatrix::setColon()
{
      setDisplayColumn(6, 0);

      setDisplayColumn(13, 0);
      setDisplayColumn(14, 0);

      setDisplayColumn(15, 0x66);
      setDisplayColumn(16, 0x66);

      setDisplayColumn(17, 0);
      setDisplayColumn(18, 0);

      setDisplayColumn(25, 0);
}

void ClockMatrix::print2digitsLower(int val, boolean leadingZero)
{
  writeChar(26, val % 10 + BIGNUM_OFFSET, 6);
  if (leadingZero || val >= 10)
    writeChar(19, val / 10 + BIGNUM_OFFSET, 6);
  else
    writeChar(19, ' ', 6);
}

void ClockMatrix::print2digitsUpper(int val, boolean leadingZero)
{
  writeChar(7, val % 10 + BIGNUM_OFFSET, 6);
  if (leadingZero || val >= 10)
    writeChar(0, val / 10 + BIGNUM_OFFSET, 6);
  else
    writeChar(0, ' ', 6);
}


// end of class definitions
