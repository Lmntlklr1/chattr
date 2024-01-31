#ifdef _WIN32
#include "util.h"
#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

int set_socket_nonblocking(SOCKET sock) {
  printf("Setting socket %llu as non-blocking\n", (unsigned long long)sock);
  u_long mode = 1; // 1 to enable non-blocking socket
  if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
    closesocket(sock);
    perror("Error: Unable to set socket as non-blocking");
    return -1;
  }
  return 0;
}

SOCKET make_client_socket(const char *ip_addr, int port) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    perror("Error: Couldn't make socket");
    return -1;
  }

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip_addr);
  addr.sin_port = htons(port);
  int connect_result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (connect_result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
    closesocket(sock);
    perror("Error: Unable to connect");
    return -1;
  }
  if (set_socket_nonblocking(sock) < 0) {
    return -1;
  }

  return sock;
}

SOCKET make_server_socket(int port) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    perror("Server: Couldn't make socket");
    return -1;
  }
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  int bind_result = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (bind_result == SOCKET_ERROR) {
    closesocket(sock);
    perror("Server: Couldn't bind to port");
    return -1;
  }
  // Hardcode backlog of 20.
  int listen_result = listen(sock, 20);
  if (listen_result == SOCKET_ERROR) {
    closesocket(sock);
    perror("Server: Couldn't listen to port");
    return -1;
  }
  if (set_socket_nonblocking(sock) < 0) {
    return -1;
  }

  return sock;
}

SOCKET accept_connection(SOCKET sock, struct in_addr *client_addr) {
  struct sockaddr_in addr = {0};
  int addrlen = sizeof(addr);
  SOCKET new_sock = accept(sock, (struct sockaddr *)&addr, &addrlen);
  if (new_sock == INVALID_SOCKET) {
    perror("Server: Couldn't accept connection");
    return -1;
  }
  memcpy(client_addr, &addr.sin_addr, sizeof(struct in_addr));
  if (set_socket_nonblocking(new_sock) < 0) {
    return -1;
  }

  return new_sock;
}

int send_string(SOCKET sock, char *string) {
  unsigned short length = strlen(string);
  char *buffer = malloc(2 + length);
  if (buffer == NULL) {
    fprintf(stderr, "Unable to allocate buffer to send string. Aborting..\n");
    closesocket(sock);
    return -1;
  }
  // Store the length first
  *(unsigned short *)buffer = htons(length);
  // Then the string
  memcpy(buffer + 2, string, length);
  int send_result = send(sock, buffer, length + 2, 0);
  if (send_result == SOCKET_ERROR) {
    perror("Unable to send message");
    free(buffer);
    closesocket(sock);
    return -1;
  }
  free(buffer);
  return 0;
}

int recv_message(SOCKET sock, char **buffer) {
  unsigned short length;
  int recv_result;
  recv_result = recv(sock, (char *)&length, 2, MSG_WAITALL);

  if (recv_result == SOCKET_ERROR) {
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      // Not a real error code. Ignore
      return 0;
    }
    perror("Error: Unable to recv message length");
    closesocket(sock);
    return -1;
  } else if (recv_result == 0) {
    return 0; // Connection has been gracefully closed
  } else if (recv_result < 2) {
    fprintf(stderr, "Incomplete packet. Only %d/%d bytes received\n", recv_result, 2);
    closesocket(sock);
    return -1;
  }
  length = ntohs(length);
  *buffer = (char *)realloc(*buffer, length + 1);
  recv_result = recv(sock, *buffer, length, MSG_WAITALL);
  if (recv_result == SOCKET_ERROR) {
    perror("Unable to recv message body");
    closesocket(sock);
    return -1;
  } else if (recv_result < length) {
    fprintf(stderr, "Incomplete packet. Only %d/%d bytes received\n", recv_result + 2, length + 2);
    closesocket(sock);
    return -1;
  }
  (*buffer)[length] = '\0';
  return length;
}

void inet_to_string(struct in_addr addr, char *buffer) {
  inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN);
}
#endif
