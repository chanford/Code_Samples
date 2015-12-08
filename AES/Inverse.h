#include "AES.h"
//finds inverse of the polynomical fiven to it
class Inverse
{
 public:
  Inverse(char *p);
  void run();
 private:
  unsigned char poly[4];//the polynomial given to work on
  unsigned char byteInverse[256];// x dot product byteInverse[x] = 1

  void computeByteInverse();//fills out byteInverse array
  void computeRemQuo(unsigned char num[4], unsigned char div[4], unsigned char quo[4], unsigned char rem[4]);//computes num/div and num%div
  void finalRemQuo(unsigned char num[4], unsigned char div[4], unsigned char quo[4], unsigned char rem[4]);//computes num/div when we want remainder to be 1
  void initRemQuo(unsigned char divIn[4], unsigned char quo[4], unsigned char rem[4]);//computes the remainder and quotient for the i=3 special case where num = 1,0,0,0,1
};
