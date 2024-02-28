#pragma once
#include <winsock2.h>
#include <iostream>


using namespace std;

typedef int RETURNCODE;
const RETURNCODE SUCCESS = 0, SHUTDOWN = 1, DISCONNECT = 2,
BIND_ERROR = 3, CONNECT_ERROR = 4, SETUP_ERROR = 5,
STARTUP_ERROR = 6, ADDRESS_ERROR = 7, PARAMETER_ERROR = 8, MESSAGE_ERROR = 9;

SOCKET makeClientSocket(string ip_addr, int port);
SOCKET makeServerSocket(int port);
SOCKET acceptConnection(SOCKET sock, struct in_addr *client_addr);
RETURNCODE sendString(SOCKET sock, string str);
RETURNCODE recvString(SOCKET sock, string *buffer);
void inetToString(struct in_addr addr, string buffer);
void SocketError(string str);


