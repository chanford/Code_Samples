#include "Proxy.h"
//this file is for the Proxy node that communicates with the router and the tunnel
//note for entier file. all sendto/recvfrom code taken from beej's tutorial

//sets initial values for some of the variables
Proxy::Proxy(char *fn)
{
  filename = fn;
  stage = numRouters = 1;
  pid = getpid();
  buf = new char[MAXBUFLEN];
  cirCount = 1;
  addr_len = sizeof(struct sockaddr_storage);
  srand((int)time(NULL));

  //get raw ip of each of the routers
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) 
  {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  //go through list of interfaces and get ip of each. Adapted from code provided by the TA Zi Hu
  struct sockaddr_in ipaddr;
  char number[1];
  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) 
  {
    if (ifa->ifa_addr == NULL)
      continue;
    family = ifa->ifa_addr->sa_family;
    /* we only look at AF_INET* interface address*/
    if (family == AF_INET ) 
    {
      s = getnameinfo(ifa->ifa_addr,
		      (family == AF_INET) ? sizeof(struct sockaddr_in) :
		      sizeof(struct sockaddr_in6),
		      host, NI_MAXHOST,
		      NULL, 0, NI_NUMERICHOST);
      if (s != 0) 
      {
	printf("getnameinfo() failed: %s\n", gai_strerror(s));
	exit(EXIT_FAILURE);
      }
      printf("%s\taddress: <%s>\n", ifa->ifa_name, host);
      //convert IP in string format to struct
      ipaddr.sin_addr.s_addr = inet_addr(host);
      if(strncmp("eth",ifa->ifa_name, 3)==0) //compare the first 4 characters of "ethX" with ifa_name 
      {
	number[0] = ifa->ifa_name[3];
	int i = atoi(number);
	printf("router %d ip is %s\n",i, inet_ntoa(ipaddr.sin_addr));
	sprintf(rips[i],"%s",inet_ntoa(ipaddr.sin_addr));
	//rips[i] = inet_ntoa(ipaddr.sin_addr);
      }
    }
  }
}

//main program process
void Proxy::start()
{
  readConfig();
  bindSocket();
  forkRouters();
  listenPort();
  printf("finished stage 1\n");
  if(stage>1 && stage<5)
    listenStage2();
  //testStuff();
  if(stage>=5)
    listenStage5();
}

//create a new circuit and send the message to extend the circuit to the 1st router
int Proxy::makeCircuit()
{
  bool check[numRouters+1];
  int order[minitor_hops];
  for(int i = 0; i<numRouters+1; i++)
    check[i] = false;
  int count = 0;
  printf("order of hops: ");
  while(count<minitor_hops)//create a path minitor_hops long
  {
    int random = rand()%numRouters +1;
    if(check[random]==false)
    {
      check[random] = true;
      order[count] = random;
      printf("%d",random);
      count++;
    }
  }
  printf("\n");

  int port = -1;
  short id = cirCount;
  cirCount++;
  
  Circuit *c = new Circuit(id,port,minitor_hops);//make the new circuit
  for(int i = 0; i<minitor_hops; i++)
    c->route[i] = order[i];
  printf("proxy about to make circuit\n");
  circuits.push_back(c);
  printf("proxy made circuit\n");
  
  /*for(int i = 0; i<minitor_hops; i++)
  {
    char packet [IPHEADLN+EXTENDLN];
    if(i==minitor_hops-1)
      Minitor::makeExtend(id,0xffff,packet);
    else
      Minitor::makeExtend(id,ports[order[i+1]],packet);
    sendToRouter(order[0],packet,IPHEADLN+EXTENDLN);
    //delete packet;
  }*/
  extendCircuit(c);
  printf("Proxy finished making circuit\n");
  return cirCount-2;//return the position of the created circuit in the circuits list

}

//decrypt the buffer with the keys from the sepcified circuit
void Proxy::decryptBuffer(Circuit *c)
{
  int crypt_text_len = numbytes-IPHEADLN-3;
  unsigned char crypt_text[crypt_text_len];
  for(int i = 0; i<crypt_text_len; i++)
    crypt_text[i] = buf[i+IPHEADLN+3];
  unsigned char *clear_crypt_text = 0;
  unsigned char finalMsg[MAXBUFLEN];
  int clear_crypt_text_len,finalLn;
  decMsg(c,crypt_text,crypt_text_len,clear_crypt_text,&clear_crypt_text_len,0,finalMsg,&finalLn);

  for(int i = 0; i<finalLn; i++)
    buf[i+IPHEADLN+3] = finalMsg[i];
  numbytes = finalLn+IPHEADLN+3;
}

//recursive function to decrypt a message with keys from a circuit. goes forwards in router order and uses each decryption key. saves final message and its length in finalMsg and finalLn
void Proxy::decMsg(Circuit *c, unsigned char *crypt_text, int crypt_text_len, unsigned char *clear_crypt_text, int *clear_crypt_text_len, int progress, unsigned char *finalMsg, int *finalLn)
{
  if(progress == minitor_hops)
  {
    *finalLn = crypt_text_len;
    for(int i = 0; i<crypt_text_len; i++)
    {
      finalMsg[i] = crypt_text[i];
    }
  }

  if(progress<minitor_hops)
  {
    printf("depth %d crypt_text_len %d\n",progress,crypt_text_len);
    AES_KEY dec_key;
    memcpy(&dec_key,&(c->decKey[progress]),sizeof(AES_KEY));
    printf("about to do actual decrypt \n");

    AESF::class_AES_decrypt_with_padding(crypt_text, crypt_text_len, &clear_crypt_text, clear_crypt_text_len, &dec_key);
    printf("depth %d clear_text_len %d\n",progress,*clear_crypt_text_len);

    printf("about to do recursion\n");
    unsigned char *nClear_text = 0;
    int nClear_text_len;
    decMsg(c,clear_crypt_text,*clear_crypt_text_len,nClear_text,&nClear_text_len,progress+1,finalMsg,finalLn);
    //printf("depth %d nCrypt_text_len %d\n",progress,nCrypt_text_len);

    //memcpy(crypt_text,nCrypt_text,nCrypt_text_len);
    //*crypt_text_len = nCrypt_text_len;
  }
}

//recursive function to encrypt message with all the keys in a circuit that have been established. starts at the last key and goes backwards. final message and its length are put in finalMsg and finalLn
void Proxy::encMsg(Circuit *c, unsigned char *clear_text, int clear_text_len, unsigned char *crypt_text,int *crypt_text_len,int progress, unsigned char *finalMsg, int *finalLn)
{
  //printf("encode message\n");
  if(progress<0)
  {
    printf("progress < 0 %d\n",clear_text_len);
    //crypt_text = (unsigned char *) malloc(clear_text_len);
    /*memcpy(crypt_text,clear_text,clear_text_len);
     *crypt_text_len = clear_text_len;*/

    *finalLn = clear_text_len;
    //finalMsg = new unsigned char[clear_text_len];
    //finalMsg = (unsigned char *) malloc(clear_text_len);
    for(int i = 0; i<clear_text_len; i++)
    {
      finalMsg[i] = clear_text[i];
      //printf("%hhu %hhu\n",clear_text[i],finalMsg[i]);
    }
    //memcpy(finalMsg,clear_text,clear_text_len);
    //printf("finished setting crypt\n");
  }
  if(progress>=0)
  {
    printf("depth %d clear_text_len %d\n",progress,clear_text_len);
    AES_KEY enc_key;
    memcpy(&enc_key,&(c->key[progress]),sizeof(AES_KEY));
    printf("about to do actual encrypt \n");

    AESF::class_AES_encrypt_with_padding(clear_text, clear_text_len, &crypt_text, crypt_text_len, &enc_key);
    printf("depth %d crypt_text_len %d\n",progress,*crypt_text_len);

    printf("about to do recursion\n");
    unsigned char *nCrypt_text = 0;
    int nCrypt_text_len;
    encMsg(c,crypt_text,*crypt_text_len,nCrypt_text,&nCrypt_text_len,progress-1,finalMsg,finalLn);
    //printf("depth %d nCrypt_text_len %d\n",progress,nCrypt_text_len);

    //memcpy(crypt_text,nCrypt_text,nCrypt_text_len);
    //*crypt_text_len = nCrypt_text_len;
  }
}

//send a new key down the circuit
void Proxy::sendKey(Circuit *c)
{
  printf("proxy send key\n");
  unsigned char key_text[16];
  for(int i = 0; i<16; i++)
    key_text[i] = rand()%256;

  log = fopen(ofn,"a");
  //new-fake-diffe-hellman, router index: 2, circuit outgoing: 0x01, key: 0x1b5c98e8ee38c007daf54c8ec7c7723c
  fprintf(log,"new-fake-diffe-hellman, router index: %d, circuit outgoing: 0x%x, key : 0x",c->route[c->progress], c->oid);
  for(int i = 0; i<16; i++)
  {
    //key_text[i];
    if(key_text[i]<0x10)
      fprintf(log,"0%hhx",key_text[i]);
    else
      fprintf(log,"%hhx",key_text[i]);
  }
  fprintf(log,"\n");
  fclose(log);

  int prog = c->progress;
  int routerNum = c->route[prog];

  unsigned char *xor1 = key_text;
  unsigned char xor2[16];
  for(int i = 0; i<16; i++)
    xor2[i] = routerNum;
  unsigned char xorR[16];
  for(int i = 0; i<16; i++)
    xorR[i] = (unsigned char)(xor1[i] ^ xor2[i]);//xor key with the destination routers ip number

  printf("router initial xorr\n");
  for(int i = 0; i<16; i++)
    printf("%hhu ",xorR[i]);
  printf("\n");

  unsigned char *crypt_text = 0;
  unsigned char finalMsg[MAXBUFLEN];
  int crypt_text_len,finalLen;
  int progress = (c->progress)-1;//use every key except the one for the router this is being sent to
  encMsg(c,xorR,16,crypt_text,&crypt_text_len,progress,finalMsg,&finalLen);//encrypt the key data
  printf("final msg\n");
  for(int i = 0; i<finalLen; i++)
    printf("%hhu ",finalMsg[i]);
  printf("\nenc key len %d\n",finalLen);
  //exit(1);
  
  int size = IPHEADLN+3+finalLen;
  unsigned char msg[size];
  printf("about to make fake dh\n");
  Minitor::makeFakeDH(c->oid,(char *) finalMsg,size,(char *)msg);
printf("about to send fake dh\n");
 sendToRouter(c->route[0],(char *)msg,size);//send off the key
  //printf("router key maker sent data %s\n",msg);

  unsigned char key_data[AES_KEY_LENGTH_IN_CHARS];
  AES_KEY enc_key,dec_key;

  memset(key_data, 0, sizeof(key_text));
  memcpy(key_data, key_text, 16);
	printf("proxy set key\n");
  AESF::class_AES_set_encrypt_key(key_data, &enc_key);
  AESF::class_AES_set_decrypt_key(key_data, &dec_key);

  

  //AESF::testKeys(&enc_key,&dec_key);

  
  memcpy(&(c->key[prog]),&enc_key,sizeof(AES_KEY));
  memcpy(&(c->decKey[prog]),&dec_key,sizeof(AES_KEY));

  //sendTest(routerNum,&enc_key);

  

  
}

//send a message through the circuit to the next router to extend it
void Proxy::extendCircuit(Circuit *c)
{
  printf("proxy extending circuit\n");
  if(c->progress==minitor_hops)//if already completed the circuit send the icmp message instead
  {
    printf("proxy sending ping\n");
    if(stage>=6)//if using secure router, need to encode the message before sending it out
    {
      printf("ping message is \n");
      for(int i = 0; i< c->msgLen; i++)
	printf("%hhu",c->msg[i]);
      unsigned char *crypt_text = 0;
      unsigned char finalMsg[MAXBUFLEN];
      int crypt_text_len,finalLen;
      int progress = c->progress-1;
      encMsg(c,(unsigned char *)(c->msg),c->msgLen,crypt_text,&crypt_text_len,progress,finalMsg,&finalLen);
      char *msg = new char[IPHEADLN+3+finalLen];
      
      Minitor::makeEncRelay(c->oid,msg);
      for(int i = 0; i<finalLen;i++)
	msg[i+IPHEADLN+3] = finalMsg[i];

      sendToRouter(c->route[0],msg,IPHEADLN+3+finalLen);
      return;
    }

    sendToRouter(c->route[0],c->msg,c->msgLen);//if encryption not needed just send the message
    return;
  }
  if(stage>=6)//if using encryption, need to send a key to the next router first
  {
    sendKey(c);
    //return;
  }
  int i = c->progress;
  
  unsigned short nextP = 0;
  if(i==minitor_hops-1)
    nextP = 0xFFFF;
  else
    nextP = ports[c->route[i+1]];

  printf("about to make extend header\n");
  if(stage==5)//if no encryption, just send the port
  {
    char packet [IPHEADLN+EXTENDLN];
    Minitor::makeExtend(c->oid,nextP,packet);
    sendToRouter(c->route[0],packet,IPHEADLN+EXTENDLN);
    c->progress++;
    log = fopen(ofn,"a");
    fprintf(log,"hop: %d, router: %d\n",c->progress, c->route[c->progress - 1]);
    fclose(log);
  }
  else//if encryption needed, encrypt the port before sending it.
  {
    unsigned char clear_text[2];
    nextP = htons(nextP);
    memcpy(clear_text,&nextP,2);
    int clear_text_len = 2;
    unsigned char *crypt_text = 0;
    unsigned char finalMsg[MAXBUFLEN];
    int crypt_text_len,finalLen;
    printf("about to encrypt header\n");
    encMsg(c,clear_text,clear_text_len,crypt_text,&crypt_text_len,i,finalMsg,&finalLen);
    printf("encrypted header port %hu finallen %d\n",ntohs(nextP),finalLen);

    char packet [IPHEADLN+3+finalLen];

    Minitor::makeEncExtend(c->oid,finalMsg,finalLen,packet);

    sendToRouter(c->route[0],packet,IPHEADLN+3+finalLen);
    c->progress++;
    log = fopen(ofn,"a");
    fprintf(log,"hop: %d, router: %d\n",c->progress, c->route[c->progress - 1]);
    fclose(log);
  }

  
}

//remove ip header and control header from data
void Proxy::decapsule()
{
  for(int i = IPHEADLN+RELAYRTNLN; i<numbytes; i++)
    buf[i-IPHEADLN-RELAYRTNLN] = buf[i];
  numbytes = numbytes-IPHEADLN-RELAYLN;
}

//main loop for stage 5/6
void Proxy::listenStage5()
{
  char tun_name[IFNAMSIZ];
  
  /* Connect to the device */
  strcpy(tun_name, "tun1");
  tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface */

  if(tun_fd < 0)
  {
    perror("Allocating interface");
    exit(1);
  }
  printf("listening to tunnel\n");

  while(1)//most of select framework taken from backreference tutorial
  {
    int ret,nread;
    fd_set rd_set;

    FD_ZERO(&rd_set);
    FD_SET(tun_fd, &rd_set); FD_SET(sockfd, &rd_set);
    printf("waiting for select\n");
    ret = select(sizeof(rd_set)*8, &rd_set, NULL, NULL, NULL);// this line taken from https://support.sas.com/documentation/onlinedoc/sasc/doc700/html/lr2/select.htm . backreference version of this line was not working properly

    if (ret < 0) 
    {
      perror("select()");
      exit(1);
    }

    if(FD_ISSET(tun_fd, &rd_set)) //if the tunnel is set
    {
      printf("tunnel selected\n");
      nread = read(tun_fd, buf, MAXBUFLEN-1);//read from tunnel
      printf("received %d bytes from tunnel. message is %s\n",nread,buf);

      log = fopen(ofn,"a");//write to log about packet received
      fprintf(log,"ICMP from tunnel, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu, type: %d\n",buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19],buf[20]);
      fclose(log);

      Circuit *c = circuits[makeCircuit()];//make a circuit for the new ping
      if(stage==5)//if stage 5, we can just make the entire message now
      {
	c->msg = new char[nread+IPHEADLN+RELAYLN];
	c->msgLen = nread+IPHEADLN+RELAYLN;
	Minitor::makeRelay(c->oid,c->msg);//make the relay message used for when the acutal ping is send and store it in the circuits msg arrat
	for(int i = 0; i<nread; i++)
	  c->msg[i+IPHEADLN+RELAYLN] = buf[i];
      }
      else//if stage 6, dont know how long encypted contents will be, so just save the ping message
      {
	c->msg = new char[nread];
	c->msgLen = nread;
	for(int i = 0; i<nread; i++)
	  c->msg[i] = buf[i];
      }
      
    }

    if(FD_ISSET(sockfd, &rd_set)) //if the socket is set
    {
      printf("Proxy socket selected\n");
      
      //read from socket
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
      {
	perror("recvfrom Proxy stage 5");
	exit(1);
      }

      //pkt from port: 51358, length: 3, contents: 0x530001
      log = fopen(ofn,"a");
      fprintf(log,"pkt from port: %d, length: %d, contents: 0x",ntohs(((sockaddr_in *) &their_addr)->sin_port),numbytes-IPHEADLN);
      for(int i = IPHEADLN; i<numbytes; i++)
      {
	if(buf[i]<0x10)//since no seperators between each byte, need to make sure each one is 2 charachters long
	  fprintf(log,"0%hhx",buf[i]);
	else
	  fprintf(log,"%hhx",buf[i]);
      }
      fprintf(log,"\n");
      fclose(log);

      if(buf[IPHEADLN]==0x53 || buf[IPHEADLN]==0x63)//if received a circuit extend done message
      {
	printf("proxy circuit extend done\n");
	unsigned short oid;
	memcpy(&oid,&buf[IPHEADLN+1],2);
	oid = ntohs(oid);
	printf("Proxy received circuit extend done id %hu\n",oid);
	Circuit *c = 0;

	for(unsigned int i = 0; i<circuits.size(); i++)//find the circuit its for
	{
	  if(circuits[i]->oid == oid)
	    {
	      printf("proxy circuit found\n");
	      c = circuits[i];
	      log = fopen(ofn,"a");
	      fprintf(log,"incoming extend-done circuit, incoming: 0x%hx from %d: \n",oid,ntohs(((sockaddr_in *) &their_addr)->sin_port));
	      fclose(log);
	      extendCircuit(c);
	      break;
	    }
	}

	if(c==0)
	{
	  printf("Error proxy, circuit not found\n");
	  exit(1);
	}
      }
      

      if(buf[IPHEADLN]==0x54 || buf[IPHEADLN]==0x64)//if its a returned ping, write to tunnel
      {
	int nwrite;
	unsigned short oid;
	memcpy(&oid,&buf[IPHEADLN+1],2);
	oid = ntohs(oid);
	printf("proxy relay return");

	if(buf[IPHEADLN]==0x64)//if stage 6 or farther, need to decrypt the ping reply before writing to tunnel
	{
	  Circuit *c = 0;
	  for(unsigned int i = 0; i<circuits.size(); i++)
	  {
	    c = circuits[i];
	    if(oid==c->oid)
	    {
	      decryptBuffer(c);
	      break;
	    }
	  }
	  
	}

	log = fopen(ofn,"a");
	fprintf(log,"incoming packet, circuit incoming: 0x%hx, ",oid);
	decapsule();
	fprintf(log,"src:%hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu.\n",buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19]);
	fclose(log);
	if((nwrite = write(tun_fd, buf, numbytes))<0)
	{
	  perror("writing to tunnel");
	  exit(1);
	}
	printf("wrote %d\n",nwrite);
      }
    }
  }
}

//listens to tunnel and routers. makes use of code form backreference and beej
void Proxy::listenStage2()
{
  char tun_name[IFNAMSIZ];
  
  /* Connect to the device */
  strcpy(tun_name, "tun1");
  tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface */

  if(tun_fd < 0)
  {
    perror("Allocating interface");
    exit(1);
  }
  printf("listening to tunnel\n");

  while(1)//most of select framework taken from backreference tutorial
  {
    int ret,nread,nwrite;
    fd_set rd_set;

    FD_ZERO(&rd_set);
    FD_SET(tun_fd, &rd_set); FD_SET(sockfd, &rd_set);
    printf("waiting for select\n");
    ret = select(sizeof(rd_set)*8, &rd_set, NULL, NULL, NULL);// this line taken from https://support.sas.com/documentation/onlinedoc/sasc/doc700/html/lr2/select.htm . backreference version of this line was not working properly

    if (ret < 0) 
    {
      perror("select()");
      exit(1);
    }

    if(FD_ISSET(tun_fd, &rd_set)) //if the tunnel is set
    {
      printf("tunnel selected\n");
      nread = read(tun_fd, buf, MAXBUFLEN-1);//read from tunnel
      printf("received %d bytes from tunnel. message is %s\n",nread,buf);

      log = fopen(ofn,"a");//write to log about packet received
      fprintf(log,"ICMP from tunnel, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu, type: %d\n",buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19],buf[20]);
      fclose(log);

      int rid = 1;

      if(stage>3)
      {
	char dstIP[50];
	sprintf(dstIP,"%hhu.%hhu.%hhu.%hhu",buf[16],buf[17],buf[18],buf[19]);
	rid = getRID(dstIP);
      }
      
      sendToRouter(rid,buf,nread);
      //send packet over udp to router
      /*if ((numbytes = sendto(sockfd, buf, nread, 0,(sockaddr *)&their_addr, sizeof(struct sockaddr_storage))) == -1) 
      {
	perror("listener stage 2 send");
	exit(1);
      }*/
    }

    if(FD_ISSET(sockfd, &rd_set)) //if the socket is set
    {
      printf("socket selected\n");
      
      //read from socket
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
      {
	perror("recvfrom stage 2");
	exit(1);
      }
      //testing code to print out contents of packet for examination
      /*printf("received %d bytes from router, message is %s\n",numbytes,buf);
      printf("\n contents of packet ");
      for(int i = 0; i<numbytes; i++)
	printf("%hhu ",buf[i]);
	printf("\n");*/

      //prints packet info to log
      log = fopen(ofn,"a");
      fprintf(log,"ICMP from port: %d, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu, type: %d\n",ntohs(((struct sockaddr_in *)&their_addr)->sin_port),buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19],buf[20]);
      fclose(log);
      //write data to tunnel interface
      if((nwrite = write(tun_fd, buf, numbytes))<0)
      {
	perror("writing to tunnel");
	exit(1);
      }
      printf("wrote %d\n",nwrite);
    }
  }
}

//sends a message to the router with the specified rid
void Proxy::sendToRouter(int rid, char *msg, int size)
{
  printf("sendtorouter %d",rid);
  memcpy(&their_addr,&(their_addr_list[rid]),sizeof(struct sockaddr_storage));

  if ((numbytes = sendto(sockfd, msg, size, 0,(sockaddr *)&their_addr, sizeof(struct sockaddr_storage))) == -1) 
  {
    printf("sendToRouter %d failed",rid);
    perror("");
    exit(1);
  }
}

//process stage 1 message from router
void Proxy::processMessage()
{
  //printf("listener: got packet from %s\n",inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));
  printf("listener: packet is %d bytes long\n", numbytes);
  buf[numbytes] = '\0';//make sure data ends with terminating 0
  printf("listener: packet contains \"%s\"\n", buf);

  char *split;//split the message sent into substrings
  split = strtok(buf," ");
  if(strcmp(split,"rtrInit")==0)//structure of message in stage 1 is "rtrInit routerNumber routerPID"
  {
    split = strtok(NULL," ");
    int routerNum = atoi(split);
    split = strtok(NULL," ");
    int rtrPID = atoi(split);

    memcpy(&(their_addr_list[routerNum]),&their_addr,sizeof(struct sockaddr_storage));
    ports[routerNum] = ntohs(((struct sockaddr_in *)&their_addr)->sin_port);

    log = fopen(ofn,"a");//write info to log
    if(stage>=5)
    {
      char tmpIP[50];
      inet_ntop(AF_INET, &their_addr,tmpIP, 50);
      //fprintf(log,"router: %d, pid: %d, port: %d, IP: %s\n",routerNum,rtrPID,ntohs(((struct sockaddr_in *)&their_addr)->sin_port),rips[routerNum]);
      fprintf(log,"router: %d, pid: %d, port: %d, IP: %s\n",routerNum,rtrPID,ntohs(((struct sockaddr_in *)&their_addr)->sin_port),tmpIP);
    }
    else
      fprintf(log,"router: %d, pid: %d, port: %d\n",routerNum,rtrPID,ntohs(((struct sockaddr_in *)&their_addr)->sin_port));
    fclose(log);
  }
}

//stage 1 listen to router over udp
void Proxy::listenPort()
{
  printf("listener: waiting to recvfrom...\n");
  addr_len = sizeof their_addr;
  for(int i = 0; i<numRouters; i++)//for stage one, expecting 1 message from each router
  {
    printf("i%d numRouters%d\n",i,numRouters);
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
    {
      perror("recvfrom");
      exit(1);
    }

    processMessage();
    printf("processed message\n");
  }
  printf("finished listening\n");
  return;
}

//finds an open socket and binds to it. Most of teh code in this function taken from Beej's guide
void Proxy::bindSocket()
{
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
  hints.ai_socktype = SOCK_DGRAM;//udp datagrams
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, "0", &hints, &servinfo)) != 0) //0 for parameter 2 indicates any port fine
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(0);
  }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
      {
	perror("listener: socket");
	continue;
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
      {
	close(sockfd);
	perror("listener: bind");
	continue;
      }

      break;
    }

    if (p == NULL) 
    {
      fprintf(stderr, "listener: failed to bind socket\n");
      exit(0);
    }

    freeaddrinfo(servinfo);

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)//code to get the port number assigned to my socket and print to the log. taken from stackoverflow
      perror("getsockname");
    else
    {
      printf("port number %d\n", ntohs(sin.sin_port));
      port = ntohs(sin.sin_port);
      log = fopen(ofn,"a");
      fprintf(log,"proxy port: %d\n",port);
      fclose(log);
    }

    printf("listener: waiting to recvfrom...\n");
}

//takes in a destinationip and decides which router it should be sent to.
int Proxy::getRID(char *ip)
{
  int i;
  for(i = 1; i<=numRouters; i++)
  {
    if(strcmp(ip,rips[i])==0)//if the destination is the router, send it straight there
      return i;
  }

  unsigned long converted_ip;//else do ip%numrouters + 1
  inet_pton(AF_INET,ip,&converted_ip);
  int ret = (converted_ip % numRouters)+1;
  printf("%s mapped to router %d\n",ip,ret);
  return ret;
}

//fork off router processes
void Proxy::forkRouters()
{
  printf("forking %d routers\n",numRouters);
  for(int i = 1; i<=numRouters; i++)
  {
    int p = fork();
    if(p==0)//p = 0 means inside child process, so go start a router
    {
      char rip[50];
      sprintf(rip,"192.168.20%i.2",i);
      Router r(port,i,stage,rip);
      r.start();
      exit(0);
    }
  }
}

//read the config file and set variables
void Proxy::readConfig()
{
  string line;
  ifstream  config;
  config.open(filename);
  if(config.is_open())
  {
    while(getline(config,line))
    {
      char *in = new char[line.size()+1];
      copy(line.begin(),line.end(),in);
      in[line.size()] = '\0';

      cout<<in<<"\n";
      if(in[0] !='#')//# indicates comment line
      {
	char *split;
	split = strtok(in," ");

	if(strcmp(split,"stage")==0)//get stage number
	{
	  split = strtok(NULL," ");
	  if(split!=NULL)
	  {
	    stage = atoi(split);
	    if(stage<1)
	    {
	      printf("\nInvalid stage number. Stage number should be >=1\n");
	      exit(1);
	    }
	  }
	}

	if(strcmp(split,"num_routers")==0)//get number of routers to make
	{
	  split = strtok(NULL," ");
	  if(split!=NULL)
	  {
	    numRouters = atoi(split);
	    if(numRouters<1)
	    {
	      printf("\nInvalid number of routers. num_routers should be >=1\n");
	      exit(1);
	    }
	  }
	}

	if(strcmp(split,"minitor_hops")==0)
	{
	  split = strtok(NULL," ");
	  if(split !=NULL)
	  {
	    minitor_hops = atoi(split);
	  }
	}
      }
    }
    config.close();
  }
  else
  {
    cout<<"invalid config file"<<"\n";
    exit(1);
  }
  sprintf(ofn,"stage%d.proxy.out",stage);
  log = fopen(ofn,"w");
  fclose(log);
}

//allocate tunnel interface. Copied entirely from backreference tutorial
int Proxy::tun_alloc(char *dev, int flags) 
{

  struct ifreq ifr;
  int fd, err;
  char clonedev[] =  "/dev/net/tun";

    if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

  strcpy(dev, ifr.ifr_name);

  return fd;
}
