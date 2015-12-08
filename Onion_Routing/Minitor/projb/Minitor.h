#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#define IPHEADLN 20//length of ip header for icmp echo request
#define EXTENDLN 5//length of extend type header
#define RELAYLN 3//length of relay type header
#define RELAYRTNLN 3//length of relay return type header
#define EXTENDDNLN 3//length of extend return header
#define FAKEDHLN 19//length of fake dh header.
//this class is a collection of static functions used by proxy/routers to create minitor headers
class Minitor
{
 public:
  static void makeIP(char *);//make Minitor ip header and put it in the provided string
  static void makeEncRelay(unsigned short, char *);//make an encrypted relay packet header
  static void makeRelay(unsigned short, char *);//make a relay packet header
  static void makeExtend(unsigned short, unsigned short, char *);//make a circuit extend header
  static void makeExtendDone(unsigned short, char *);//make an extend done header
  static void makeEncExtendDone(unsigned short, char *);//make an encrypted circuit extend done message header
  static void makeEncRelayReturn(unsigned short, char *);//make an encrypted relay return message header
  static void makeRelayReturn(unsigned short, char *);//make a relay return header
  static void makeFakeDH(unsigned short, char *, int, char *);//make a fake dh mesage header
  static void makeEncExtend(unsigned short, unsigned char *, int, char*);//make an encrypted circuit extend header
};

