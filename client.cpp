#include "client.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <processthreadsapi.h>
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

  lt.sock = sock;
  lt.consolePtr = make_shared<ConsoleStruct>();
  consolePtr = lt.consolePtr;
  // lt's lifetime is shared with the clients, however lt is not valid until now
  CreateThread(NULL, 0, &ListenThread::run, reinterpret_cast<LPVOID>(&lt), 0, NULL);
  
  cout << "Enter a username: " << endl;
  cin >> username;

  stringstream ss;
  ss << "/identify " <<username;
  if (sendString(sock, ss.str()) < 0) {
    return -1;
  }

  return 0;
}

void Client::run() {
  if (init() < 0) {
    cout << "Aborting client..";
    return;
  }
  string buffer;
  while (1) {
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

// TODO:: Error Checking on WriteFile
int ListenThread::processMessages() {
  int response = 0;
  string message;
  int count = 0;
  do {
    response = recvMessage(sock, &message);
    if (response < 0) {
      cout << "Could not get message from server\n";
      break;
    } else if (response > 0) {
      message.push_back('\n');
      DWORD bytesWritten;
      count++;
    }
  } while (response != 0);
  if (response < 0) {
    return response;
  } else {
    return count;
  }
}

DWORD __stdcall ListenThread::run(LPVOID lpParameter)
{
    ListenThread* ltr = reinterpret_cast<ListenThread*>(lpParameter);
    int return_code;
    do
    {
        return_code = ltr->processMessages();
    } while (return_code > 0);
    return 0;
}
