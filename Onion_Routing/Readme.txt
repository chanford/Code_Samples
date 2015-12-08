For the full explanation of what this assignemnt was, see projb_7pm_current.pdf in the Minitor directory.

In summary, this project was to implement an Onion routing network https://en.wikipedia.org/wiki/Onion_routing in c/c++ that is able to send/recieve pings and their responses to/from the real internet. The environment for this assignment was a Fedora virtual machine provided to us by the professor. 

In that vm, pings to a certain ip range would be sent into a tunnel to my Proxy program (Proxy.cpp). The Proxy contacts virtual Router programs that I created (Router.cpp) to setup an onion routing network. The config file determines how many routers are spawned and how many hops are made between routers before the ping is sent out to the internet. After sending the ping out into the internet, the ping reply is sent back through the same virtual routers in reverse order to the Proxy. The Proxy sends the ping response back through the tunnel it received the ping from and the response is shown on the shell that the original "ping" command was run on.


projb/ contains the code I wrote
the sampleout directories contain samples of what the Proxy and routers would log during hte program's operation, detailing their communicaitons with the other routers and proxy.