#include "Router.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/if_tun.h>
#include <stdlib.h>
#include <time.h>

//This is the .h file for the proxy that defines various variables and the funcitons inside the class
class Proxy
{
public:
  Proxy(char *);
  void start();

private:
  //ofstream output;
  char *filename;//config file filename
  char ofn[50];//output file filename
  int numRouters;//number of routers
  int stage;//stage to run through
  int port;//proxy's dynamic port
  int pid;//proxy's pid
  FILE *log;//file representing the output file
  int tun_fd;//file descriptor for tunnel interface
  int minitor_hops;//# of hops in config file
  short cirCount;//number of circuits created
  vector<Circuit *> circuits;//circuits created
  int ports[9];//ports of routers
  char rips[9][50];//ips of routers

  int sockfd;//file descriptor for udp socket
  struct addrinfo hints, *servinfo, *p;//hints=connection settings, servinfo = list of possible ports to use, p = port to use
  int rv;//return value of getaddrinfo
  int numbytes;//number of bytes sent/received over udp socket
  struct sockaddr_storage their_addr;//address of router that sends proxy data.
  struct sockaddr_storage their_addr_list[9];//address info of routers
  char *buf;//storage buffer for data sent/received
  socklen_t addr_len;//size of their_addr received
  //char *s;

  void encMsg(Circuit *, unsigned char *, int, unsigned char *, int *, int, unsigned char *, int *);//encrypt a message with the keys from a circuit
  void sendKey(Circuit *);//send a new key down a circuit
  void extendCircuit(Circuit *);//send a message through the circuit to the next hop to extend the circuit there
  void decapsule();//remove minitor's ip and control headers
  int makeCircuit();//make a minitor circuit
  int getRID(char *);//get the appropriate router for the provided ip
  void sendToRouter(int,char *,int);//send message to router
  void readConfig();//read the config file
  void forkRouters();//fork router processes
  void bindSocket();//bind a dynamic socket
  void listenPort();//listen to dynamic port for stage 1
  void processMessage();//process stage 1 message
  int tun_alloc(char *, int);//creates a tunnel interface. taken from http://backreference.org/2010/03/26/tuntap-interface-tutorial/
  void listenStage2();//listen to both tunnel and router and pass messages between them
  void listenStage5();//minitor listen
  //void testStuff();
  void decryptBuffer(Circuit *);//decrypt contents of buffer with a circuits keys
  void decMsg(Circuit *, unsigned char *, int, unsigned char *, int *, int, unsigned char *, int *);//decrypt a message with the provided circuits keys
};
