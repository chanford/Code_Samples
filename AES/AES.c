#include "AES.h"

//i=column number, j = row number

AES::AES(char *k, char *tf, char *fn, char fnSet, char isDec)
{
  if(fnSet==0)
    fileIn = stdin;
  else
  {
    fileIn = fopen(fn,"r");
    if(fileIn==0)
    {
      fprintf(stderr,"Invalid input file\n");
      exit(1);
    }
  }

  isDecrypt = isDec;

  TableCheck *tc = new TableCheck(tf);
  tc->run();
  memcpy(s,tc->s,256);
  memcpy(p,tc->p,4);
  memcpy(invp,tc->invp,4);
  delete tc;

  KeyExpand *ke = new KeyExpand(k,tf,1);
  ke->run();
  //memcpy(w,ke->w,4*Nb*(Nr+1));
  for(int i = 0; i<Nb*(Nr+1); i++)
  {
    for(int j = 0; j<4; j++)
      w[i][j] = ke->w[i][j];
  }
  delete ke;

  if(isDecrypt==1)
  {
    for(int i = 0; i<256; i++)
      is[s[i]] = i;
  }
}

void AES::run()
{
  unsigned char in[4*Nb];
  unsigned char out[4*Nb];
  getBytes(in);
  if(isDecrypt==0)
    AES_encrypt(in,out);
  else
    AES_decrypt(in,out);
}

//get bytes from input
void AES::getBytes(unsigned char *in)
{
  for(int i = 0; i<16; i++)
  {
    int eofCheck = getc(fileIn);
    if(eofCheck==EOF)
    {
      fprintf(stderr,"not enough bytes in input\n");
      exit(1);
    }
    in[i] = (unsigned char)eofCheck;
    //printf("%02x",in[i]);
  }
}

//print current state after performing the operation provided by header
void AES::printState(unsigned char a[4][4],char const *head,int round)
{
  if(isDecrypt==1)
    round = 10-round;
  printf("round[");
  if(round<10)
    printf(" ");
  printf("%i].",round);
  if(isDecrypt==0)
  {
    if(head[0]=='o')
      printf("%s   ",head);
    else
      printf("%s    ",head);
  }
  else
  {
    if(head[1]=='o')
      printf("%s  ",head);
    else
      printf("%s   ",head);
  }
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      printf("%02x",a[i][j]);
  }
  printf("\n");
}

//encrypt in
void AES::AES_encrypt(unsigned char *in, unsigned  char *out)
{
  char const *start = "start";
  char const *input = "input";
  char const *s_box = "s_box";
  char const *s_row = "s_row";
  char const *s_col = "m_col";
  char const *outputT = "output";

  unsigned char state[4][Nb];
  //memcpy(state,in,4*Nb);
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      state[i][j] = in[4*i + j];
  }

  printState(state,input,0);

  addRoundKey(state,0);


  for(int round = 1; round<Nr; round++)
  {
    printState(state,start,round);
    subBytes(state);
    printState(state,s_box,round);
    shiftRows(state);
    printState(state,s_row,round);
    mixColumns(state);
    printState(state,s_col,round);
    addRoundKey(state,round);
  }
  printState(state,start,10);
  subBytes(state);
  printState(state,s_box,10);
  shiftRows(state);
  printState(state,s_row,10);
  addRoundKey(state,10);
  printState(state,outputT,10);
  memcpy(out,state,4*Nb);
}

//add round key to state
void AES::addRoundKey(unsigned char state[4][4],int round)
{
  unsigned char roundKey[4][4];
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      roundKey[i][j] = w[4*round + i][j];
  }
  char const *k_sch;
  if(isDecrypt==0)
    k_sch = "k_sch";
  else
    k_sch = "ik_sch";
  printState(roundKey,k_sch,round);
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      state[i][j] = state[i][j] ^ roundKey[i][j];
  }
}

//substitute bytes in state
void AES::subBytes(unsigned char state[4][4])
{
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      state[i][j] = s[state[i][j]];
  }
}

//shift rows in state
void AES::shiftRows(unsigned char state[4][4])
{
  for(int j = 0; j<4; j++)
  {
    for(int k = 0; k<j; k++)
    {
      unsigned char tmp = state[0][j];
      for(int i = 0; i<3; i++)
	state[i][j] = state[i+1][j];
      state[3][j] = tmp;
    }
  }
}

//mix columns in state
void AES::mixColumns(unsigned char state[4][4])
{
  for(int i = 0; i<4; i++)
  {
    unsigned char result[4];

    ModProd::polyMult(p,state[i],result);

    for(int j = 0; j<4; j++)
      state[i][j] = result[j];
  }
}

//decrypt in
void AES::AES_decrypt(unsigned char *in, unsigned  char *out)
{
  char const *start = "istart";
  char const *input = "iinput";
  char const *s_box = "is_box";
  char const *s_row = "is_row";
  //char const *s_col = "im_col";
  char const *outputT = "ioutput";
  char const *ik_add = "ik_add";

  unsigned char state[4][Nb];
  //memcpy(state,in,4*Nb);
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      state[i][j] = in[4*i + j];
  }

  printState(state,input,10);

  addRoundKey(state,10);

  for(int round = Nr-1; round>=1; round--)
  {
    printState(state,start,round);

    iShiftRows(state);
    printState(state,s_row,round);

    iSubBytes(state);
    printState(state,s_box,round);
    
    addRoundKey(state,round);
    printState(state,ik_add,round);

    iMixColumns(state);
    //printState(state,s_col,round);
    
  }

  printState(state,start,0);

  iShiftRows(state);
  printState(state,s_row,0);

  iSubBytes(state);
  printState(state,s_box,0);
  
  addRoundKey(state,0);

  printState(state,outputT,0);
  memcpy(out,state,4*Nb);
}

//inverse substitution
void AES::iSubBytes(unsigned char state[4][4])
{
  for(int i = 0; i<4; i++)
  {
    for(int j = 0; j<4; j++)
      state[i][j] = is[state[i][j]];
  }
}

//inverse shift rows
void AES::iShiftRows(unsigned char state[4][4])
{
  for(int j = 0; j<4; j++)
  {
    for(int k = 0; k<j; k++)
    {
      unsigned char tmp = state[3][j];
      for(int i = 3; i>0; i--)
	state[i][j] = state[i-1][j];
      state[0][j] = tmp;
    }
  }
}

//inverse mix columns
void AES::iMixColumns(unsigned char state[4][4])
{
  for(int i = 0; i<4; i++)
  {
    unsigned char result[4];

    ModProd::polyMult(invp,state[i],result);

    for(int j = 0; j<4; j++)
      state[i][j] = result[j];
  }
}
