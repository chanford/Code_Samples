#include <sys/socket.h>
#include "aesF.h"
//used by proxy and routers to sotre circuit data
class Circuit
{
 public:
  Circuit(int,int,int,struct sockaddr_storage *);//constructor for routers
  Circuit(int,int,int);//constructor for proxy
  int iid;//incoming circuit id
  int oid;//outgoing circuit id
  int toPort;//port of next hop
  AES_KEY rKey;//routers 1 encryption key
  AES_KEY rDecKey;//routers 1 decryption key
  AES_KEY key[8];//encryption keys, only 1 is used for routers
  AES_KEY decKey[8];//decryption keys used by proxy
  int route[8];//only used by proxy, full path of the circuit
  int progress;//only used by proxy, current progress in establishing circuit
  struct sockaddr_storage toSock,fromSock;//storage for address of prev/next hop
  int toLen,length;//size of tosock, length of route
  char *msg;//only used by proxy, message to be sent out once circuit is complete
  int msgLen;//length of msg
  char destIP[4];//destination ip in ping packet
  char sourceIP[4];//saved source ip of ping packet
  bool fulfilled;//only used by end router, true if this circuit has received a reply packet
  bool keySet;//if the key has been set yet
};
