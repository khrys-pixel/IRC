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
    Channel(const std::string& name);

    const std::string& getName() const;
    const std::string& getTopic() const;
    const std::map<int, Client*>& getMembers() const;

    void addMember(Client* client);
    void removeMember(int fd);
    bool isEmpty() const;
    void broadcast(const std::string& message, Client* sender);

    std::string getMemberListString() const;
};

#endif
