#include "server.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int Server::init() {
  FD_ZERO(&fds);

  cout << "Enter a port: ";
  cin >> port;

  cout << "Making socket on port " << port << endl;
  socket = make_socket(port);
  if (socket < 0)
    return -1;
  std::cout << "Socket bound and listening...\n";

  FD_SET(socket, &fds);
  max_fd = socket;
  return 0;
}

int Server::handleConnect(SOCKET new_socket, struct in_addr addr) {
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    // socket == 0 means the connection slot is free.
    if (connections[i].socket == 0)
      break;
  }
  if (i == MAX_CONNECTIONS) {
    send_format(new_socket, "No more room on this  sorry.");
    close(new_socket);
    return -1;
  }
  send_format(new_socket, "Welcome!");
  if (new_socket > max_fd)
    max_fd = new_socket;
  // We're accepting this connection. Add it to our list
  connections[i].socket = new_socket;
  connections[i].addr = addr;
  connections[i].user_id = NULL;
  connections[i].room = NULL;
  FD_SET(new_socket, &fds);
  return 0;
}

int Server::processMessage(SOCKET skt) {
  string message;
  int recv_result = recvMessage(sdk, &message);
  if (recv_result < 0) {
    cout << "Error " << recv_result;
    return -1;
  } else if (recv_result == 0) {
    // Nothing to read
    return 0;
  }
  cout << "Recieved " << message;
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    SOCKET socket = connections[i].socket;
    if (socket) {
      sendString(socket, message);
    }
  }
  return 1;
}

void Server::run() {
  if (init() < 0)
    goto abort;

  while (1) {
    fd_set read_fds = fds;
    int select_result = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    if (select_result < 0) {
      cout << "Error select failed";
      goto abort;
    } else if (select_result == 0) {
      continue;
    }
    /* printf("Select returned %d\n", select_result); */
    /* printf("read_fds = %d\n", ((unsigned int*)&read_fds)[0]); */
    for (int i = 0; i <= max_fd; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == socket) {
          // New connection
          struct in_addr addr;
          SOCKET new_socket = acceptConnection(socket, &addr);
          if (new_socket < 0)
            goto abort;
          if (handleConnect(&new_socket, addr) < 0) {
            goto abort;
          }
        } else {
          if (processMessage(i) < 0)
            goto abort;
        }
      }
    }
  }

  struct in_addr addr = {0};
  int connected_socket = acceptConnection(socket, &addr);
  if (connected_socket < 0)
    cout << "Errror";
  cout << "Accepting connection on " << connected_socket << endl;
  string buffer;
  int count = recvMessage(connected_socket, &buffer);
  if (count < 0) {
    cout << "Aborting server.\n";
    return;
  } else if (count > 0) {
    printf("Recieved '%s'\n", buffer);
  } else {
    printf("Recieved empty message.\n");
  }

  return;

}
