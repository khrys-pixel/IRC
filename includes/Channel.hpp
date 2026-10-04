#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include "Client.hpp"

class Channel {
private:
    std::string             _name;
    std::string             _topic;
    std::map<int, Client*>  _members;
    std::map<int, Client*>  _operators;

public:
    Channel(const std::string& name) : _name(name) {}

    const std::string& getName() const { return _name; }
    const std::string& getTopic() const { return _topic; }
    const std::map<int, Client*>& getMembers() const { return _members; }

    void addMember(Client* client) { _members[client->getFd()] = client; }
    void removeMember(int fd) { _members.erase(fd); _operators.erase(fd); }
    bool isEmpty() const { return _members.empty(); }

    void broadcast(const std::string& message, Client* sender = NULL) {
        for (std::map<int, Client*>::iterator it = _members.begin(); it != _members.end(); ++it) {
            if (sender == NULL || it->second->getFd() != sender->getFd()) {
                it->second->addToOutputBuffer(message);
            }
        }
    }

    std::string getMemberListString() const {
        std::string list = "";
        for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it) {
            if (!list.empty()) list += " ";
            list += it->second->getNickname();
        }
        return list;
    }
};

#endif
