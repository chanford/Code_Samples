#include "TableCheck.h"

TableCheck::TableCheck(char *tf)
{
  in = fopen(tf,"r");//check the tablefile
  if(in==0)
  {
    fprintf(stderr,"Invalid table file\n");
    exit(1);
  }
  sSet = 0;
  pSet = 0;
  invpSet = 0;
  eofFound = 0;
}

void TableCheck::run()
{
  for(int i = 0; i<3; i++)//for the expected 3 tables
  {
    readLine();
    if(eofFound == 1)
    {
      fprintf(stderr,"not enough tables in tablefile\n");
      exit(1);
    }
    int head = getHeader();
    if(head==1)
      processS();
    if(head==2)
      processP();
    if(head==3)
      processINVP();
    if(pSet==1 && invpSet==1)//once p and invp are set,we need to make sure they are inverses
      processPINVP();
  }
  fclose(in);
}

//reads in a line
void TableCheck::readLine()
{
  int eofCheck;
  char cur;
  int eqIndex = -1;
  int numRead;
  for(numRead = 0; numRead<1024; numRead++)
  {
    eofCheck = getc(in);
    if(eofCheck == EOF)//check for end of file
    {
      eofFound = 1;
      return;
    }
    cur = (char) eofCheck;
    if(cur=='\n')//end of line
    {
      currLine[numRead] = '\0';
      break;
    }
    if(cur=='=' && eqIndex==-1)//note down the first place = is found. this divides the header aprt of the table and teh table data
      eqIndex = numRead;
    currLine[numRead] = cur;
  }
  //printf("numread %i %i %i %s\n",numRead,hexSize,eqIndex,currLine);
  hexSize = 0;
  for(int i = eqIndex+1; i<numRead; i+=2)//process each hex value in the input and turn them into bytes
  {
    //printf("%i %i %i\n",i,numRead,hexSize);
    if(i+1 == numRead)
    {
      fprintf(stderr,"odd amount of hex digits in %s\n",currLine);
      exit(1);
    }
    currHex[(i-eqIndex-1)/2] = ModProd::convertHex(&(currLine[i]));
    //printf("read value %i %hhu\n",(i-eqIndex-1)/2,currHex[(i-eqIndex-1)/2]);
    hexSize++;
  }
  //printf("hexSize is %i\n",hexSize);
}

//returns which table the current line represents
int TableCheck::getHeader()
{
  if(strncmp(currLine,"S=",strlen("S=")) == 0)
    return 1;
  if(strncmp(currLine,"P=",strlen("P=")) == 0)
    return 2;
  if(strncmp(currLine,"INVP=",strlen("INVP=")) == 0)
    return 3;
  fprintf(stderr,"not a valid table type %s\n",currLine);
  exit(1);
  return -1;
}

//proces s table
void TableCheck::processS()
{
  if(hexSize != 256)
  {
    fprintf(stderr,"S table has wrong number of values %i\n",hexSize);
    exit(1);
  }
  unsigned char check[256];
  memset(check,0,256);

  for(int i = 0; i<256; i++)
  {
    unsigned char tmp = currHex[i];
    if(check[tmp]==1)//table shouldnt have repeat values
    {
      fprintf(stderr,"repeat value %hhu in S table\n",tmp);
      exit(1);
    }
    check[tmp] = 1;
    s[i] = tmp;
  }
  sSet = 1;
}

//process p table
void TableCheck::processP()
{
  if(hexSize != 4)
  {
    fprintf(stderr,"P table has wrong number of values %i\n",hexSize);
    exit(1);
  }
  for(int i = 0; i<4; i++)
    p[3-i] = currHex[i];
  pSet = 1;
}

//process invp table
void TableCheck::processINVP()
{
  if(hexSize != 4)
  {
    fprintf(stderr,"INVP table has wrong number of values %i\n",hexSize);
    exit(1);
  }
  for(int i = 0; i<4; i++)
    invp[3-i] = currHex[i];
  pSet = 1;
}

//check if p*invp = 1
void TableCheck::processPINVP()
{
  unsigned char result[4];
  ModProd::polyMult(p,invp,result);
  if(result[0]!=1 || result[1]!=0 || result[2]!=0 || result[3]!=0)
  {
    fprintf(stderr,"modprod of p and invp indicates they are not inverses\n");
    exit(1);
  }
}
