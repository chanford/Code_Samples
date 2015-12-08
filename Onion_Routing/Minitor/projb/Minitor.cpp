#include "Minitor.h"
//make ip header for provided char *
void Minitor::makeIP(char *ip)
{ 
  for(int i = 0; i<IPHEADLN; i++)
    ip[i] = 0;

  ip[9] = 253;

  ip[12] = ip[16] = 127;
  ip[15] = ip[19] = 1;
}
//make encrypted extend header
void Minitor::makeEncExtend(unsigned short id, unsigned char *encPort, int size, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];
  //delete ip;

  msg[IPHEADLN] = 0x62;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);

  for(int i = 0; i<size; i++)
    msg[IPHEADLN+3+i] = encPort[i];
  //memcpy(&msg[IPHEADLN+3],encPort,size);
}
//make fake dh header
void Minitor::makeFakeDH(unsigned short id, char *key, int size, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];
printf("made ip\n");
  msg[IPHEADLN] = 0x65;
printf("set type\n");
  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
  printf("set id\n");
  for(int i = 0; i<size; i++)
  {
    //printf("derp");
    //printf("%d\n",i);
    msg[i+IPHEADLN+3] = key[i];
  }
  //printf("set msg %hu %\n",id);
}
//make encrypted relay return header
void Minitor::makeEncRelayReturn(unsigned short id, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];

  msg[IPHEADLN] = 0x64;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
}
//make relay return header
void Minitor::makeRelayReturn(unsigned short id, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];

  msg[IPHEADLN] = 0x54;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
}
//make encrypted extend done header
void Minitor::makeEncExtendDone(unsigned short id, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];

  msg[IPHEADLN] = 0x63;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
}
//make extend done header
void Minitor::makeExtendDone(unsigned short id, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];

  msg[IPHEADLN] = 0x53;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
}
//make encrypted relay header
void Minitor::makeEncRelay(unsigned short id, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];

  msg[IPHEADLN] = 0x61;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
}
//make relay header
void Minitor::makeRelay(unsigned short id, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];

  msg[IPHEADLN] = 0x51;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);
}
//make extend header
void Minitor::makeExtend(unsigned short id, unsigned short p, char *msg)
{
  char ip[IPHEADLN];
  makeIP(ip);
  for(int i = 0; i<IPHEADLN; i++)
    msg[i] = ip[i];
  //delete ip;

  msg[IPHEADLN] = 0x52;

  id = htons(id);
  memcpy(&msg[IPHEADLN+1],&id,2);

  uint16_t port = htons(p);
  memcpy(&msg[IPHEADLN+3],&port,2);

  printf("id %hu port %hu\n",id,p);
  printf("packet bytes %hhu %hhu %hhu %hhu %hhu\n",msg[IPHEADLN],msg[IPHEADLN+1],msg[IPHEADLN+2],msg[IPHEADLN+3],msg[IPHEADLN+4]);
  
}
