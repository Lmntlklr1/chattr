#include "server.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Server::init() {
  FD_ZERO(&fds);

  cout << "Enter a port: ";
  cin >> port;

  cout << "Making sock on port " << port << endl;
  sock = makeServerSocket(port);
  if (sock < 0)
    return -1;
  std::cout << "Socket bound and listening...\n";

  FD_SET(sock, &fds);
  max_fd = sock;
  return 0;
}

Connection* Server::GetConnectionFromSocket(SOCKET sock)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (connections[i].sock == sock)
        {
            return &connections[i];
        }
    }
    return NULL;
}

int Server::handleConnect(SOCKET new_socket, struct in_addr addr) {
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    // sock == 0 means the connection slot is free.
    if (connections[i].sock == 0)
      break;
  }
  if (i == MAX_CONNECTIONS) {
    sendString(new_socket, "No more room on this  sorry.");
    closesocket(new_socket);
    return -1;
  }
  sendString(new_socket, "Welcome!");
  if (new_socket > max_fd)
    max_fd = new_socket;
  // We're accepting this connection. Add it to our list
  connections[i].sock = new_socket;
  connections[i].addr = addr;
  FD_SET(new_socket, &fds);
  return 0;
}

int Server::processMessage(SOCKET skt) {
  string message;
  int recv_result = recvMessage(skt, &message);
  if (recv_result < 0) {
    cout << "Error " << recv_result << "\n";
    return -1;
  } else if (recv_result == 0) {
    // Nothing to read
    return 0;
  }
  Connection* cnct = GetConnectionFromSocket(skt);
  char buffer[100];
  int ifcheck = sscanf_s(message.c_str(), "/identify %s", buffer, 100);
  if (ifcheck == 1)
  {
      buffer[99] = '\0';
      cnct->user_id = buffer;
  }
  ifcheck = sscanf_s(message.c_str(), "/disconnect %s", buffer, 100);
  if (ifcheck == 1)
  {
      buffer[99] = '\0';
      cnct->user_id = buffer;
      closesocket(cnct->sock);
  }
  cout << "Recieved " <<  message << "\n";
  sprintf_s(buffer, "%s : %s", cnct->user_id.c_str(), message.c_str());
  
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    SOCKET sock = connections[i].sock;
    if (sock) {
      sendString(sock, buffer);
    }
  }
  return 1;
}

void Server::run() {
  if (init() < 0)
    return;

  while (1) {
    fd_set read_fds = fds;
    int select_result = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    if (select_result < 0) {
      cout << "Error select failed";
      return;
    } else if (select_result == 0) {
      continue;
    }
    /* printf("Select returned %d\n", select_result); */
    /* printf("read_fds = %d\n", ((unsigned int*)&read_fds)[0]); */
    for (int i = 0; i <= max_fd; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == sock) {
          // New connection
          struct in_addr addr;
          SOCKET new_socket = acceptConnection(sock, &addr);
          if (new_socket < 0)
            return;
          if (handleConnect(new_socket, addr) < 0) {
            return;
          }
        } else {
          if (processMessage(i) < 0)
            return;
        }
      }
    }
  }

  struct in_addr addr = {0};
  int connected_socket = acceptConnection(sock, &addr);
  if (connected_socket < 0)
    cout << "Errror";
  cout << "Accepting connection on " << connected_socket << endl;
  string buffer;
  int count = recvMessage(connected_socket, &buffer);
  if (count < 0) {
    cout << "Aborting server.\n";
    return;
  } else if (count > 0) {
    cout << buffer << endl;
  } else {
    cout << "Recieved empty message.\n";
  }

  return;

}
