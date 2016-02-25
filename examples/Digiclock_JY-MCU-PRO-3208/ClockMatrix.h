// ClockMatrix.h
//
//************************************************
// extend HT1632Class with clock specific functions
//************************************************

#include <HT1632.h>

class ClockMatrix : public HT1632Class
{
  public:

    void print2digitsUpper(int val, boolean leadingZero);
    void print2digitsLower(int val, boolean leadingZero);
    void setColon();

  private:
    //
};

// end class

