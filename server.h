#include "socket.h"
#include <iostream>
#include <vector>

using namespace std;

// Maximum users
#define MAX_ROOMS 10
#define MAX_USERS_PER_ROOM 10
#define HOSTNAME_LENGTH 100

class User;
class Server;

struct Connection {
  SOCKET sock;
  struct in_addr addr;
  string user_id;
  Connection(SOCKET sock, in_addr addr) : sock(sock), addr(addr), user_id("") {}
};

/* class User { */
/*   string username; */
/*   string password; */
/*   Connection *connection; */
/* }; */

class Server {
public:
  int init();
  int handleConnect(SOCKET new_socket, struct in_addr addr);
  int processMessage(SOCKET sock);
  void run();
  shared_ptr<Connection> GetConnectionFromSocket(SOCKET sock);
private:
  SOCKET sock;
  short port;
  fd_set fds;
  SOCKET max_fd;
  int maxConnections;
  vector<shared_ptr<Connection>> connections;
  char hostname[HOSTNAME_LENGTH];
  /* User users[MAX_USERS]; */
};

void server_run();
