#include "client.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Client::init() {
  cout << "Enter an IP address: " << endl;
  cin >> server;
  cout << "Enter a port: " << endl;
  cin >> port;

  cout << "Connecting to " << client->server << "on port " << client->port << endl;
  socket = makeClientSocket(client->server, client->port);
  if (socket < 0)
    return -1;
  else
    printf("Successfully connected!\n");

  cout << "Enter a username: " <<endl;
  cin >> client->username;

  stringstream ss;
  ss << "/identify " <<username;
  if (sendString(socket, ss.str()) < 0) {
    return -1;
  }

  return 0;
}

int Client::processMessages() {
  int response = 0;
  string message;
  int count = 0;
  do {
    response = recvMessage(client->socket, &message);
    if (response < 0) {
      cout << "Could not get message from server\n";
      break;
    } else if (response > 0) {
      printf("%s\n", message);
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
  if (client_init(&client) < 0) {
    cout << "Aborting client..";
    return;
  }
  string buffer = NULL;
  while (1) {
    processMessages(&client);
    printf("\r> ");
    int getline_result = getline(cin, buffer);
    if (getline_result < 0) {
      perror("Error occurred while reading line");
      continue;
    } else if (getline_result <= 1) {
      // Empty message. Just pull new messages again.
      continue;
    }
    buffer[getline_result-1] = '\0';
    printf("Sending '%s'\n", buffer);
    if (sendString(socket, buffer) < 0) {
      cout << "Error sending message\n";
      continue;
    }
  }
}
