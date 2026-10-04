#include "Client.hpp"
#include <iostream>

Client::Client(int fd, const std::string& ip, int port) 
    : _fd(fd), 
      _ip(ip), 
      _port(port), 
      _isRegistered(false), 
      _isOperator(false), 
      _hasGivenPassword(false) {}

Client::~Client() {}

void Client::addToInputBuffer(const std::string& data) {
    _inputBuffer += data;
}

// Function to handle command aggregation as required by the subject
bool Client::getNextCommand(std::string& commandOut) {
    // IRC commands terminate with \r\n
    size_t rnPos = _inputBuffer.find("\r\n");
    if (rnPos != std::string::npos) {
        // Complete command obtained
        commandOut = _inputBuffer.substr(0, rnPos);
        
        // Remove processed command and the \r\n terminator from input buffer
        _inputBuffer.erase(0, rnPos + 2);
        return true; 
    }
    // Only partial command received so far, return false to wait for more data
    return false;
}

void Client::addToOutputBuffer(const std::string& message) {
    // Ensure all outgoing messages use the correct \r\n terminator
    if (!message.empty() && (message.length() < 2 || message.substr(message.length() - 2) != "\r\n")) {
        _outputBuffer += (message + "\r\n");
    } else {
        _outputBuffer += message;
    }
}

const std::string& Client::getOutputBuffer() const {
    return _outputBuffer;
}

void Client::removeFromOutputBuffer(int bytesSent) {
    if (bytesSent > 0) {
        _outputBuffer.erase(0, bytesSent);
    }
}

// Getters and Setters
int Client::getFd() const { return _fd; }
const std::string& Client::getIp() const { return _ip; }
int Client::getPort() const { return _port; }

bool Client::hasGivenPassword() const { return _hasGivenPassword; }
void Client::setHasGivenPassword(bool status) { _hasGivenPassword = status; }

bool Client::isRegistered() const { return _isRegistered; }
void Client::setRegistered(bool status) { _isRegistered = status; }

bool Client::isOperator() const { return _isOperator; }
void Client::setOperator(bool status) { _isOperator = status; }

const std::string& Client::getNickname() const { return _nickname; }
void Client::setNickname(const std::string& nick) { _nickname = nick; }

const std::string& Client::getUsername() const { return _username; }
void Client::setUsername(const std::string& user) { _username = user; }

const std::string& Client::getRealname() const { return _realname; }
void Client::setRealname(const std::string& real) { _realname = real; }
