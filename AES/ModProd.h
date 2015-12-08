#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
//finds modular product of 2 polynomials and provides related functions to other classes
class ModProd
{
 public:
  ModProd(char *p1, char *p2);
  void run();
  static void polyMult(unsigned char *a, unsigned char *b, unsigned char *result);//perfroms polynomical multiplication on 2 4 byte arrays, puts result in result
  static unsigned char convertHex(char *check);//converts check[0],check[1] into its hex value
  static unsigned char xtime(unsigned char a);//performs xtime operation on a and returns the result
  static unsigned char multi(unsigned char a, unsigned char b);//performs dot product multiplication on a and b and returns result
 private:
  unsigned char poly1[4];//p1 during ./hw6 modprod
  unsigned char poly2[4];//p2 during ./hw6 modprod
};
