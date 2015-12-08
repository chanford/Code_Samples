#include "Inverse.h"
int main(int argc,char **argv)
{
  char badCom = 0;//bad command
  char mode = 0;//1=tablecheck,2=modprod,3=keyexpand,4=encrypt,5=decrypt,6=inverse
  char *key;//text representation of key
  char keySet = 0;
  char *fn;//file name when file is input
  char fnSet = 0;
  char *table;//table file name
  char tableSet = 0;
  char *p1;//polynomial 1
  char p1Set = 0;
  char pSet = 0;
  char *p2;//polynomial 2
  char p2Set = 0;

  if(argc<3 || argc>5)//valid commands have >2 and <6 parameters
    badCom = 1;

  for(int i = 1; i<argc; i++)//for each parameter besides hw6
  {
    char *tmp = argv[i];
    if(strncmp(tmp,"tablecheck",strlen("tablecheck")+5)==0 && mode==0)
    {
      mode = 1;
      continue;
    }
    if(strncmp(tmp,"modprod",strlen("modprod")+5)==0 && mode==0)
    {
      mode = 2;
      continue;
    }
    if(strncmp(tmp,"keyexpand",strlen("keyexpand")+5)==0 && mode==0)
    {
      mode = 3;
      continue;
    }
    if(strncmp(tmp,"encrypt",strlen("encrypt")+5)==0 && mode==0)
    {
      mode = 4;
      continue;
    }
    if(strncmp(tmp,"decrypt",strlen("decrypt")+5)==0 && mode==0)
    {
      mode = 5;
      continue;
    }
    if(strncmp(tmp,"inverse",strlen("inverse")+5)==0 && mode==0)
    {
      mode = 6;
      continue;
    }
    if(strncmp(tmp,"-k=",3)==0)//set key
    {
      key = &(tmp[3]);
      keySet = 1;
      continue;
    }
    if(strncmp(tmp,"-t=",3)==0)//set table file name
    {
      table = &(tmp[3]);
      struct stat s;
      if(stat(table, &s) == -1)//check to see if table file name
      {
	fprintf(stderr,"invalid file %s\n",table);
	return 0;
      }

      tableSet = 1;
      continue;
    }
    if(strncmp(tmp,"-p1=",4)==0)//set polynomial
    {
      p1 = &(tmp[4]);
      p1Set = 1;
      continue;
    }
    if(strncmp(tmp,"-p2=",4)==0)//set polynomial
    {
      p2 = &(tmp[4]);
      p2Set = 1;
      continue;
    }
    if(strncmp(tmp,"-p=",3)==0)//set polynomial
    {
      p1 = &(tmp[3]);
      pSet = 1;
      continue;
    }
    if(tmp[0]=='-')//unknown - parameter
    {
      fprintf(stderr,"unknown option %s\n",tmp);
      return 0;
    }
    struct stat s;
    if(stat(tmp, &s) == -1)//check filename validity
    {
      fprintf(stderr,"invalid file %s\n",tmp);
      return 0;
    }
    //printf("%s\n",fn);
    fn = argv[i];
    fnSet = 1;
  }

  if(mode==0)
    badCom = 1;

  if(mode==1)
  {
    if(tableSet==0 || argc!=3)//tablecheck must have table filename set
      badCom = 1;
    if(badCom!=1)
    {
      TableCheck *tc = new TableCheck(table);
      tc->run();
      delete(tc);
    }
  }
  if(mode==2)
  {
    if(p1Set == 0 || p2Set == 0 || argc!= 4)//modprod needs both plynomials set
      badCom = 1;
    if(badCom!=1)
    {
      ModProd *mp = new ModProd(p1,p2);
      mp->run();
      delete(mp);
    }
  }
  if(mode==3)
  {
    if(keySet == 0 || tableSet == 0 || argc != 4)//key expand needs a key and table file set
      badCom = 1;
    if(badCom!=1)
    {
      KeyExpand *ke = new KeyExpand(key,table,0);
      ke->run();
      delete(ke);
    }
  }
  if(mode==4 || mode==5)//encryption/decryption
  {
    if(keySet == 0 || tableSet == 0)//needs key and table set
      badCom = 1;
    if((fnSet==0 && argc!=4) || (fnSet==1 && argc!=5))//without/with filename
      badCom = 1;
    AES *aes;
    if(badCom!=1)
    {
      if(mode==4)
	aes = new AES(key,table,fn,fnSet,0);//encrypt
      else
	aes = new AES(key,table,fn,fnSet,1);//decrypt
      aes->run();
      delete(aes);
    }
  }
  if(mode==6)
  {
    if(pSet==0 || argc!=3)//inverse needs polynomial set
      badCom = 1;
    if(badCom != 1)
    {
      Inverse *i = new Inverse(p1);
      i->run();
      delete(i);
    }
  }
  if(badCom==1)
  {
    fprintf(stderr,"Invalid command. Valid commands are:\n");
    fprintf(stderr,"hw6 tablecheck -t=tablefile\n");
    fprintf(stderr,"hw6 modprod -p1=poly1 -p2=poly2\n");
    fprintf(stderr,"hw6 keyexpand -k=key -t=tablefile\n");
    fprintf(stderr,"hw6 encrypt -k=key -t=tablefile [file]\n");
    fprintf(stderr,"hw6 decrypt -k=key -t=tablefile [file]\n");
    fprintf(stderr,"hw6 inverse -p=poly\n");
  }
}
