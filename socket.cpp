#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>


void SocketError(string str) {
    cout << str << " : " << WSAGetLastError() << "\n";
}

int set_socket_nonblocking(SOCKET sock) {
  printf("Setting socket %llu as non-blocking\n", (unsigned long long)sock);
  u_long mode = 1; // 1 to enable non-blocking socket
  if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
    closesocket(sock);
    SocketError("Error: Unable to set socket as non-blocking");
    return -1;
  }
  return 0;
}

SOCKET makeClientSocket(const string ip_addr, int port) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
      SocketError("Error: Couldn't make socket");
    return -1;
  }

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
  addr.sin_port = htons(port);
  int connect_result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (connect_result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
    closesocket(sock);
    SocketError("Error: Unable to connect");
    return -1;
  }
  if (set_socket_nonblocking(sock) < 0) {
    return -1;
  }

  return sock;
}

SOCKET makeServerSocket(int port) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    SocketError("Server: Couldn't make socket");
    return -1;
  }
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  int bind_result = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (bind_result == SOCKET_ERROR) {
    closesocket(sock);
    SocketError("Server: Couldn't bind to port");
    return -1;
  }
  // Hardcode backlog of 20.socket
  int listen_result = listen(sock, 20);
  if (listen_result == SOCKET_ERROR) {
    closesocket(sock);
    SocketError("Server: Couldn't listen to port");
    return -1;
  }
  if (set_socket_nonblocking(sock) < 0) {
    return -1;
  }

  return sock;
}

SOCKET acceptConnection(SOCKET sock, struct in_addr *client_addr) {
  struct sockaddr_in addr = {0};
  int addrlen = sizeof(addr);
  SOCKET new_sock = accept(sock, (struct sockaddr *)&addr, &addrlen);
  if (new_sock == INVALID_SOCKET) {
    SocketError("Server: Couldn't accept connection");
    return INVALID_SOCKET;
  }
  memcpy(client_addr, &addr.sin_addr, sizeof(struct in_addr));
  if (set_socket_nonblocking(new_sock) < 0) {
    return INVALID_SOCKET;
  }

  return new_sock;
}

RETURNCODE sendString(SOCKET sock, string str) {
  if (str.length() != (unsigned char)str.length())
  {
      cout << "String length is too long to send.\n";
      return MESSAGE_ERROR;
  }
  unsigned char length = (unsigned char)str.length();
  char *buffer = new char [1 + (size_t)length];
  if (buffer == NULL) {
    fprintf(stderr, "Unable to allocate buffer to send string. Aborting..\n");
    closesocket(sock);
    return MESSAGE_ERROR;
  }
  // Store the length first
  *buffer = (char)(unsigned char)(length);
  // Then the string
  memcpy(buffer + 1, str.c_str(), length);
  int send_result = send(sock, buffer, length + 1, 0);
  if (send_result == SOCKET_ERROR) {
      int error = WSAGetLastError();
      if (error == WSAESHUTDOWN)
      {
          return SHUTDOWN;
      }
      else
      {
          SocketError("Unable to send message");
          closesocket(sock);
          return DISCONNECT;
      }
  }
  return SUCCESS;
}

RETURNCODE recvString(SOCKET sock, string* str) {
  unsigned char length;
  int recv_result;
  recv_result = recv(sock, (char *)&length, 1, 0);

  if (recv_result == SOCKET_ERROR) {
      int error = WSAGetLastError();
    if (error == WSAEWOULDBLOCK) {
      // Not a real error code. Ignore
      return SUCCESS;
    }
    else if (error == WSAESHUTDOWN)
    {
        return SHUTDOWN;
    }

    else
    {
        SocketError("Error: Unable to recv message length");
        closesocket(sock);
        return DISCONNECT;
    }
  } else if (recv_result == 0) {
    return DISCONNECT; // Connection has been gracefully closed
  } 
  char* buffer = new char[(size_t)length + 1];
  recv_result = recv(sock, buffer, length, 0);
  if (recv_result == SOCKET_ERROR) {
    SocketError("Unable to recv message body");
    closesocket(sock);
    return MESSAGE_ERROR;
  } else if (recv_result < length) {
    fprintf(stderr, "Incomplete packet. Only %d/%d bytes received\n", recv_result + 1, length + 1);
    closesocket(sock);
    return MESSAGE_ERROR;
  }
  buffer[length] = '\0';
  *str = buffer;
  return SUCCESS;
}

string inetToString(struct in_addr addr) {
  char* buffer = new char[22];
  inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN);
  return string(buffer);
}