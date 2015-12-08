#include "Proxy.h"

//Simple cpp file to contain main. Processes the command line input and starts the real code.
int main(int argc, char* argv[])
{
  if(argc<2)
  {
    cout<<"insufficient command line parameters"<<"\n";
    return 0;
  }
  Proxy p(argv[1]);
  p.start();
}
