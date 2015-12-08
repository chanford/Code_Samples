#include "Router.h"

//Router class, as of stage 2, its main purpose is to make icmp echo replies when it receives icmp echo requests

//note for entire file. all sendto/recvfrom code taken from beej's tutorial

//set some variables. port=dynamic port number of proxy. router= router number, sta = stage number
Router::Router(int port, int router, int sta, char *ip)
{
  printf("router %d created\n",router);
  proxyPort = port;
  routerNum = router;
  stage = sta;
  filename = new char[100];
  sprintf(filename,"stage%d.router%d.out",stage,routerNum);
  log = fopen(filename,"w");
  fclose(log);
  buf = new char[MAXBUFLEN];
  pid = getpid();
  cirCount = 1;
  addr_len = sizeof(struct sockaddr_storage);
  //open_ping = false;
  //myIP = ip;
}

//bind a raw socket to the eth associated with this router
void Router::bindRawSock()
{
  if ((rawsock = socket(AF_INET, SOCK_RAW,IPPROTO_ICMP)) == -1) 
    {
      perror("raw talker: socket");
      exit(-1);
    }

    char eth[10];
    sprintf(eth,"eth%d",routerNum);

    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    //find the interface for this routers eth and bind its ip. adapted from code provided by the TA Zi Hu
    struct sockaddr_in ipaddr;
    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
            continue;
        family = ifa->ifa_addr->sa_family;
        /* we only look at AF_INET* interface address*/
        if (family == AF_INET ) {
            s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                          sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("%s\taddress: <%s>\n", ifa->ifa_name, host);
            //convert IP in string format to struct
            ipaddr.sin_addr.s_addr = inet_addr(host);
            if(strncmp(eth,ifa->ifa_name, 4)==0) //compare the first 4 characters of "ethX" with ifa_name 
            {
                printf("router %d ip is %s\n",routerNum, inet_ntoa(ipaddr.sin_addr));
                break;
            }
        }
    }

    if((bind(rawsock, (struct sockaddr*)&ipaddr, sizeof(struct sockaddr)) ==-1))
    {
      close(rawsock);
      perror("rawsender: bind");
      exit(1);
    }
    else
    {
      inet_ntop(AF_INET,&(ipaddr.sin_addr),myIP,50);
      printf("router %d is binded to IP: %s\n", routerNum, myIP);
    }
    freeifaddrs(ifaddr);

}

//send circuit extend message to the next hop in a circuit
void Router::extendCircuit(Circuit *c)
{

  unsigned short nid = htons(c->oid);
  memcpy(&buf[IPHEADLN+1],&nid,2);

  char hostName[50];
  int hostLen = 50;
  gethostname(hostName,hostLen);//get this systems host name
  char portStr[20];
  sprintf(portStr,"%d",c->toPort);
  
  
  if ((rv = getaddrinfo(hostName, portStr, &hints, &servinfo)) != 0) 
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(0);
  }

  nextR = servinfo;
  freeaddrinfo(servinfo);

  
  if ((numbytes = sendto(sockfd, buf, numbytes, 0,nextR->ai_addr, nextR->ai_addrlen)) == -1) 
  {
    perror("talker: sendto");
    exit(1);
  }
  memcpy(&(c->toSock),nextR->ai_addr,nextR->ai_addrlen);
  c->toLen = nextR->ai_addrlen;
}

//handle when a relay return packet comes in through the udp socket
void Router::handleReplyReturn()
{
  printf("router %d ping return\n",routerNum);
  unsigned short oid;
  memcpy(&oid,&buf[IPHEADLN+1],2);
  oid = ntohs(oid);
  printf("router%d received ping reply id %hu\n",routerNum,oid);
  Circuit *c = 0;

  for(unsigned int i = 0; i<circuits.size(); i++)//find the right circuit
  {
    if(circuits[i]->oid == oid)
    {
      printf("router %d circuit found\n",routerNum);
      c = circuits[i];
      break;
    }
  }

  if(c==0)
  {
    printf("Error router %d, circuit not found\n",routerNum);
    return;
    //exit(1);
  }
  if(stage==5)//on stage 5 dont need to encrypt the message, just change the id field and send it out
  {
    char oIP[50];
    inet_ntop(AF_INET, &(c->fromSock),oIP, 50);

    log = fopen(filename,"a");
    if(stage==5)
      fprintf(log,"relay reply packet, circuit incoming: 0x%x, outgoing: 0x%x, src: %hhu.%hhu.%hhu.%hhu, incoming dst: %s, outgoing dest: %s\n",c->oid,c->iid,buf[12+IPHEADLN+RELAYRTNLN],buf[13+IPHEADLN+RELAYRTNLN],buf[14+IPHEADLN+RELAYRTNLN],buf[15+IPHEADLN+RELAYRTNLN],myUDPIP,oIP); 
    else
      fprintf(log,"relay reply packet, circuit incoming: 0x%x, outgoing: 0x%x\n",c->oid,c->iid); 
    fclose(log);

    unsigned short nid = htons(c->iid);
    memcpy(&buf[IPHEADLN+1],&nid,2);
  
    if ((numbytes = sendto(sockfd, buf, numbytes, 0,(sockaddr *)&(c->fromSock), sizeof(sockaddr_storage)) == -1))
    {
      perror("router circuit done");
      exit(1);
    }
  }


  if(stage>=6)//when its stage 6 or farther, we need to encrypt the relay reply before sending it off
  {
    printf("router %d relaying encrypted relay return\n",routerNum);
    
    int clear_Len = numbytes-IPHEADLN-3;
    unsigned char clear[clear_Len];
    for(int i = 0; i<clear_Len; i++)
      clear[i] = buf[i+IPHEADLN+3];

    unsigned char *crypt_text;
    int crypt_text_len;
    AESF::class_AES_encrypt_with_padding(clear, clear_Len, &crypt_text, &crypt_text_len, &(c->rKey));

    char msg[IPHEADLN+3+crypt_text_len];
    Minitor::makeEncRelayReturn(c->iid,msg);
    for(int i = 0; i<crypt_text_len; i++)
      msg[i+IPHEADLN+3] = crypt_text[i];

    printf("old encrypted length %d new %d\n",clear_Len,crypt_text_len);

    if ((numbytes = sendto(sockfd, msg, IPHEADLN+3+crypt_text_len, 0,(sockaddr *)&(c->fromSock), sizeof(sockaddr_storage)) == -1))
    {
      perror("router relay return");
      exit(1);
    }
  }
}

//handle when a circuit done message comes in the udp socket
void Router::handleCircuitDone()
{
  printf("router %d circuit extend done\n",routerNum);
  unsigned short oid;
  memcpy(&oid,&buf[IPHEADLN+1],2);
  oid = ntohs(oid);
  printf("router%d received circuit extend done id %hu\n",routerNum,oid);
  Circuit *c = 0;

  for(unsigned int i = 0; i<circuits.size(); i++)
  {
    if(circuits[i]->oid == oid)
    {
      printf("router %d circuit found\n",routerNum);
      c = circuits[i];
      break;
    }
  }

  if(c==0)
  {
    printf("Error router %d, circuit not found\n",routerNum);
    return;
    //exit(1);
  }

  unsigned short nid = htons(c->iid);
  memcpy(&buf[IPHEADLN+1],&nid,2);
  
  sockaddr_in *tmp = (sockaddr_in *)(&(c->fromSock));
  int port = ntohs(tmp->sin_port);
  log = fopen(filename,"a");
  //forwarding extend-done circuit, incoming: 0x101, outgoing: 0x01 at 49306
  fprintf(log,"forwarding extend-done circuit, incoming: 0x%x, outgoing: 0x%x at %d\n",c->oid,c->iid,port);  
  fclose(log);
  
  if ((numbytes = sendto(sockfd, buf, numbytes, 0,(sockaddr *)&(c->fromSock), sizeof(sockaddr_storage)) == -1))
  {
    perror("router circuit done");
    exit(1);
  }
}

//send a reply return through the circuit when a ping reply comes in the raw socket
void Router::sendRelayReturn(Circuit *c)
{
  if(stage==5)//in stage 5 just need to change the id field then send it off
  {
    printf("router %d send relay return\n",routerNum);
    //Circuit *c = lastCircuit;
    char msg[numbytes+IPHEADLN+RELAYRTNLN];
    Minitor::makeRelayReturn(c->iid,msg);
    for(int i = 0; i<numbytes; i++)
      msg[i+IPHEADLN+RELAYRTNLN] = buf[i];

    if ((numbytes = sendto(sockfd, msg, numbytes+IPHEADLN+RELAYRTNLN, 0,(sockaddr *)&(c->fromSock), sizeof(sockaddr_storage)) == -1))
      {
	perror("router relay return");
	exit(1);
      }
  }
  if(stage>=6)//for stage 6 and later, need to encrypt before sending
  {
    printf("router %d send encrypted relay return\n",routerNum);
    
    unsigned char *crypt_text;
    int crypt_text_len;
    AESF::class_AES_encrypt_with_padding((unsigned char *)buf, numbytes, &crypt_text, &crypt_text_len, &(c->rKey));

    char msg[IPHEADLN+3+crypt_text_len];
    Minitor::makeEncRelayReturn(c->iid,msg);
    for(int i = 0; i<crypt_text_len; i++)
      msg[i+IPHEADLN+3] = crypt_text[i];

    if ((numbytes = sendto(sockfd, msg, IPHEADLN+3+crypt_text_len, 0,(sockaddr *)&(c->fromSock), sizeof(sockaddr_storage)) == -1))
    {
      perror("router relay return");
      exit(1);
    }
  }
}

//handle when a data relay packet comes in the udp socket
void Router::handleRelay()
{
  printf("router %d circuit relay\n",routerNum);
  unsigned short iid;
  memcpy(&iid,&buf[IPHEADLN+1],2);
  iid = ntohs(iid);
  printf("router%d received relay message id %hu\n",routerNum,iid);
  Circuit *c = 0;

  for(unsigned int i = 0; i<circuits.size(); i++)
  {
    if(circuits[i]->iid == iid)
    {
      printf("router %d circuit found\n",routerNum);
      c = circuits[i];
      if(stage>=6)
	decryptBuf(&(c->rDecKey));
      break;
    }
  }

  if(c==0)
  {
    printf("Error router %d, circuit not found\n",routerNum);
    log = fopen(filename,"a");
    //unknown incoming circuit: 0xIDi, src: S, dst: D
    fprintf(log,"unknown incoming circuit: 0x%x, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu",iid,buf[IPHEADLN+RELAYLN+12],buf[IPHEADLN+RELAYLN+13],buf[IPHEADLN+RELAYLN+14],buf[IPHEADLN+RELAYLN+15],buf[IPHEADLN+RELAYLN+16],buf[IPHEADLN+RELAYLN+17],buf[IPHEADLN+RELAYLN+18],buf[IPHEADLN+RELAYLN+19]);  
    fclose(log);
    //exit(1);
    return;
  }

  //if this is the last hop send it out through the raw socket
  if(c->toPort==0xFFFF)
  {
    printf("router %d: end router reached\n",routerNum);
    //lastCircuit = c;


    char iIP[50];
    inet_ntop(AF_INET, &their_addr,iIP, 50);
    char *oIP = myUDPIP;
    char dstIP[4];
    for(int i = 0; i<4; i++)
    {
      dstIP[i] = buf[16+i+IPHEADLN+RELAYLN];
    }

    log = fopen(filename,"a");
    if(stage==5)
      fprintf(log,"outgoing packet, circuit incoming: 0x%x, incoming src: %s, outgoing src: %s, dst: %hhu.%hhu.%hhu.%hhu\n",c->iid,iIP,oIP,dstIP[0],dstIP[1],dstIP[2],dstIP[3]);  
    else
      fprintf(log,"outgoing packet, circuit incoming: 0x%x, incoming src: 0.0.0.0, outgoing src: %s, dst: %hhu.%hhu.%hhu.%hhu\n",c->iid,oIP,dstIP[0],dstIP[1],dstIP[2],dstIP[3]);
    fclose(log);

    decapsule();

    for(int i = 0; i<4; i++)
    {
      c->sourceIP[i] = buf[12+i];
      c->destIP[i] = buf[16+i];
    }

    char destIP[50];
    sprintf(destIP,"%hhu.%hhu.%hhu.%hhu",buf[16],buf[17],buf[18],buf[19]);
    printf("destip of packet is %s\n",destIP);


    if(strcmp(myIP,destIP) == 0)//if its directed to this router, do the same thing as stage 2
    {	
      sendRelayReturn(c);
    }
    else
    {

      struct sockaddr_in dst_addr;
      dst_addr.sin_family = AF_INET;
      inet_pton(AF_INET, destIP, &(dst_addr.sin_addr));

      send_icmp_rawsock(dst_addr);
    }
    
    return;
  }
  //if this isnt the last hop, send it to the next hop
  unsigned short nid = htons(c->oid);
  memcpy(&buf[IPHEADLN+1],&nid,2);

  char iIP[50];
  inet_ntop(AF_INET, &their_addr,iIP, 50);
  char *oIP = myUDPIP;
  char dstIP[4];
  for(int i = 0; i<4; i++)
  {
    dstIP[i] = buf[16+i+IPHEADLN+RELAYLN];
  }

  log = fopen(filename,"a");
  if(stage==5)
    fprintf(log,"relay packet, circuit incoming: 0x%x, outgoing: 0x%x, incoming src: %s, outgoing src: %s, dst: %hhu.%hhu.%hhu.%hhu\n",c->iid,c->oid,iIP,oIP,dstIP[0],dstIP[1],dstIP[2],dstIP[3]);  
  else
    fprintf(log,"relay encrypted packet, circuit incoming: 0x%x, outgoing: 0x%x\n",c->iid,c->oid);
  fclose(log);
  
  if ((numbytes = sendto(sockfd, buf, numbytes, 0,(sockaddr *)&(c->toSock), c->toLen)) == -1) 
  {
    perror("talker: sendto");
    exit(1);
  }
}

//remove ip header and mintor contorl data from packet
void Router::decapsule()
{
  for(int i = IPHEADLN+RELAYLN; i<numbytes; i++)
    buf[i-IPHEADLN-RELAYLN] = buf[i];
  numbytes = numbytes-IPHEADLN-RELAYLN;
}

//handle when an encrypted circuit extend message comes in
void Router::handleEncCircuitExtend()
{
  printf("router %d encrypted circuit extend\n",routerNum);

  unsigned short iid,oid,toPort;
  memcpy(&iid,&buf[IPHEADLN+1],2);
  iid = ntohs(iid);

  bool found = false;
  for(unsigned int i = 0; i<circuits.size(); i++)
  {
    if(circuits[i]->iid == iid)//if the circuit is found, then go to extend circuit
    {
      Circuit *c = circuits[i];
      found = true;
      printf("router %d circuit exists\n",routerNum); 
      
      decryptBuf(&(c->rDecKey));//need to decrypt it with this routers key so next circuit can read the message

      log = fopen(filename,"a");
      fprintf(log,"forwarding extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",c->iid,c->oid,c->toPort);  
      fclose(log);

      extendCircuit(c);
      break;
    }
  }

  if(!found)// if the circuit wasnt found, make a new one
  {

    AES_KEY enc_key,dec_key;

    for(unsigned int i = 0; i<keys.size(); i++)//find the stored key so we can assign it to this circuit
    {
      char *msg = keys[i];
      unsigned short cid;
      memcpy(&cid,&msg[1],2);
      cid = ntohs(cid);
      printf("checking id %hu\n",cid);
      if(cid == iid)
      {
	printf("router %d assigning keys to circuit",routerNum);
	unsigned char key_text[16];
	for(int i = 0; i<16; i++)
	{
	  key_text[i] = (unsigned char) msg[i+3];
	  printf("%c",key_text[i]);
	}

	unsigned char *xor1 = key_text;//get the key text by xoring each char with the router num
	unsigned char xor2[16];
	for(int i = 0; i<16; i++)
	  xor2[i] = routerNum;
	for(int i = 0; i<16; i++)
	{
	  key_text[i] = (unsigned char)(xor1[i] ^ xor2[i]);
	  //printf("%c",key_text[i]);
	}

	//fake-diffie-hellman, new circuit incoming: 0x01, key: 0x6f2272720618b7253f2494e71f2024ea
	/*log = fopen(filename,"a");
	fprintf(log,"fake-diffie-hellman, new circuit incoming: 0x%hx, key: 0x",iid);
	for(int i = 0; i<16; i++)
	{
	  unsigned char tmp = key_text[i];
	  if(tmp<0x10)
	    fprintf(log,"0%hhx",tmp);
	  else
	    fprintf(log,"%hhx",tmp);
	}

	fprintf(log,"\n");
	fclose(log);*/

	unsigned char key_data[AES_KEY_LENGTH_IN_CHARS];
      

	memset(key_data, 0, sizeof(key_text));
	memcpy(key_data, key_text, 16);
	
	AESF::class_AES_set_encrypt_key(key_data, &enc_key);
	AESF::class_AES_set_decrypt_key(key_data, &dec_key);

	//AESF::testKeys(&enc_key,&dec_key);

	


	decryptBuf(&dec_key);//decrypt the encrypted port

	break;
      }
    }

    memcpy(&toPort,&buf[IPHEADLN+3],2);
    port = ntohs(toPort);
    printf("router%d received extend message id %hu port %hu\n",routerNum,iid,port);
    oid = routerNum*256 + cirCount;
    cirCount++;
    Circuit *c = new Circuit(iid,oid,port,&their_addr);

    memcpy(&(c->rKey),&enc_key,sizeof(AES_KEY));
    memcpy(&(c->rDecKey),&dec_key,sizeof(AES_KEY));
    c->keySet = true;

    //AESF::testKeys(&(c->rKey),&(c->rDecKey));
    
    circuits.push_back(c);

    printf("router%d created circuit iid %hu oid %hu toPort %hu\n",routerNum,c->iid,c->oid,c->toPort);
    printf("router%d fromport %hu\n",routerNum,((struct sockaddr_in *)(&their_addr))->sin_port);


    log = fopen(filename,"a");
    fprintf(log,"new extend circuit: incoming: 0x%hx, outgoing: 0x%hx at %hu\n",iid,oid,port);  
    fclose(log);

    int family = their_addr.ss_family;
    printf("family %d %d\n",family,AF_INET);
    char msg[IPHEADLN+EXTENDDNLN];
    Minitor::makeEncExtendDone(iid,msg);
    if ((numbytes = sendto(sockfd, msg, IPHEADLN+EXTENDDNLN, 0,(struct sockaddr *)&their_addr, sizeof(struct sockaddr_storage))==-1)) 
    {
      perror("router stage 5 circuit made");
      exit(1);
    }
  }
}

//handle when a circuit extend message comes in
void Router::handleCircuitExtend()
{
  printf("router %d circuit extend\n",routerNum);
  unsigned short iid,oid,toPort;
  memcpy(&iid,&buf[IPHEADLN+1],2);
  memcpy(&toPort,&buf[IPHEADLN+3],2);
  iid = ntohs(iid);
  port = ntohs(toPort);
  printf("router%d received extend message id %hu port %hu\n",routerNum,iid,port);

  bool found = false;
  for(unsigned int i = 0; i<circuits.size(); i++)
  {
    if(circuits[i]->iid == iid)//if the circuit is found, then go to extend circuit
    {
      Circuit *c = circuits[i];
      found = true;
      printf("router %d circuit exists\n",routerNum);

      log = fopen(filename,"a");
      fprintf(log,"forwarding extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",c->iid,c->oid,c->toPort);  
      fclose(log);

      extendCircuit(c);
      break;
    }
  }

  if(!found)// if the circuit wasnt found, make a new one
  {
    oid = routerNum*256 + cirCount;
    cirCount++;
    Circuit *c = new Circuit(iid,oid,port,&their_addr);
    
    circuits.push_back(c);

    printf("router%d created circuit iid %hu oid %hu toPort %hu\n",routerNum,c->iid,c->oid,c->toPort);
    printf("router%d fromport %hu\n",routerNum,((struct sockaddr_in *)(&their_addr))->sin_port);


    log = fopen(filename,"a");
    fprintf(log,"new extend circuit: incoming: 0x%hx, outgoing: 0x%hx at %hu\n",iid,oid,port);  
    fclose(log);

    int family = their_addr.ss_family;
    printf("family %d %d\n",family,AF_INET);
    char msg[IPHEADLN+EXTENDDNLN];
    Minitor::makeExtendDone(iid,msg);
    if ((numbytes = sendto(sockfd, msg, IPHEADLN+EXTENDDNLN, 0,(struct sockaddr *)&their_addr, sizeof(struct sockaddr_storage))==-1)) 
    {
      perror("router stage 5 circuit made");
      exit(1);
    }
  }
  
}

//decrypt everything in the buffer after the ip header and minitor header
void Router::decryptBuf(AES_KEY *decKey)
{
  printf("router %d decrypting buffer\n",routerNum);
  int cryptLen = numbytes-IPHEADLN-3;
  printf("numbytes %d cryptlen %d\n",numbytes,cryptLen);
  unsigned char cryptTxt[cryptLen];

  for(int i = 0; i<cryptLen; i++)
  {
    //printf("derp %d %hhu\n",i,buf[i+IPHEADLN+3]);
    cryptTxt[i] =  buf[i+IPHEADLN+3];
    }
  
  printf("finished copying crypt from buf\n");
  
  //printf("1\n");
  //memcpy(&decKey,&(c->rDecKey),sizeof(AES_KEY));
  unsigned char *clear_crypt_text;
  //printf("2\n");
  int clear_crypt_text_len;
  //printf("3\n");
  
  //printf("4\n");
  AESF::class_AES_decrypt_with_padding(cryptTxt, cryptLen, &clear_crypt_text, &clear_crypt_text_len, decKey);
  printf("router %d decrypted buf\n",routerNum);
  for(int i = 0; i<clear_crypt_text_len; i++)//overwrite buffer with decrypted data, replace numbytes with new value
  {
    buf[i+IPHEADLN+3] = clear_crypt_text[i];
    printf("%hhu ",clear_crypt_text[i]);
  }
  numbytes = IPHEADLN+3+clear_crypt_text_len;
  printf("router %d new numbytes is %d\n",routerNum,numbytes);
}

//relay a key to the next router in the circuit
void Router::relayKey(Circuit *c)
{
  printf("router %d relaying key\n",routerNum);
  decryptBuf(&(c->rDecKey));//decrypt the current message so next router can read it
  unsigned short nid = htons(c->oid);
  memcpy(&buf[IPHEADLN+1],&nid,2);
  
  //fake-diffie-hellman, forwarding,  circuit incoming: 0x01, key:
  log = fopen(filename,"a");
  fprintf(log,"fake-diffie-hellman, forwarding, circuit incoming: 0x%x, key: 0x",c->iid);
  for(int i = IPHEADLN+3; i<numbytes; i++)
  {
    unsigned char tmp = (unsigned char) buf[i];
    if(tmp<0x10)
      fprintf(log,"0%hhu",tmp);
    else
      fprintf(log,"%hhu",tmp);
  }

  fprintf(log,"\n");
  fclose(log);

  char hostName[50];
  int hostLen = 50;
  gethostname(hostName,hostLen);//get this systems host name
  char portStr[20];
  sprintf(portStr,"%d",c->toPort);
  
  
  if ((rv = getaddrinfo(hostName, portStr, &hints, &servinfo)) != 0) 
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(0);
  }

  nextR = servinfo;
  freeaddrinfo(servinfo);

  if ((numbytes = sendto(sockfd, buf, numbytes, 0,nextR->ai_addr, nextR->ai_addrlen)) == -1) 
  {
    perror("talker: sendto");
    exit(1);
  }
}

//handle an incoming fake dh message
void Router::handleFakeDH()
{
  printf("router %d received fakeDH %s\n",routerNum,buf);
  unsigned short iid;
  memcpy(&iid,&buf[IPHEADLN+1],2);
  iid = ntohs(iid);
  printf("fakedh iid is %hu\n",iid);

  for(unsigned int i = 0; i<circuits.size(); i++)//find the right circuit
  {
    Circuit *c = circuits[i];
    if(c->iid == iid)//if a circuit is found, relay the key
    {
      printf("fake dh found iid\n");
      relayKey(c);
      
      return;
    }
  }

  printf("fake dh didnt find iid\n");//if a circuit isnt found then save the received message so the circuit created can make keys form it
  int size = numbytes-IPHEADLN;
  char *save = new char[size];
  for(int i = 0; i<size; i++)
    save[i] = buf[i+IPHEADLN];
  keys.push_back(save);
  lastKey = save;

  unsigned char key_text[16];
  for(int i = 0; i<16; i++)
  {
    key_text[i] = (unsigned char) save[i+3];
    printf("%c",key_text[i]);
  }

  unsigned char *xor1 = key_text;
  unsigned char xor2[16];
  for(int i = 0; i<16; i++)
    xor2[i] = routerNum;
  for(int i = 0; i<16; i++)
  {
    key_text[i] = (unsigned char)(xor1[i] ^ xor2[i]);
    //printf("%c",key_text[i]);
  }

  //fake-diffie-hellman, new circuit incoming: 0x01, key: 0x6f2272720618b7253f2494e71f2024ea
  log = fopen(filename,"a");
  fprintf(log,"fake-diffie-hellman, new circuit incoming: 0x%hx, key: 0x",iid);
  for(int i = 0; i<16; i++)
  {
    unsigned char tmp = key_text[i];
    if(tmp<0x10)
      fprintf(log,"0%hhx",tmp);
    else
      fprintf(log,"%hhx",tmp);
  }

  fprintf(log,"\n");
  fclose(log);
  
  //printf("saved key %hhu\n",keys.back()[1]);
  
}



//listen loop for stage 5/6
void Router::stage5Listen()
{

  
    

  while(1)
  {
      fd_set rd_set;

      FD_ZERO(&rd_set);
      FD_SET(rawsock, &rd_set); FD_SET(sockfd, &rd_set);
      printf("router waiting for select\n");
      int ret = select(sizeof(rd_set)*8, &rd_set, NULL, NULL, NULL);// this line taken from https://support.sas.com/documentation/onlinedoc/sasc/doc700/html/lr2/select.htm . backreference version of this line was not working properly

      if (ret < 0) 
      {
        perror("router select()");
        exit(1);
      }

      printf("router %d stage %d waiting for select\n\n",routerNum,stage);

    if(FD_ISSET(sockfd, &rd_set)) //if the udp socket is set
    {
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
      {
        perror("recvfrom stage 3 router");
        exit(1);
      }

      printf("\n%d received %d bytes from proxy, message is %s\n",routerNum,numbytes,buf);
      printf("router%d fromport %hu\n",routerNum,((struct sockaddr_in *)(&their_addr))->sin_port);

      log = fopen(filename,"a");
      fprintf(log,"pkt from port: %d, length: %d, contents: 0x",ntohs(((sockaddr_in *) &their_addr)->sin_port),numbytes-IPHEADLN);
      for(int i = IPHEADLN; i<numbytes; i++)
      {
	unsigned char tmp =(unsigned char) buf[i];
	if(tmp<0x10)
	  fprintf(log,"0%hhx",tmp);
	else
	  fprintf(log,"%hhx",tmp);
      }
      fprintf(log,"\n");
      fclose(log);

      if(buf[IPHEADLN]==0x62)
	handleEncCircuitExtend();

      if(buf[IPHEADLN]==0x52)
	handleCircuitExtend();

      if(buf[IPHEADLN]==0x51 || buf[IPHEADLN]==0x61)
	handleRelay();

      if(buf[IPHEADLN]==0x53 || buf[IPHEADLN]==0x63)
	 handleCircuitDone();

      if(buf[IPHEADLN]==0x54 || buf[IPHEADLN] == 0x64)
	handleReplyReturn();

      if(buf[IPHEADLN]==0x65)
	handleFakeDH();

      
    }

    if(FD_ISSET(rawsock,&rd_set))//if packet from raw socket
    {
      if ((numbytes = recvfrom(rawsock, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
      {
        perror("recvfrom stage 3 rawsock");
        exit(1);
      }

      

      printf("\nreceived %d bytes from rawsock, message is\n",numbytes);
      for(int i = 0; i<IPHEADLN; i++)//print ip header bytes
	printf("%hhu ",buf[i]);
      printf("\n\n");
      for(int i = IPHEADLN; i<numbytes; i++)//print icmp bytes
	printf("%hhu ",buf[i]);
      printf("\n");
      //if ip protocol 1 and tos 0, and icmp type 0 and icmp code 0, means its a ping reply
      if(numbytes>IPHEADLN && buf[1] == 0 && buf[9]==1 && buf[IPHEADLN]==0 && buf[IPHEADLN+1]==0)
      {
	char destIP[50];
	sprintf(destIP,"%hhu.%hhu.%hhu.%hhu",buf[16],buf[17],buf[18],buf[19]);
	printf("dst ip of raw is %s\n",destIP);
	if(strcmp(destIP,myIP)==0)//should only care about packets for me
	{
	  
	  Circuit *c = 0;

	  for(unsigned int i = circuits.size()-1; i>=0; i--)
	  {
	    char *temp = circuits[i]->destIP;
	    if(!(circuits[i]->fulfilled) && temp[0]==buf[12] && temp[1]==buf[13] && temp[2]==buf[14] && temp[3]==buf[15])
	      {//sometimes packets dont come back form the internet, we only want to respond to the most recent ping with the same source/destination pair
	      c = circuits[i];
	      c->fulfilled = true;
	      //write ping reply to log
	      log = fopen(filename,"a");
	      fprintf(log,"incoming packet, src:%hhu.%hhu.%hhu.%hhu, dst: %s, outgoing circuit: 0x%x\n",buf[12],buf[13],buf[14],buf[15],myIP,c->iid);  
	      fclose(log);

	      break;
	    }
	  }    
	  if(c==0)
	  {
	    printf("router %d pre relay return, circuit not found",routerNum);
	    return;
	    //exit(1);
	  }

	  char ipHdr[IPHEADLN];
	  for(int i = 0; i<IPHEADLN; i++)//copy ip header from message received
	    ipHdr[i] = buf[i];

	  for(int i = 0; i<4; i++)//replace destination ip with the one saved earlier
	    ipHdr[16+i] = c->sourceIP[i];
	  
	  printf("copied dest ip is %hhu.%hhu.%hhu.%hhu\n",ipHdr[16],ipHdr[17],ipHdr[18],ipHdr[19]);
	  ipHdr[10] = ipHdr[11] = 0;//reset checksum
	  int cksum = in_cksum((u_short *)ipHdr,IPHEADLN);

	  ipHdr[11] = (cksum >> 8) & 0xFF;//put new checksum in
	  ipHdr[10] = cksum & 0xFF;
	  
	  for(int i = 0; i<IPHEADLN; i++)
	    buf[i] = ipHdr[i];

	  sendRelayReturn(c);
	}
      }
    }
  }
}

//function to generate the icmp echo reply packet. used by stage 2.
void Router::makeReply(char *reply, int numbytes)
{
  char ipHeader[IPHEADLN];//ip header portion of reply
  //char chk[IPHEADLN];//used for testing that i know how to correctly generte the checksum

    //printf("ip1 ");
    for(int i = 0; i<IPHEADLN; i++)//copies initial ip header and also prints every byte
    {
      //printf("%hhu ",buf[i]);
      ipHeader[i] = buf[i];
      //chk[i] = buf[i];
    }
    //printf("\n");

    

    //switch source and destination ip
    char source[4];
    for(int i = 0; i<4; i++)
    {
      source[i] = ipHeader[12+i];
      ipHeader[12+i] = ipHeader[16+i];
      ipHeader[16+i] = source[i];
    }

    //reset checksum
    ipHeader[10] = 0;
    ipHeader[11] = 0;
    int cksum = in_cksum((u_short *)ipHeader,IPHEADLN);
    ipHeader[11] = (cksum >> 8) & 0xFF;
    ipHeader[10] = cksum & 0xFF;

    int icmpLn = numbytes-IPHEADLN;//length of icmp pakcet

    //char chki[icmpLn];//same as chk, but for icmp instead of ip header
 
    char icmp[icmpLn];//icmp packet to be sent back

    for(int i = 0; i<icmpLn; i++)//copies icmp packet sent.
      icmp[i] = buf[i+IPHEADLN];
    //type = 0
    icmp[0] = 0;
    //code = 0
    icmp[1] = 0;
    //temp checksum = 0
    icmp[2] = 0;
    icmp[3] = 0;
    

    cksum = in_cksum((u_short *)icmp,icmpLn);//computes new checksum
    //printf("bytes in int %u.%u.%u.%u\n",(cksum >> 24) & 0xFF,(cksum >> 16) & 0xFF,(cksum >> 8) & 0xFF,cksum & 0xFF);

    icmp[3] = (cksum >> 8) & 0xFF;
    icmp[2] = cksum & 0xFF;

    //printf("ip2 ");
    for(int i = 0; i<IPHEADLN; i++)//copies my new ip header into the reply data
    {
      //printf("%hhu ",ipHeader[i]);//prints out each byte of the new header
      reply[i] = ipHeader[i];
    }
    //printf("\n");

    //printf("icmp2 ");
    for(int i = IPHEADLN; i<numbytes; i++)//copies new icmp packet into reply
    {
      reply[i] = icmp[i-IPHEADLN];
      //printf("%hhu ",reply[i]);//prints new icmp packet
    }
    printf("\n");
    //prints out source and destination ip for reply packet
    //printf("src %d.%d.%d.%d dst %d.%d.%d.%d\n",reply[12],reply[13],reply[14],reply[15],reply[16],reply[17],reply[18],reply[19]);
}

//send icmp ping to dst_addr using raw socket
int Router::send_icmp_rawsock(struct sockaddr_in dst_addr)
{
  /*struct Packet 
    {
      struct icmphdr icmp;
      struct timeval tv; 
      } p;*/

    struct iovec iov;
    
    struct msghdr m = {&dst_addr, sizeof(struct sockaddr_in), &iov,1, 0, 0, 0};  

    ssize_t bs; 
    /*p.icmp.type = ICMP_ECHO;
    p.icmp.code = 0;
    p.icmp.un.echo.id = 0;
 
    iov.iov_base = &p; 
    iov.iov_len = sizeof(struct icmphdr)+ sizeof(struct timeval);
 
    p.icmp.checksum = 0;
    p.icmp.un.echo.sequence = htons (0);
    gettimeofday(&p.tv, NULL);
 
    p.icmp.checksum = in_cksum((uint16_t*)&p, iov.iov_len);*/

    int icmpln = numbytes-IPHEADLN;
    char send[icmpln];
    for(int i = 0; i<icmpln; i++)//copy over bytes from buffer that make up the ping packet
      send[i] = buf[i+IPHEADLN];

    iov.iov_base = send;
    iov.iov_len = icmpln;
 
    if (0> (bs = sendmsg (rawsock, &m, 0))) 
    {   
      perror ("ERROR: sendmsg ()");
    }   
    printf("raw send successful\n");
    //memset(&p, 0, sizeof(struct Packet));
    memset(&iov, 0, sizeof(struct iovec));
    memset(&m, 0, sizeof(struct msghdr));
    return bs; 
}

void Router::stage3Listen()
{

  bindRawSock();
    

  while(1)
  {
      fd_set rd_set;

      FD_ZERO(&rd_set);
      FD_SET(rawsock, &rd_set); FD_SET(sockfd, &rd_set);
      printf("router waiting for select\n");
      int ret = select(sizeof(rd_set)*8, &rd_set, NULL, NULL, NULL);// this line taken from https://support.sas.com/documentation/onlinedoc/sasc/doc700/html/lr2/select.htm . backreference version of this line was not working properly

      if (ret < 0) 
      {
        perror("router select()");
        exit(1);
      }

      printf("router %d stage %d waiting for select\n\n",routerNum,stage);

    if(FD_ISSET(sockfd, &rd_set)) //if the udp socket is set
    {
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
      {
        perror("recvfrom stage 3 router");
        exit(1);
      }

      printf("\nreceived %d bytes from proxy, message is %s\n",numbytes,buf);

      //write ping request to log
      log = fopen(filename,"a");
      fprintf(log,"ICMP from port: %d, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu, type: %d\n",proxyPort,buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19],buf[20]);
      fclose(log);

      char destIP[50];
      char reply[numbytes];//reply to be sent back
      sprintf(destIP,"%hhu.%hhu.%hhu.%hhu",buf[16],buf[17],buf[18],buf[19]);
      printf("destip of packet is %s\n",destIP);

      if(strcmp(myIP,destIP) == 0)//if its directed to this router, do the same thing as stage 2
      {	
	makeReply(reply,numbytes);

	//sends reply packet to proxy
	if ((numbytes = sendto(sockfd, reply, numbytes, 0,p->ai_addr, p->ai_addrlen)) == -1) 
	{
          perror("talker: sendto");
          exit(1);
        }
      }
      else//else send it to the internet
      {
	//open_ping = true;
	memcpy(&return_addr,&their_addr,sizeof(sockaddr_in));//copy down the senders info for easy returning later
	for(int i = 0; i<4; i++)
	  return_ip[i] = buf[12+i];

	struct sockaddr_in dst_addr;
	dst_addr.sin_family = AF_INET;
	inet_pton(AF_INET, destIP, &(dst_addr.sin_addr));

	send_icmp_rawsock(dst_addr);
	
      }
    }

    if(FD_ISSET(rawsock,&rd_set))//if packet from raw socket
    {
      if ((numbytes = recvfrom(rawsock, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
      {
        perror("recvfrom stage 3 rawsock");
        exit(1);
      }

      printf("\nreceived %d bytes from rawsock, message is\n",numbytes);
      for(int i = 0; i<IPHEADLN; i++)//print ip header bytes
	printf("%hhu ",buf[i]);
      printf("\n");
      for(int i = IPHEADLN; i<numbytes; i++)//print icmp bytes
	printf("%hhu ",buf[i]);
      printf("\n");
      //if ip protocol 1 and tos 0, and icmp type 0 and icmp code 0, means its a ping reply
      if(numbytes>IPHEADLN && buf[1] == 0 && buf[9]==1 && buf[IPHEADLN]==0 && buf[IPHEADLN+1]==0)
      {
	char destIP[50];
	sprintf(destIP,"%hhu.%hhu.%hhu.%hhu",buf[16],buf[17],buf[18],buf[19]);
	printf("dst ip of raw is %s\n",destIP);
	if(strcmp(destIP,myIP)==0)
	{
	  //write ping reply to log
	  log = fopen(filename,"a");
	  fprintf(log,"ICMP from raw sock, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu, type: %d\n",buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19],buf[20]);
	  fclose(log);

	  //if(!open_ping)
	  //{
	    /*char reply[numbytes];//reply to be sent back
      
	    makeReply(reply,numbytes);

	    //sends reply packet to proxy
	    if ((numbytes = sendto(sockfd, reply, numbytes, 0,(struct sockaddr *)&their_addr, addr_len)) == -1) 
	    {
	      perror("talker: sendto");
	      exit(1);
	    }*/
	    //continue;
	  //}

	  char ipHdr[IPHEADLN];
	  for(int i = 0; i<IPHEADLN; i++)//copy ip header from message received
	    ipHdr[i] = buf[i];

	  for(int i = 0; i<4; i++)//replace destination ip with the one saved earlier
	    ipHdr[16+i] = return_ip[i];
	  
	  printf("copied dest ip is %hhu.%hhu.%hhu.%hhu\n",ipHdr[16],ipHdr[17],ipHdr[18],ipHdr[19]);
	  ipHdr[10] = ipHdr[11] = 0;//reset checksum
	  int cksum = in_cksum((u_short *)ipHdr,IPHEADLN);

	  ipHdr[11] = (cksum >> 8) & 0xFF;//put new checksum in
	  ipHdr[10] = cksum & 0xFF;
	  
	  for(int i = 0; i<IPHEADLN; i++)
	    buf[i] = ipHdr[i];

	  struct sockaddr_in returnto;//make a sockaddr for the proxy
	  returnto.sin_family = AF_INET;
	  returnto.sin_port = htons(proxyPort);
	  char temp[50];
	  sprintf(temp,"%hhu.%hhu.%hhu.%hhu",return_ip[0],return_ip[1],return_ip[2],return_ip[3]);
	  inet_pton(AF_INET,temp,&(returnto.sin_addr.s_addr));
	  //if ((numbytes = sendto(sockfd, buf, numbytes, 0,(struct sockaddr *)&return_addr, sizeof(struct sockaddr_in))==-1)) 
	  if ((numbytes = sendto(sockfd, buf, numbytes, 0,(struct sockaddr *)&returnto, sizeof(struct sockaddr_in))==-1)) 
	  {
	    perror("stage 3 ping reply: sendto");
	    exit(1);
	  }
	  //open_ping = false;
	}
      }
    }
  }
}

//loop to receive ping request, formulate response, and send it
void Router::stage2Listen()
{
  while(1)
  {
    //receive ping request
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
    {
      perror("recvfrom stage 2 router");
      exit(1);
    }

    printf("\nreceived %d bytes from proxy, message is %s\n",numbytes,buf);

    //write ping request to log
    log = fopen(filename,"a");
    fprintf(log,"ICMP from port: %d, src: %hhu.%hhu.%hhu.%hhu, dst: %hhu.%hhu.%hhu.%hhu, type: %d\n",proxyPort,buf[12],buf[13],buf[14],buf[15],buf[16],buf[17],buf[18],buf[19],buf[20]);
    fclose(log);

    char reply[numbytes];//reply to be sent back
    
    makeReply(reply,numbytes);

    //sends reply packet to proxy
    if ((numbytes = sendto(sockfd, reply, numbytes, 0,p->ai_addr, p->ai_addrlen)) == -1) 
    {
      perror("talker: sendto");
      exit(1);
    }
  }
}

//send a hello message to the proxy for stage 1
void Router::sendHello()
{
  char hostName[50];
  int hostLen = 50;
  gethostname(hostName,hostLen);//get this systems host name
  char portStr[20];
  sprintf(portStr,"%d",proxyPort);//set this port number to proxy's port number

  //get the address info of the proxy port number
  if ((rv = getaddrinfo(hostName, portStr, &hints, &servinfo)) != 0) 
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(0);
  }

  p = servinfo;//store address infor of proxy into p
  freeaddrinfo(servinfo);
  char send[MAXBUFLEN];//message to be sent to proxy
  sprintf(send,"rtrInit %d %d",routerNum,pid);

  //send message to proxy
  if ((numbytes = sendto(sockfd, send, strlen(send), 0,p->ai_addr, p->ai_addrlen)) == -1) 
  {
    perror("talker: sendto");
    exit(1);
  }
  
  

  printf("router%d: sent %d bytes to proxy\n",routerNum, numbytes);
}

//bind socket to this router. most of this function taken from beej's guide
void Router::bindSocket()
{
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  

  //if ((rv = getaddrinfo(hostName, portStr, &hints, &servinfo)) != 0) 
  
  if ((rv = getaddrinfo(NULL, "0", &hints, &servinfo)) != 0) 
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(0);
  }
    // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) 
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
    {
      perror("talker: socket");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)//bind socket
    {
      close(sockfd);
      perror("sender: bind");
      continue;
    }

    break;
  }


  if (p == NULL) 
  {
    fprintf(stderr, "talker: failed to bind socket\n");
    exit(0);
  }

  freeaddrinfo(servinfo);

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)//taken from stack overflow
    perror("getsockname");
  else
  {
    printf("port number %d\n", ntohs(sin.sin_port));
    port = ntohs(sin.sin_port);//get this routers dynamic port number
  }

  inet_ntop(AF_INET, &sin,myUDPIP, 50);
  printf("%s\n",myUDPIP);


  log = fopen(filename,"w");//print router info to its log file
  if(stage>=5)
    fprintf(log,"router: %d, pid: %d, port: %d, IP: %s\n",routerNum,pid,port,myUDPIP);
  else
    fprintf(log,"router: %d, pid: %d, port: %d\n",routerNum,pid,port);
  fclose(log);

}

//overall flow of router code
void Router::start()
{
  cout<<"router"<<routerNum<<" "<<proxyPort<<" "<<pid<<"\n";
  /*log = fopen(filename,"a");
  fprintf(log,"router: %d, pid: %d, port: %d\n",routerNum,pid,proxyPort);
  fclose(log);*/

  if(stage>=3)
    bindRawSock();

  bindSocket();

  

  sendHello();

  //testStuff();

  if(stage==2)
    stage2Listen();
  if(stage>2 && stage<5)
    stage3Listen();
  if(stage>=5)
    stage5Listen();
}

//function from ping.c to compute checksums
int Router::in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
