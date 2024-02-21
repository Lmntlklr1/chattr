#include "server.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>
#include <cstring>
#include <sstream>

int Server::init() {
  FD_ZERO(&fds);

  int errorcheck = gethostname(hostname, HOSTNAME_LENGTH);
  if (errorcheck != 0)
  {
      cout << "Could not gather Host Name. \n";
      return SOCKET_ERROR;
  }
  addrinfo AddrInfo = { 0 , AF_INET, SOCK_STREAM, IPPROTO_TCP, 0 };
  addrinfo* OutAddr;
  //free structure
  errorcheck = getaddrinfo(hostname, NULL, &AddrInfo, &OutAddr);
  if (errorcheck != 0)
  {
      cout << "Could not gather Address Information.";
      return -1;
  }

  char Ip[100];
  Ip[0] = 0;
  while (OutAddr != NULL)
  {
      if (OutAddr->ai_addr != NULL)
      {
          in_addr inAddr = reinterpret_cast<sockaddr_in*>(OutAddr->ai_addr)->sin_addr;
          inet_ntop(OutAddr->ai_family, &inAddr, Ip, 100);
          break;
      }
      OutAddr = OutAddr->ai_next;
  }
  cout << "Hostname: " << hostname << "\n";
  cout << "IPv4 Address: " << Ip << "\n";
  cout << "Enter a port: ";
  cin >> port;

  //prompt for commandChar
  commandChar = '~';

  cout << "Making sock on port " << port << endl;
  cout << "How many Users are looking to chat: ";
  cin >> maxConnections;
  sock = makeServerSocket(port);
  if (sock < 0)
    return -1;
  std::cout << "Socket bound and listening...\n";

  FD_SET(sock, &fds);
  max_fd = sock;
  return 0;
}

shared_ptr<Connection> Server::GetConnectionFromSocket(SOCKET sock)
{
    for (auto &connection : connections)
    {
        if (connection->sock == sock)
        {
            return connection;
        }
    }
    return nullptr;
}

int Server::handleConnect(SOCKET new_socket, struct in_addr addr) {
  if (connections.size() == maxConnections) {
    sendString(new_socket, "No more room on this sorry.");
    closesocket(new_socket);
    return 0;
  }
  sendString(new_socket, "Welcome!");
  if (new_socket > max_fd)
    max_fd = new_socket;
  // We're accepting this connection. Add it to our list
  shared_ptr<Connection> cnct = make_shared<Connection>(Connection(new_socket, addr));
  connections.push_back(cnct);
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
  shared_ptr<Connection> cnct = GetConnectionFromSocket(skt);
  if (message[0] == commandChar)
  {
      string commandWord;
      istringstream messageStream(message);
      // skip command char
      messageStream.get();
      messageStream >> commandWord;
      if (commandWord == "help")
      {
          sendString(skt, "~register to register a username and password before chatting");
      }
      else if (commandWord == "register")
      {
          string username;
          string password;
          int i = 1;
          messageStream >> username >> password;
          Register(cnct, username, password);
      }
      else
      {
          sendString(skt, "Command not recognized");
      }
  }
  char buffer[100];
  int ifcheck = sscanf_s(message.c_str(), "register %s %s", buffer, 100);
  if (ifcheck == 1)
  {
      buffer[99] = '\0';
      cnct->user_id = buffer;
  }
  ifcheck = sscanf_s(message.c_str(), "disconnect %s", buffer, 100);
  if (ifcheck == 1)
  {
      buffer[99] = '\0';
      cnct->user_id = buffer;
      closesocket(cnct->sock);
  }
  ifcheck = sscanf_s(message.c_str(), "help", buffer, 100);
  if (ifcheck == 1)
  {
      buffer[99] = '\0';
      cnct->user_id = buffer;
  }
  cout << "Recieved " <<  message << "\n";
  sprintf_s(buffer, "%s : %s", cnct->user_id.c_str(), message.c_str());
  
  for (auto& connection : connections) {
    SOCKET sock = connection->sock;
    if (sock) {
      sendString(sock, buffer);
    }
  }
  return 1;
}

int Server::Register(shared_ptr<Connection> cnct, string username, string pass) {
    if (cnct->user_id != "") {
        sendString(cnct->sock, "You are already logged in.");
        return 0;
    }
    if (users.find(username) != users.end()) {
        sendString(cnct->sock, "Username has already already taken.");
        return 0;
    }
    users[username] = make_shared<User>(User(username, pass));
    ostringstream os;
    os << "Welcome " << username << ".\n"
        << "You are now registered.";
    string registerStr;
    os.str(registerStr);
    sendString(cnct->sock, registerStr);
    return 0;
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


