#include "socket.h"
#include <iostream>

using namespace std;

class Client {
public:  
  int init();
  int processMessages();
  void run();

private:
  SOCKET sock;
  string server;
  short port;
  string username;
};