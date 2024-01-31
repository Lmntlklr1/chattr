#include "socket.h"

// Maximum users
#define MAX_CONNECTIONS 20
#define MAX_USERS 100
#define MAX_ROOMS 10
#define MAX_USERS_PER_ROOM 10

class User;
class Server;

class Connection {
  SOCKET socket;
  struct in_addr addr;
  User *user_id;
};

/* class User { */
/*   string username; */
/*   string password; */
/*   Connection *connection; */
/* }; */

class Server {
  SOCKET socket;
  short port;
  fd_set fds;
  SOCKET max_fd;
  Connection connections[MAX_CONNECTIONS];
  /* User users[MAX_USERS]; */
};

void server_run();
