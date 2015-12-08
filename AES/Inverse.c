#include "Inverse.h"

Inverse::Inverse(char *p)
{
  if(strlen(p) != 8)
  {
    fprintf(stderr,"%s is wrong length\n",p);
    exit(1);
  }
  for(int i = 0; i<4; i++)
    poly[3-i] = ModProd::convertHex(&(p[i*2]));

  computeByteInverse();
}

//find byte inverse of every byte
void Inverse::computeByteInverse()
{
  //printf("byte inverse");
  for(int i = 1; i<256; i++)
  {
    for(int j = 1; j<256; j++)
    {
      if(ModProd::multi((unsigned char)i,(unsigned char)j)==0x01)
      {
	byteInverse[i] = (unsigned char)j;
	break;
      }
    }
  }
}

//find remainder and quotient for speical case where the number being divided is 5 digits, 10001
void Inverse::initRemQuo(unsigned char divIn[4], unsigned char quo[4], unsigned char rem[4])
{
  unsigned char div[5];
  div[4] = 0;
  memcpy(div,divIn,4);

  //printf("init rem quo\n");
  unsigned char num[5] = {1,0,0,0,1};
  int powerNum,powerDiv,powerDiff;//power of num,div,difference between powers of num and div
  powerNum = 4;
  for(int i = 4; i>=0; i--)
  {
    if(div[i]!=0)
    {
      powerDiv = i;
      break;
    }
  }
  powerDiff = powerNum-powerDiv;

  //printf("powernum %i powerdiv %i powerdiff %i\n",powerNum,powerDiv,powerDiff);

  memset(quo,0,4);
  memset(rem,0,4);
  unsigned char workingNum[5];
  memcpy(workingNum,num,5);

  /*printf("num");
  for(int i = 0; i<5; i++)
    printf("%02x ",num[4-i]);
  
  printf("\ndiv");
  for(int i = 0; i<5; i++)
    printf("%02x ",div[4-i]);
    printf("\n");*/

  for(int i = 0; i<=powerDiff; i++)//degree of quotient is = to powerdiff
  {
    //printf("\nworking %02x div %02x\n",workingNum[powerNum-i],div[powerDiv]);
    //quo = num/div
    quo[powerDiff-i] = ModProd::multi(workingNum[powerNum-i],byteInverse[div[powerDiv]]);

    //printf("quo %i %02x\n",powerDiff-i,quo[powerDiff-i]);

    unsigned char tmp[5];
    memset(tmp,0,5);
    unsigned char quoTmp[4] = {quo[powerDiff-i],0,0,0};
    ModProd::polyMult(quoTmp,div,tmp);//quotient X div

    /*printf("tmp ");
    for(int j = 4; j>=0; j--)
      printf("%02x ",tmp[j]);
      printf("\n");*/

    for(int j = i; j<powerDiff; j++)//shift tmp over until its heighest power si the same as working's
    {
      unsigned char tChar = tmp[4];
      for(int k = 4; k>0; k--)
	tmp[k] = tmp[k-1];
      tmp[0] = tChar;
    }

    /*printf("tmp ");
    for(int j = 4; j>=0; j--)
      printf("%02x ",tmp[j]);
    printf("\n");

    printf("working ");
    for(int j = 4; j>=0; j--)
      printf("%02x ",workingNum[j]);
      printf("\n");*/
    
    for(int j = 4; j>=0; j--)//num - (quo*div)
      workingNum[j] = workingNum[j] ^ tmp[j];

    /*printf("working ");
    for(int j = 4; j>=0; j--)
      printf("%02x ",workingNum[j]);
      printf("\n");*/
  }
  memcpy(rem,workingNum,4);
  
  //exit(1);
}

//compute normal case of num/div where each is 4 byte array
void Inverse::computeRemQuo(unsigned char num[4], unsigned char div[4], unsigned char quo[4], unsigned char rem[4])
{
  //printf("compute rem quo\n");
  int powerNum,powerDiv,powerDiff;

  /*printf("num");
  for(int i = 0; i<4; i++)
    printf("%02x ",num[3-i]);
  
  printf("\ndiv");
  for(int i = 0; i<4; i++)
    printf("%02x ",div[3-i]);
    printf("\n");*/

  for(int i = 3; i>=0; i--)//ighest power of Num
  {
    if(num[i]!=0)
    {
      powerNum = i;
      break;
    }
  }
  for(int i = 3; i>=0; i--)//highest power of div
  {
    if(div[i]!=0)
    {
      powerDiv = i;
      break;
    }
  }
  powerDiff = powerNum-powerDiv;//difference between teh 2 inputs highest powers

  //printf("powernum %i powerDiv %i powerDiff %i\n",powerNum,powerDiv,powerDiff);

  memset(quo,0,4);
  memset(rem,0,4);
  unsigned char workingNum[4];//workign copy of num
  memcpy(workingNum,num,4);
  
  for(int i = 0; i<=powerDiff; i++)//degree of quotient = powerdiff
  {
    //printf("\nworking %02x div %02x\n",workingNum[powerNum-i],div[powerDiv]);
    //quo = workingNum/div
    quo[powerDiff-i] = ModProd::multi(workingNum[powerNum-i],byteInverse[div[powerDiv]]);

    //printf("quo %i %02x\n",powerDiff-i,quo[powerDiff-i]);

    unsigned char tmp[4];
    unsigned char quoTmp[4] = {quo[powerDiff-i],0,0,0};
    ModProd::polyMult(quoTmp,div,tmp);//quotient * div
    /*printf("tmp ");
    for(int j = 0; j<4; j++)
    printf("%02x ",tmp[3-j]);*/
    
    for(int j = i; j<powerDiff; j++)//rotate tmp until its same degree as num
    {
      for(int k = 0; k<3; k++)
	KeyExpand::rotWord(tmp);
    }

    /*printf("\ntmp ");
    for(int j = 0; j<4; j++)
    printf("%02x ",tmp[3-j]);*/

    //printf("\nworking ");
    //for(int j = 3-i; j>=3-powerDiff; j--)
    for(int j = 0; j<4; j++)// working num = working num - (quo*div)
    {
      workingNum[j] = workingNum[j] ^ tmp[j];
      //printf("%02x ",workingNum[j]);
    }
    /*for(int j = 0; j<4; j++)
      printf("%02x ",workingNum[3-j]);
      printf("\n");*/
  }
  memcpy(rem,workingNum,4);
  //exit(1);
}

//special case of num/div where we want remainder to be 1
void Inverse::finalRemQuo(unsigned char num[4], unsigned char div[4], unsigned char quo[4], unsigned char rem[4])
{
  int powerNum,powerDiv,powerDiff;

  for(int i = 3; i>=0; i--)
  {
    if(num[i]!=0)
    {
      powerNum = i;
      break;
    }
  }
  for(int i = 3; i>=0; i--)
  {
    if(div[i]!=0)
    {
      powerDiv = i;
      break;
    }
  }
  powerDiff = powerNum-powerDiv;

  memset(quo,0,4);
  memset(rem,0,4);
  unsigned char workingNum[4];
  memcpy(workingNum,num,4);
  
  for(int i = 0; i<powerDiff; i++)//same as normal case until we get to last digit
  {
    //printf("\nworking %02x div %02x\n",workingNum[powerNum-i],div[powerDiv]);
    quo[powerDiff-i] = ModProd::multi(workingNum[powerNum-i],byteInverse[div[powerDiv]]);

    //printf("quo %i %02x\n",powerDiff-i,quo[powerDiff-i]);

    unsigned char tmp[4];
    unsigned char quoTmp[4] = {quo[powerDiff-i],0,0,0};
    ModProd::polyMult(quoTmp,div,tmp);
    /*printf("tmp ");
    for(int j = 0; j<4; j++)
    printf("%02x ",tmp[3-j]);*/
    

    for(int j = i; j<powerDiff; j++)
    {
      for(int k = 0; k<3; k++)
	KeyExpand::rotWord(tmp);
    }

    /*printf("\ntmp ");
    for(int j = 0; j<4; j++)
    printf("%02x ",tmp[3-j]);*/

    //printf("\nworking ");
    //for(int j = 3-i; j>=3-powerDiff; j--)
    for(int j = 0; j<4; j++)
    {
      workingNum[j] = workingNum[j] ^ tmp[j];
      //printf("%02x ",workingNum[j]);
    }
    /*for(int j = 0; j<4; j++)
      printf("%02x ",workingNum[3-j]);
      printf("\n");*/
  }
  
  //quotient = (num-1)/div
  quo[0] = ModProd::multi(0x01 ^ workingNum[0],byteInverse[div[powerDiv]]);

  unsigned char tmp[4];
  unsigned char quoTmp[4] = {quo[0],0,0,0};
  ModProd::polyMult(quoTmp,div,tmp);

  for(int j = 0; j<4; j++)//num = num-(quo*div)
    workingNum[j] = workingNum[j] ^ tmp[j];
  
  memcpy(rem,workingNum,4);
}

//performs table method
void Inverse::run()
{
  //printf("run\n");
  unsigned char rem[6][4];//remainder column in table method
  unsigned char x[6][4];//auxilliary column in table method
  unsigned char quo[6][4];//quotient column in table method

  memset(x[0],0,4);//set initial values for i= 1,2 of table method

  memset(x[1],0,4);
  x[1][0] = 1;

  memset(quo[0],0,4);

  memset(quo[1],0,4);

  memset(rem[0],0,4);
  rem[0][0] = 1;

  memcpy(rem[1],poly,4);

  initRemQuo(rem[1],quo[2],rem[2]);//special case of i=3 since rem[0] is actually 1,0,0,0,1

  ModProd::polyMult(quo[2],x[1],x[2]);
  for(int j = 0; j<4; j++)
    x[2][j] = x[2][j]^x[0][j];
  
  for (int i=3; i<6; i++) 
  {
    //printf("i=%i\n",i+1);
    if(rem[i-1][3]==0 && rem[i-1][2]==0 && rem[i-1][1]==0)//when remainder only has 1 digit left, were on last step
    {
      //printf("six hit\n");
      finalRemQuo(rem[i-2],rem[i-1],quo[i],rem[i]);
    }
    else
      computeRemQuo(rem[i-2],rem[i-1],quo[i],rem[i]);
    
    ModProd::polyMult(quo[i],x[i-1],x[i]);
    for(int j = 0; j<4; j++)
      x[i][j] = x[i][j]^x[i-2][j];

    if(rem[i][0]==0 && rem[i][1]==0 && rem[i][2]==0 && (rem[i][3]==0 || rem[i][3]==1))//stop if remainder = 1 or 0
      break;
  }

  for(int i = 0; i<6; i++)//print each round
  {
    printf("i=%i, rem[i]=",i+1);

    for(int j = 0; j<4; j++)
      printf("{%02x}",rem[i][3-j]);

    printf(", quo[i]=");

    for(int j = 0; j<4; j++)
      printf("{%02x}",quo[i][3-j]);

    printf(", aux[i]=");

    for(int j = 0; j<4; j++)
      printf("{%02x}",x[i][3-j]);

    printf("\n");

    if(i>2 && rem[i][0]==0 && rem[i][1]==0 && rem[i][2]==0 && rem[i][3]==0)//if remainder = 0
    {
      for(int j = 0; j<4; j++)
	printf("{%02x}",rem[1][3-j]);
      printf(" does not have a multiplicative inverse.\n");
      break;
    }

    if(i>2 && rem[i][0]==1 && rem[i][1]==0 && rem[i][2]==0 && rem[i][3]==0)//if remainder = 1
    {
      printf("Multiplicative inverse of ");
      for(int j = 0; j<4; j++)
	printf("{%02x}",rem[1][3-j]);
      printf(" is ");
      for(int j = 0; j<4; j++)
      printf("{%02x}",x[i][3-j]);
      printf("\n");
      break;
    }
  }
}
