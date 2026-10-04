#include "IrcServer.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio> // For perror
#include <errno.h>

IrcServer::IrcServer(int port, const std::string& password)
    : _port(port), _password(password), _listenSocketFd(-1) {
    initSocket();
}

IrcServer::~IrcServer() {
    std::cout << "IRC Server shutting down..." << std::endl;
    for (size_t i = 0; i < _pollFds.size(); ++i) {
        close(_pollFds[i].fd);
    }
    // and clean up _clients map ...
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
    _clients.clear();
}

void IrcServer::initSocket() {
    _listenSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSocketFd == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(_listenSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(_listenSocketFd);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    // --- NON-BLOCKING IS REQUIRED ---
    setSocketNonBlocking(_listenSocketFd);

    memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on any interface
    _serverAddr.sin_port = htons(_port);

    if (bind(_listenSocketFd, (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) == -1) {
        perror("bind");
        close(_listenSocketFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(_listenSocketFd, SOMAXCONN) == -1) {
        perror("listen");
        close(_listenSocketFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    // 7. Initialize pollFds with the listening socket
    struct pollfd listenPollFd;
    listenPollFd.fd = _listenSocketFd;
    listenPollFd.events = POLLIN; // Wait for read
    listenPollFd.revents = 0;
    _pollFds.push_back(listenPollFd);
}

void IrcServer::setSocketNonBlocking(int fd) {
    // Correct O_NONBLOCK setup, essential for single-threaded operation
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl O_NONBLOCK");
        throw std::runtime_error("Failed to set socket to non-blocking");
    }
}

void IrcServer::run() {
    std::cout << "IRC Server started on port " << _port << "..." << std::endl;

    while (true) {
        int pollCount = poll(_pollFds.data(), _pollFds.size(), -1);

        if (pollCount == -1) {
            perror("poll");
            continue;
        }

        if (pollCount == 0) {
            continue;
        }

        // --- Core multiplexing logic with single poll() ---
        if (_pollFds[0].revents & POLLIN) {
            acceptNewConnection();
            pollCount--; // prioritize accept but continue with client IO.
        }

        // starting from index 1 (clients)
        for (size_t i = 1; i < _pollFds.size() && pollCount > 0; ++i) {
            if (_pollFds[i].revents == 0) {
                continue; 
            }

            handleClientActivity(_pollFds[i].fd, i);
            pollCount--; 
        }
    }
}

void IrcServer::acceptNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientFd = accept(_listenSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientFd == -1) {
        perror("accept");
        return; 
    }

    try {
        setSocketNonBlocking(clientFd);
    } catch (const std::exception& e) {
        close(clientFd);
        std::cerr << "Failed to make new connection non-blocking: " << e.what() << std::endl;
        return;
    }

    std::cout << "New connection accepted (fd: " << clientFd << ", ip: " 
              << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << ")" << std::endl;

    // --- Client object creation now required ---
    _clients[clientFd] = new Client(clientFd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    // Add to pollFds
    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN; // Wait for read
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);
}


void IrcServer::handleClientActivity(int fd, int index) {
    Client* client = _clients[fd];
    if (!client) return; 

    // --- Input Buffering (POLLIN) ---
    if (_pollFds[index].revents & POLLIN) {
        char buffer[2048];
        int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            
            // 1. Buffer the raw data received within the Client object
            client->addToInputBuffer(buffer);

            // 2. Aggregate packets and extract complete IRC commands (\r\n)
            std::string commandString;
            while (client->getNextCommand(commandString)) {
                // Buffer management successful, ONE complete command obtained
                parseAndExecute(client, commandString);
            }
        } else {
            if (bytesRead == 0) {
                std::cout << "Client (fd: " << fd << ") disconnected gracefully." << std::endl;
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Common with non-blocking sockets (no data ready)
                std::cerr << "POLLIN with no data (fd:" << fd << ")" << std::endl;
            } else {
                // Actual receive error
                perror("recv error");
            }
            disconnectClient(fd, index);
            return; // Client gone, exit loop
        }
    }

    // --- Output Buffering (POLLOUT) ---
    if (_pollFds[index].revents & POLLOUT) {
        const std::string& outputBuffer = client->getOutputBuffer();
        if (!outputBuffer.empty()) {
            int bytesSent = send(fd, outputBuffer.c_str(), outputBuffer.length(), 0);

            if (bytesSent > 0) {
                // Remove sent bytes from the buffer
                client->removeFromOutputBuffer(bytesSent);
            } else {
                if (bytesSent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // Send buffer is full, cannot write right now; POLLOUT will fire again
                } else {
                    perror("send error");
                    disconnectClient(fd, index);
                    return; // exit early
                }
            }
        }

        // --- Crucial optimization: If the buffer is now empty, STOP waiting for POLLOUT ---
        if (outputBuffer.empty()) {
            _pollFds[index].events &= ~POLLOUT;
        }
    }

    // Check for POLLERR or POLLHUP, which also require disconnecting.
    if (_pollFds[index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        if (_pollFds[index].revents & POLLNVAL) std::cerr << "POLLNVAL (fd:" << fd << ")" << std::endl;
        else if (_pollFds[index].revents & POLLERR) perror("POLLERR (socket broken)");
        else std::cout << "Client (fd: " << fd << ") hung up." << std::endl;
        
        disconnectClient(fd, index);
    }
}

// **CORRECTED**: Scope added
// Helper function to queue a message for a client and request POLLOUT
void IrcServer::sendToClientBuffer(Client* client, const std::string& message) {
    if (!client) return;

    // Buffer the message within the Client object
    client->addToOutputBuffer(message);

    // Find the client FD in pollFds and request to monitor POLLOUT
    int clientFd = client->getFd();
    for (size_t i = 1; i < _pollFds.size(); ++i) {
        if (_pollFds[i].fd == clientFd) {
            _pollFds[i].events |= POLLOUT;
            break;
        }
    }
}

void IrcServer::disconnectClient(int fd, int index) {
    close(fd);
    _pollFds.erase(_pollFds.begin() + index);

    // clean up Client object
    delete _clients[fd];
    _clients.erase(fd);
}
