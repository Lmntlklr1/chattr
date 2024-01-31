#include "client.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

int Client::init() {
  cout << "Enter an IP address: " << endl;
  cin >> server;
  cout << "Enter a port: " << endl;
  cin >> port;

  cout << "Connecting to " << server << "on port " << port << endl;
  sock = makeClientSocket(server, port);
  if (sock < 0)
    return -1;
  else
    printf("Successfully connected!\n");

  cout << "Enter a username: " <<endl;
  cin >> username;

  stringstream ss;
  ss << "/identify " <<username;
  if (sendString(sock, ss.str()) < 0) {
    return -1;
  }

  return 0;
}

int Client::processMessages() {
  int response = 0;
  string message;
  int count = 0;
  do {
    response = recvMessage(sock, &message);
    if (response < 0) {
      cout << "Could not get message from server\n";
      break;
    } else if (response > 0) {
      cout << message <<endl;
      count++;
    }
  } while (response != 0);
  if (response < 0) {
    return response;
  } else {
    return count;
  }
}

void Client::run() {
  if (init() < 0) {
    cout << "Aborting client..";
    return;
  }
  string buffer;
  while (1) {
    processMessages();
    printf("\r> ");
    getline(cin, buffer);
    if (buffer.length() <= 1) {
      // Empty message. Just pull new messages again.
      continue;
    }
  //  buffer[buffer.length()] = '\0';
    cout << "Sending " << buffer << endl;
    if (sendString(sock, buffer) < 0) {
      cout << "Error sending message\n";
      continue;
    }
  }
}
