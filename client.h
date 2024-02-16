#include "socket.h"
#include <iostream>
#include <mutex>
#include <curses.h>

using namespace std;

#define EVENT_COUNT 2

struct ConsoleStruct
{
	mutex mtx;
	HANDLE hConsole;
};

struct ListenThread
{
	SOCKET sock;
	static DWORD WINAPI run(LPVOID lpParameter);
	int processMessages();
	shared_ptr<ConsoleStruct> consolePtr;
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
  shared_ptr<ConsoleStruct> consolePtr;
};