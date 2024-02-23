#include "server.h"
#include "message.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>
#include <cstring>
#include <sstream>
#include "socket.h"
#include <algorithm>

int Server::init() {
  FD_ZERO(&fds);

  int errorcheck = gethostname(hostname, HOSTNAME_LENGTH);
  if (errorcheck != 0)
  {
      cout << "Could not gather Host Name. \n";
      return -1;
  }
  cout << "Hostname: " << hostname << "\n";
  cout << "IPv4 Address: " << GetIPAddress(AF_INET, SOCK_STREAM, IPPROTO_TCP) << "\n";
  cout << "IPv6 Address: " << GetIPAddress(AF_INET6, SOCK_STREAM, IPPROTO_TCP) << "\n";
  cout << "Enter a port: ";
  cin >> port;

  //prompt for commandChar
  cout << "Command character: ";
  cin >> commandChar;
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
  if (new_socket > max_fd)
    max_fd = new_socket;
  FD_SET(new_socket, &fds);
  // We're accepting this connection. Add it to our list
  shared_ptr<Connection> cnct = make_shared<Connection>(Connection(new_socket, addr));
  connections.push_back(cnct);
  ostringstream message;
  message << "Welcome, the default character for issueing commands is " << commandChar << ".\n" << "Use the /help command to find out any other commands.\n";
  if (sendMessage(cnct, message.str()) < 0)
  {
      return -1;
  }
  return 0;
}

int Server::processMessage(SOCKET skt) {
  shared_ptr<Connection> cnct = GetConnectionFromSocket(skt);
  string message;
  int recv_result = recvString(skt, &message);
  if (recv_result != 0) {
    cout << "Error " << recv_result << "\n";
    if (recv_result == SHUTDOWN)
    {
        closesocket(cnct->sock);
        cout << "Shutdown socket trying to process message: SHUTDOWN\n";
        return 0;
    }
    if (recv_result == DISCONNECT)
    {
        shutdown(cnct->sock, SD_BOTH);
        RemoveConnection(cnct);
        cout << "Disconnected trying to process message: DISCONNECT\n";
        return 0;
    }
    else
    {
        shutdown(cnct->sock, SD_BOTH);
        RemoveConnection(cnct);
        cout << "Encountered error trying to process message: "  << recv_result << "\n";
        return 0;
    }
  } else if (message.length() == 0) {
    // Nothing to read
    return 0;
  }
  if (message[0] == commandChar)
  {
      string commandWord;
      istringstream messageStream(message);
      // skip command char
      messageStream.get();
      messageStream >> commandWord;
      if (commandWord == "help")
      {
          ostringstream helpMessage;
          helpMessage << "Here are all the available commands:\n\t"
              << commandChar << "register <username <password> to register a username and password before chatting\n\t"
              << commandChar << "help to find all available commands\n";
          sendMessage(cnct, helpMessage.str());
      }
      else if (commandWord == "register")
      {
          string username;
          string password;
          messageStream >> username >> password;
          Register(cnct, username, password);
      }
      else if (commandWord == "login")
      {
          string username;
          string password;
          messageStream >> username >> password;
          Login(cnct, username, password);
      }
      else if (commandWord == "logout")
      {
          Logout(cnct);
      }
      else if (commandWord == "send")
      {

      }
      else if (commandWord == "getlog")
      {

      }
      else if (commandWord == "getlist")
      {

      }
      else
      {
          sendMessage(cnct, "Command not recognized");
      }
  }
  else
  {
      char buffer[100];
      cout << "Recieved " << message << "\n";
      sprintf_s(buffer, "%s : %s", cnct->user_id.c_str(), message.c_str());

      for (auto& connection : connections) {
          SOCKET sock = connection->sock;
          if (sock) {
              sendString(sock, buffer);
          }
      }
  }
 
  return 1;
}

int Server::Register(shared_ptr<Connection> cnct, string username, string pass) {
    if (cnct->user_id != "") {
        sendMessage(cnct, "You are already logged in.");
        return 0;
    }
    if (username == "" || pass == "")
    {
        sendMessage(cnct, "No username or password was provided.");
        return 0;
    }
    if (users.find(username) != users.end()) {
        sendMessage(cnct, "Username has already already taken.");
        return 0;
    }
    cnct->user_id = username;
    users[username] = make_shared<User>(User(username, pass));
    ostringstream os;
    os << "Welcome " << username << ".\n"
        << "You are now registered.";
    sendMessage(cnct, os.str());
    return 0;
}

int Server::sendMessage(shared_ptr<Connection> cnct, string str)
{
    RETURNCODE code = sendString(cnct->sock, str);
    if (code == SUCCESS)
    {
        return 0;
    }
    else
    {
        cout << "Send failed, failed with code " << code << "\n";
        if (code == SHUTDOWN)
        {
            shutdown(cnct->sock, SD_BOTH);
            cout << "Send failed, shutting down socket\n";
        }
        RemoveConnection(cnct);
        return -1;
    }
}

int Server::recvMessage(shared_ptr<Connection> cnct, string* buffer)
{
    RETURNCODE code = recvString(cnct->sock, buffer);
    if (code == SUCCESS)
    {
        return 0;
    }
    else
    {
        cout << "Recieve failed, failed with code " << code << "\n";
        if (code == SHUTDOWN)
        {
            shutdown(cnct->sock, SD_BOTH);
            cout << "Recieve failed, shutting down socket\n";
        }
        RemoveConnection(cnct);
        return -1;
    }
}

void Server::RemoveConnection(shared_ptr<Connection> cnct)
{
    cnct->Close();
    auto it = find(connections.begin(), connections.end(), cnct);
    if (it != connections.end())
    {
        connections.erase(it);
    }
}

string Server::GetIPAddress(int family, int stream, int protocol)
{
    addrinfo AddrInfo = { 0 , family, stream, protocol, 0 };
    addrinfo* OutAddr;
    //free structure
    int errorcheck = getaddrinfo(hostname, NULL, &AddrInfo, &OutAddr);
    if (errorcheck != 0)
    {
        cout << "Could not gather Address Information.";
        return "";
    }

    char Ip[200];
    Ip[0] = 0;
    while (OutAddr != NULL)
    {
        if (OutAddr->ai_addr != NULL)
        {
            if (family == AF_INET)
            {
                in_addr inAddr = reinterpret_cast<sockaddr_in*>(OutAddr->ai_addr)->sin_addr;
                inet_ntop(OutAddr->ai_family, &inAddr, Ip, 200);
                break;
            }
            else if (family == AF_INET6)
            {
                in_addr6 inAddr = reinterpret_cast<sockaddr_in6*>(OutAddr->ai_addr)->sin6_addr;
                inet_ntop(OutAddr->ai_family, &inAddr, Ip, 200);
                break;
            }
        }
        OutAddr = OutAddr->ai_next;
    }
    return Ip;
}

int Server::Login(shared_ptr<Connection> cnct, string username, string pass)
{
    if (cnct->user_id != "") {
        sendMessage(cnct, "You are already logged in.");
        return 0;
    }
    if (username == "" || pass == "")
    {
        sendMessage(cnct, "No username or password was provided.");
        return 0;
    }
    auto user = users.find(username);
    if (user == users.end()) {
        sendMessage(cnct, "Username has not been found.");
        return 0;
    }
    if ((*user).second->password == pass)
    {
        cnct->user_id = username;
        ostringstream os;
        os << "Welcome " << username << ".\n"
            << "You are now logged back in.";
        sendMessage(cnct, os.str());
    }
    else
    {
        ostringstream os;
        os << "Password is not correct, please try again.";
        sendMessage(cnct, os.str());
    }
    return 0;
}

int Server::Logout(shared_ptr<Connection> cnct)
{
    if (cnct->user_id == "") {
        sendMessage(cnct, "You are not logged in.");
        return 0;
    }
    cnct->user_id = "";
    sendMessage(cnct, "You have been successfully logged out.");
    return 0;

}

int Server::SendDirectMessage(shared_ptr<Connection> cnct, string username, string str)
{
    return 0;
}

int Server::GetLog(shared_ptr<Connection> cnct)
{
    return 0;
}

int Server::GetList(shared_ptr<Connection> cnct)
{
    return 0;
}

void Server::run() {
  if (init() < 0)
    return;

  while (1) {
    fd_set read_fds = fds;
    int select_result = select((int)max_fd + 1, &read_fds, NULL, NULL, NULL);
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
          if (new_socket == INVALID_SOCKET)
          {
              cout << "Accept connection failed \n";
              continue;
          }
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
}

//Connection::~Connection()
//{
//    Close();
//}

void Connection::Close()
{
    if (sock != INVALID_SOCKET)
    {
        if (closesocket(sock) == SOCKET_ERROR)
        {
            SocketError("Error, attempting to close socket");
        }
        sock = INVALID_SOCKET;
    }
}
