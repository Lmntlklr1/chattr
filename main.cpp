#include "client.h"
#include "server.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

int main() {
  char mode;
  cout << "Client or server? ";
  cin >> mode;

  if (mode == 'c') {
    Client *client = new Client();
    client->run();
  } else if (mode == 's') {
    Server *server = new Server();
    server->run();
  } else {
    printf("Invalid input.\n");
  }
}
