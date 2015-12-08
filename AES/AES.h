#include "KeyExpand.h"
//performs aes encryption/decryption on the first 16 bytes of the input
class AES
{
 public:
  AES(char *k, char *tf, char *fn, char fnSet, char isDec);
  void run();
 private:
  char isDecrypt;//1 if in decryption mode
  unsigned char w[Nb*(Nr+1)][4];//subkeys
  unsigned char s[256];//s table
  unsigned char is[256];//inverted s table
  unsigned char p[4];//p table
  unsigned char invp[4];//invp table
  FILE *fileIn;//file being read

  void getBytes(unsigned char *in);//get 16 bytes from fileIn
  void AES_encrypt(unsigned char *in, unsigned  char *out);//encrypts in and puts results in out
  void AES_decrypt(unsigned char *in, unsigned  char *out);//decrypts in and puts results in out
  void addRoundKey(unsigned char state[4][4],int round);//adds the roudnkeys specified by i into the provided state
  void subBytes(unsigned char state[4][4]);//performs byte substitution on state
  void iSubBytes(unsigned char state[4][4]);//performs inverse byte subsittution on state
  void shiftRows(unsigned char state[4][4]);//performs shift row operation on state
  void iShiftRows(unsigned char state[4][4]);//performs inverse shift row operation on state
  void mixColumns(unsigned char state[4][4]);//performs mix Column operation on state
  void iMixColumns(unsigned char state[4][4]);//performs inverse mix column operation on state
  void printState(unsigned char a[4][4],char const *head,int round);//prints out the state and operation information
};
