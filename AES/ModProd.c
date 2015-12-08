#include "ModProd.h"

ModProd::ModProd(char *p1, char *p2)
{
  if(strlen(p1)!=8 || strlen(p2)!=8)
  {
    fprintf(stderr,"Invalid polynomial provided\n");
    exit(1);
  }
  for(int i = 0; i<4; i++)//turn the string polynomials into hex values
  {
    poly1[3-i] = convertHex(&(p1[i*2]));
    poly2[3-i] = convertHex(&(p2[i*2]));
  }
}

void ModProd::run()
{
  unsigned char result[4];
  memset(result,0,4);

  polyMult(poly1,poly2,result);
  //modularProduct(poly1,poly2,result);

  for(int i = 0; i<4; i++)//print results of modprod
    printf("{%02hhx}",poly1[3-i]);
  printf(" CIRCLEX ");
  for(int i = 0; i<4; i++)
    printf("{%02hhx}",poly2[3-i]);
  printf(" = ");
  for(int i = 0; i<4; i++)
    printf("{%02hhx}",result[3-i]);
  printf("\n");
}

//convert check[0],check[1] into byte if they are vlaid hex digits
unsigned char ModProd::convertHex(char *check)
{
  if(!((check[0]>='0' && check[0]<='9') || (check[0]>='a' && check[0]<='f')))
  {
    fprintf(stderr,"%c is not a hex number\n",check[0]);
    exit(1);
  }
  if(!((check[1]>='0' && check[1]<='9') || (check[1]>='a' && check[1]<='f')))
  {
    fprintf(stderr,"%c is not a hex number\n",check[1]);
    exit(1);
  }

  char subs[3];
  subs[0] = check[0];
  subs[1] = check[1];
  subs[2] = '\0';
  unsigned char r;
  sscanf(subs,"%hhx",&r);
  return r;
}

//perform polynomial multiplication on a and b
void ModProd::polyMult(unsigned char *a, unsigned char *b, unsigned char *result)
{
  result[0] = multi(a[0],b[0])^multi(a[3],b[1])^multi(a[2],b[2])^multi(a[1],b[3]);
  result[1] = multi(a[1],b[0])^multi(a[0],b[1])^multi(a[3],b[2])^multi(a[2],b[3]);
  result[2] = multi(a[2],b[0])^multi(a[1],b[1])^multi(a[0],b[2])^multi(a[3],b[3]);
  //printf("%hhu %hhu %hhu %hhu\n",multi(a[3],b[0]),multi(a[2],b[1]),multi(a[1],b[2]),multi(a[0],b[3]));
  //printf("%hhu %hhu\n",a[1],b[2]);
  result[3] = multi(a[3],b[0])^multi(a[2],b[1])^multi(a[1],b[2])^multi(a[0],b[3]);
}

//find dot product of 2 bytes
unsigned char ModProd::multi(unsigned char a, unsigned char b)
{
  //printf("calculating %hhu*%hhu\n",a,b);
  unsigned char v[8];
  v[0] = a;
  for(int i = 1; i<8; i++)
    v[i] = xtime(v[i-1]);

  unsigned char ret = 0;
  for(int i = 0; i<8; i++)
  {
    if(((b>>i) & 0x01) != 0)
      ret = ret^v[i];
  }
  //printf("answer is %hhu\n",ret);
  return ret;
}

//perform xtime operation on a and return result
unsigned char ModProd::xtime(unsigned char a)
{
  unsigned char ret = (a<<1);
  if((a & 0x80) !=0)
    ret = (ret ^ 0x1b);
  //printf("xtime(%hhu)=%hhu\n",a,ret);
  return ret;
}
