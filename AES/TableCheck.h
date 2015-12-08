#include "ModProd.h"
//checks a table file to make sure all tables are valid
class TableCheck
{
 public:
  TableCheck(char *tf);
  void run();
  unsigned char s[256];//s table
  unsigned char p[4];//p table
  unsigned char invp[4];//invp table

 private:
  char currLine[1024];//current line of text being processed
  unsigned char currHex[256];//hex values gotten from currLine
  unsigned int hexSize;//number of values in currHex for this line
  FILE *in;//the file being red
  char sSet;//if s table has been set
  char pSet;//if p table has been set
  char invpSet;//if invp table has been set
  char eofFound;//if end of file has been reached

  void readLine();//read next line from file and put in currLine and convert to currHex
  int getHeader();//returns which table the current line represents
  void processS();//checks S table for validity
  void processP();//checks p table for validity
  void processINVP();//checks invp table for validity
  void processPINVP();//checks if P*INVP=1
};
