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
#include <thread>
#include <chrono>

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
  publicFile.open("Convo-Log.txt", fstream::in | fstream::out | fstream::app | fstream::ate);
  cmdFile.open("Command-Log.txt", fstream::in | fstream::out | fstream::app | fstream::ate);
  cout << "Making sock on port " << port << endl;
  cout << "How many Users are looking to chat: ";
  cin >> maxConnections;
  sock = makeServerSocket(port);
  if (sock < 0)
    return -1;
  std::cout << "Socket bound and listening...\n";
  ostringstream os;
  os << hostname << " : " << GetIPAddress(AF_INET, SOCK_STREAM, IPPROTO_TCP) << " : " << GetIPAddress(AF_INET6, SOCK_STREAM, IPPROTO_TCP) << " : " << port << "\n";
  new thread(Broadcast, os.str(), port);
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
      ProcessCommand(message, cnct);
  }
  else
  {
      char buffer[100];
      cout << "Recieved " << message << "\n";
      if (cnct->user_id == "")
      {
          sendMessage(cnct, "You are not logged in, please /register");
          return 0;
      }
      sprintf_s(buffer, "%s : %s", cnct->user_id.c_str(), message.c_str());

      for (auto& connection : connections) {
          SOCKET sock = connection->sock;
          if (sock) {
              if (connection == cnct)
              {
                  continue;
              }
              sendString(sock, buffer);
          }
      }
      ostringstream os;
      os << cnct->user_id << ": " << message << "\n";
      publicFile.write(os.str().c_str(), os.str().length());
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
    string line;
    istringstream is(str);
    while (!is.eof())
    {
        getline(is, line);
        RETURNCODE code = sendString(cnct->sock, line);
        if (code != SUCCESS)
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
    return 0;    
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
    Disconnect(cnct);
    return 0;
}

int Server::SendDirectMessage(shared_ptr<Connection> cnct, string username, string str)
{
    ostringstream os;
    os << cnct->user_id << ":" << str;
    if (cnct->user_id != "")
    {
        for (int i = 0; i < connections.size(); i++)
        {
            if (connections.at(i).get()->user_id == username)
            {
                sendMessage(connections.at(i), os.str());
                return 0;
            }
        }
    }
    else
    {
        cout << "User is not logged in\n";
        sendMessage(cnct, "You need to be logged in to user this function");
        return 0;
    }
    sendMessage(cnct, "User not currently logged in");
    return 0;
}

int Server::GetLog(shared_ptr<Connection> cnct, string fileType)
{
    transform(fileType.begin(), fileType.end(), fileType.begin(), __ascii_tolower);
    if (fileType == "public")
    {
        ifstream::pos_type fileSize = publicFile.tellg();
        if (fileSize == ifstream::pos_type(-1))
        {
            return -1;
        }
        publicFile.seekg(0, ios::beg);

        vector<char> bytes(fileSize);
        publicFile.read(&bytes[0], fileSize);

        sendMessage(cnct, string(&bytes[0], fileSize));
    }
    else if (fileType == "private")
    {
        ifstream::pos_type fileSize = cmdFile.tellg();
        if (fileSize == ifstream::pos_type(-1))
        {
            return -1;
        }
        cmdFile.seekg(0, ios::beg);

        vector<char> bytes(fileSize);
        cmdFile.read(&bytes[0], fileSize);

        sendMessage(cnct, string(&bytes[0], fileSize));
    }
    return 0;
}

int Server::GetList(shared_ptr<Connection> cnct)
{
    ostringstream os;
    os << "Current Online users:\n";
    for (int i = 0; i < connections.size(); i++)
        os << "\t" << (connections.at(i)).get()->user_id << "\n";
    sendMessage(cnct, os.str());
    return 0;
}



int Server::ProcessCommand(string message, shared_ptr<Connection> cnct)
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
            << commandChar << "help to find all available commands\n\t"
            << commandChar << "login to get back into having your converstations you were previously\n\t"
            << commandChar << "logout to logout of the server\n\t"
            << commandChar << "send <username> <the message to send> to direct message someone without having the other people on the server know what are messaging\n\t"
            << commandChar << "getlog <public or private> to grab the messaging log of the client\n\t"
            << commandChar << "getlist to get a list of users who are currently connected\n";
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
        string username;
        string str;
        messageStream >> username;
        getline(messageStream, str, '\0');
        SendDirectMessage(cnct, username, str);
    }
    else if (commandWord == "getlog")
    {
        string fileType;
        messageStream >> fileType;
        GetLog(cnct, fileType);
    }
    else if (commandWord == "getlist")
    {
        GetList(cnct);
    }
    else
    {
        sendMessage(cnct, "Command not recognized");
    }
    ostringstream finalMessage(message);
    finalMessage << message << "\n";
    cmdFile.write(finalMessage.str().c_str(), finalMessage.str().length());
    return 0;
}
int Server::Broadcast(string str, int port)
{
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == SOCKET_ERROR)
    {
        cout << WSAGetLastError() << "Broadcast : Failed socket function" << "\n";
        return -1;
    }
    int optVal = 1;
    int error = setsockopt(s, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&optVal), sizeof(optVal));
    if (error != 0)
    {
        cout << WSAGetLastError() << "Broadcast : Failed setsockopt function" << "\n";
        return -1;
    }
    sockaddr_in bcAddr;
    bcAddr.sin_family = AF_INET;
    bcAddr.sin_addr.S_un.S_addr = INADDR_BROADCAST;
    bcAddr.sin_port = htons(port);
    while (true)
    {
        error = sendto(s, str.c_str(), str.length(), 0, reinterpret_cast<sockaddr*>(&bcAddr), sizeof(bcAddr));
        if (error < 0)
        {
            cout << WSAGetLastError() << "Broadcast : Failed on sendto function" << "\n";
            return -1;
        }
        this_thread::sleep_for(chrono::seconds(5));
    }
    return 0;
}

void Server::run() {
  if (init() < 0)
    return;

  try
  {
      while (1) {
          fd_set read_fds = fds;
          int select_result = select((int)max_fd + 1, &read_fds, NULL, NULL, NULL);
          if (select_result < 0) {
              cout << "Error select failed";
              return;
          }
          else if (select_result == 0) {
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
                  }
                  else {
                      if (processMessage(i) < 0)
                          return;
                  }
              }
          }
      }
  }
  catch (const std::exception& except)
  {
      publicFile.close();
      cmdFile.close();
      remove("Command-Log.txt");
      remove("Convo-Log.txt");
      cout << "Logs removed for clean-up due to exception " << except.what() << "\n";
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

int Server::Disconnect(shared_ptr<Connection> cnct)
{
    FD_CLR(cnct->sock, &fds);
    shutdown(cnct->sock, SD_BOTH);
    cnct->Close();
    connections.erase(find(connections.begin(), connections.end(), cnct));
    return 0;
}