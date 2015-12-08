#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>
#include <limits.h>
#include <assert.h>
using namespace std;

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

const int AES_KEY_LENGTH_IN_BITS = 128;
const int AES_KEY_LENGTH_IN_CHARS = 128 / CHAR_BIT;
//class for aes functions provided by aes.c
class AESF
{
 public:
  static void class_AES_set_encrypt_key(unsigned char *, AES_KEY *);

  static void class_AES_set_decrypt_key(unsigned char *, AES_KEY *);

  static void class_AES_encrypt_with_padding(unsigned char *, int , unsigned char **, int *, AES_KEY *);

  static void class_AES_decrypt_with_padding(unsigned char *, int , unsigned char **, int *, AES_KEY *);

  static void testKeys(AES_KEY *, AES_KEY *);//test that enc/dec key pair work
};
