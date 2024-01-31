#include <winsock2.h>
#include <iostream>

using namespace std;

SOCKET makeClientSocket(string ip_addr, int port);
SOCKET makeServerSocket(int port);
SOCKET acceptConnection(SOCKET sock, struct in_addr *client_addr);
int sendString(SOCKET sock, string str);
int recvMessage(SOCKET sock, string *buffer);
void inetToString(struct in_addr addr, string buffer);
