#include "KeyExpand.h"

KeyExpand::KeyExpand(char *k, char *tf, char q)
{
  if(strlen(k)!=32)
  {
    fprintf(stderr,"Wrong number of hex numbers in key %i\n",strlen(k));
    exit(1);
  }
  for(int i = 0; i<16; i++)
    key[i] = ModProd::convertHex(&(k[i*2]));

  TableCheck *tb = new TableCheck(tf);
  tb->run();
  memcpy(s,tb->s,256);
  delete tb;

  quiet = q;
}

void KeyExpand::run()
{
  for(int i = 0; i<Nk; i++)//put initial key into w
  {
    for(int j = 0; j<4; j++)
      w[i][j] = key[4*i + j];
  }

  for(int i = Nk; i< (Nb * (Nr + 1)); i++)//get each subkey
  {
    //printf("i %i ",i);

    unsigned char tmp[4];
    memcpy(tmp,w[i-1],4);
    //printf(" tmp ");
    //printWord(tmp);

    if(i%Nk == 0)
    {
      rotWord(tmp);//rotate
      //printf(" rotword ");
      //printWord(tmp);

      subWord(tmp);//substitute
      //printf(" sub ");
      //printWord(tmp);

      unsigned char r[4];
      getR(r,i/Nk);//get r
      //printf(" r ");
      //printWord(r);

      xorWord(tmp,r,tmp);//xor tmp with r
      //printf(" xor r ");
      //printWord(tmp);
    }
    
    //printf(" w[i-Nk] ");
    //printWord(w[i-Nk]);

    xorWord(w[i-Nk],tmp,w[i]);
    //printf(" w[i] ");
    //printWord(w[i]);
    //printf("\n");
    
  }
  if(quiet==0)//print results of the current round
  {
    for(int i = 0; i<(Nb * (Nr + 1)); i++)
      printRound(i);
  }
}

void KeyExpand::printWord(unsigned char *a)//prints out a word
{
  for(int i = 0; i<4; i++)
    printf("%02x",a[i]);
}

//print results of a round
void KeyExpand::printRound(int i)
{
  printf("w[");
  if(i<10)
    printf(" ");
  printf("%i]: ",i);
  printWord(w[i]);
  printf("\n");
}

//performs substitution on word
void KeyExpand::subWord(unsigned char *a)
{
  for(int i = 0; i<4; i++)
    a[i] = s[a[i]];
}

//rotate word to left by 1
void KeyExpand::rotWord(unsigned char *a)
{
  unsigned char tmp = a[0];
  for(int i = 0; i<3; i++)
    a[i] = a[i+1];
  a[3] = tmp;
}

//xor 2 words
void KeyExpand::xorWord(unsigned char *a, unsigned char *b,unsigned char *r)
{
  for(int i = 0; i<4; i++)
    r[i] = a[i]^b[i];
}

//derive R[i];
void KeyExpand::getR(unsigned char *r,int i)
{
  r[0] = 1;
  r[1] = 0;
  r[2] = 0;
  r[3] = 0;
  for(int j = 0; j<i-1; j++)
    r[0] = ModProd::multi(r[0],0x02);
}
