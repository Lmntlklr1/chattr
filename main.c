#include "client.h"
#include "server.h"
#include "socket.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int query_choice(char *string, char *options) {
  do {
    printf("%s [%s]: ", string, options);
    int answer = getchar();
    while (getchar() != '\n'); // Flush line
    for (int i = 0; options[i]; i++) {
      if (tolower(options[i]) == tolower(answer)) {
        return options[i];
      }
    }
    printf("Invalid option %c\n", answer);
  } while (1);
}

int main() {
  int mode = query_choice("Client or server?", "cs");

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
