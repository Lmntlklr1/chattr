#include "socket.h"
#include <iostream>
#include <vector>
#include <map>

using namespace std;

// Maximum users
#define MAX_ROOMS 10
#define MAX_USERS_PER_ROOM 10
#define HOSTNAME_LENGTH 100

class Server;

struct Connection {
  SOCKET sock;
  struct in_addr addr;
  string user_id;
  Connection(SOCKET sock, in_addr addr) : sock(sock), addr(addr), user_id("") {}
  //~Connection();
  void Close();
};

struct User {
	string username;
	string password;
	User(string username, string password) : username(username), password(password) {}
 };

class Server {
public:
  int init();
  int handleConnect(SOCKET new_socket, struct in_addr addr);
  int processMessage(SOCKET sock);
  void run();
  shared_ptr<Connection> GetConnectionFromSocket(SOCKET sock);
  int Register(shared_ptr<Connection> cnct, string username, string pass);
  int sendMessage(shared_ptr<Connection> cnct, string str);
  int recvMessage(shared_ptr<Connection> cnct, string* buffer);
  void RemoveConnection(shared_ptr<Connection> cnct);
  string GetIPAddress(int family, int stream, int protocol);
  int Login(shared_ptr<Connection>cnct, string username, string pass);
  int Logout(shared_ptr<Connection> cnct);
  int SendDirectMessage(shared_ptr<Connection> cnct, string username, string str);
  int GetLog(shared_ptr<Connection> cnct);
  int GetList(shared_ptr<Connection> cnct);
private:
  SOCKET sock;
  short port;
  fd_set fds;
  SOCKET max_fd;
  int maxConnections;
  vector<shared_ptr<Connection>> connections;
  char hostname[HOSTNAME_LENGTH];
  char commandChar;
  map<string, shared_ptr<User>> users;
  /* User users[MAX_USERS]; */
};

void server_run();
