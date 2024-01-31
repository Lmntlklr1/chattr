#include "socket.h"
#include <iostream>

using namespace std;

// Maximum users
#define MAX_CONNECTIONS 20
#define MAX_USERS 100
#define MAX_ROOMS 10
#define MAX_USERS_PER_ROOM 10

class User;
class Server;

struct Connection {
  SOCKET sock;
  struct in_addr addr;
  string user_id;
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
  Connection* GetConnectionFromSocket(SOCKET sock);
private:
  SOCKET sock;
  short port;
  fd_set fds;
  SOCKET max_fd;
  Connection connections[MAX_CONNECTIONS];
  /* User users[MAX_USERS]; */
};

void server_run();
