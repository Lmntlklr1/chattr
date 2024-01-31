#include "socket.h"
#include <iostream>

int awaitMessage(int sfd, string *buffer) {
  int recv_result;
  do {
    recv_result = recvMessage(sfd, buffer);
  } while (recv_result == 0);
  return recv_result;
}
