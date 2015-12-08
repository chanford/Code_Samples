#include "Circuit.h"
#include "Minitor.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <vector> 
#include <ifaddrs.h>
#include <linux/icmp.h>
#include <sys/time.h>

#define MAXBUFLEN 1500//maximum buffer length

//.h file for router class
class Router
{

 public://parts visible to Proxy
  Router(int,int,int,char *);
  char myIP[50];//printable version of raw sockets ip
  char myUDPIP[50];//printable version of the udp sockets ip 
  int port;//this routers allocated port
  void start();

 private:
  FILE *log;//log file to print to
  char *filename;//filename of log file
  int pid;//process id for router
  int stage;//stage numebr provided by config file
  int proxyPort;//the proxies dynamic port
  int routerNum;//which router this is
  int cirCount;//number of circuits created
  vector<Circuit *> circuits;//circuits created
  vector<char *> keys;//saved keys from fake dh messages that are to be processed
  char *lastKey;//most recent msg stored in keys
  //int rawport;//raw port number. no longer used i think

  int sockfd;//socket interface
  int rawsock;//raw socket interface
  struct addrinfo hints, *servinfo, *p, *nextR;//used for finding/binding a socket
  int rv;//return value for getaddrinfo
  int numbytes;//number of bytes sent/received
  char *buf;//buffer to send/receive data with
  struct sockaddr_storage their_addr;//address info of received package
  struct sockaddr_in return_addr;//address info of proxy;
  //bool open_ping;//
  char return_ip[4];//ip to return a ping reply to
  socklen_t addr_len;//size of their_addr

  int send_icmp_rawsock(struct sockaddr_in);//send ping through raw socket
  void bindSocket();//bind a socket for this router
  void sendHello();//send hello message to proxy
  void stage2Listen();//receive ping echo, formulate response, send response
  void stage3Listen();//listen between raw and udp. send to internet and proxy
  void makeReply(char *, int);//makes reply packet
  void decapsule();//remove ip header, control data from minitor packet
  void stage5Listen();// listen loop for stages 5/6
  void bindRawSock();//bind to a raw socket
  void handleCircuitDone();//handle an incoming circuit done packet
  void handleCircuitExtend();//handle an incoming circuit extend packet
  void extendCircuit(Circuit *);//expand a circuit to its next hop
  void handleRelay();//handle relay data
  void sendRelayReturn(Circuit *);//send a relay return message when the ping reply comes in from the raw socket
  void handleFakeDH();//handle messages with keys in them
  void handleEncCircuitExtend();//handle encrypted circuit extend messages
  void relayKey(Circuit *);//relay recieved key to next router in circuit
  void decryptBuf(AES_KEY *);//decrypt the buffer with the specified key
  void handleReplyReturn();//handle when a relay return packet comes to the udp socket
  static int in_cksum(u_short *addr, int);
};
