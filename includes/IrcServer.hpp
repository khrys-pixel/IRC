#ifndef IRCSERVER_HPP
#define IRCSERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "Channel.hpp"
#include "Client.hpp"

class IrcServer {
private:
    int                         _port;
    std::string                 _password;
    int                         _listenSocketFd;
    struct sockaddr_in          _serverAddr;
    
    std::vector<struct pollfd>  _pollFds;
    std::map<std::string, Channel*> _channels;
    std::map<int, Client*>      _clients;

    // Internal initialization & socket helpers
    void initSocket();
    void setSocketNonBlocking(int fd);

    // Event handlers
    void acceptNewConnection();
    void handleClientActivity(int fd, int index);
    void handleClientWrite(int fd);
    void disconnectClient(int fd, int index);
    void handleJoin(Client* client, const std::vector<std::string>& params);
    void sendToClientBuffer(Client* client, const std::string& message);

public:
    IrcServer(int port, const std::string& password);
    ~IrcServer();

    void run();
    void parseAndExecute(Client* client, const std::string& commandString);
    void checkAndRegister(Client* client);
};

#endif
