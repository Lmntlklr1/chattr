#include "socket.h"
#include <iostream>

using namespace std;

struct ListenThread
{
	SOCKET sock;
	static DWORD WINAPI run(LPVOID lpParameter);
};

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
  ListenThread lt;
};