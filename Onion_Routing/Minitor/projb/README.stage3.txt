a. In addition to the reused code from previous sections, The code used for binding to raw sockets and sending messages out of them was adapted tocode givento me by the TA Zi Hu.

b. This section is complete except for the IP binds to the 192.168.20X.2 addresses for inter-router communication. The program still works, but when a router prints out its IP address in the .out file of any stage, it will be a randomly chosen ip bound to the UDP socket instead of 192.168.201.2.

c. If the source address isn't rewritten then the ping reply wont be sent to the router. It will be sent directly to the eth0 address instead. It will miss the minitor system alltogether.

d. Each router needs a seperate ip so that each one can send/receive its own messages. One of the points of tor is to have a random exit router for each circuit for messages going to the normal internet. Since we are only running on 1 virtual machine; to provide the illusion of actual different routers, each one needs a different network device that each has their own ip.

e. The host computer sets up a NAT for the emulated os. The NAT converts the packets to the host machine's ip and sends them off. When the replies return, it reconverts the packets back to the internal ip space.
