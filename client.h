#include "socket.h"
#include <iostream>

using namespace std;

#define EVENT_COUNT 2

struct ListenThread
{
	SOCKET sock;
	static DWORD WINAPI run(LPVOID lpParameter);
	int processMessages();
};



class Client {
public:  
  int init();
  void run();

private:
  SOCKET sock;
  string server;
  short port;
  string username;
  ListenThread lt;
};