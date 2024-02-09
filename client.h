#include "socket.h"
#include <iostream>

using namespace std;

struct ListenThread
{
	SOCKET sock;
	HANDLE hWritePipe;
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
  HANDLE hReadPipe;
};