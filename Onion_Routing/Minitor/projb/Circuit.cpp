#include "Circuit.h"
//since Circuit is only used for data storage, doesnt need functions
//constructor for routers to use
Circuit::Circuit(int i,int o, int tp, struct sockaddr_storage *fs)
{
  iid = i;
  oid = o;
  toPort = tp;
  memcpy(&fromSock,fs,sizeof(struct sockaddr_storage));
  fulfilled=false;
}

//constructor for proxy to use
Circuit::Circuit(int o, int to, int s)
{
  iid = -1;
  oid = o;
  toPort = to;
  length = s;
  progress = 0;
}
