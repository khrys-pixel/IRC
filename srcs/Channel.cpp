#include "Channel.hpp"

Channel::Channel(const std::string& name)
	: _name(name)
{}

const std::string	&Channel::getName() const { 
	return _name;
}

const std::string	&Channel::getTopic() const {
	return _topic;
}

const std::map<int, Client*>	&Channel::getMembers() const {
	return _members;
}

void	Channel::addMember(Client* client) {
	_members[client->getFd()] = client;
}

void	Channel::removeMember(int fd) {
	_members.erase(fd); _operators.erase(fd);
}

bool	Channel::isEmpty() const {
	return _members.empty();
}

void	Channel::broadcast(const std::string& message, Client* sender = NULL) {
	for (std::map<int, Client*>::iterator it = _members.begin(); it != _members.end(); ++it) {
		if (sender == NULL || it->second->getFd() != sender->getFd()) {
			it->second->addToOutputBuffer(message);
		}
	}
}

std::string	Channel::getMemberListString() const {
	std::string list = "";
	for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it) {
		if (!list.empty()) list += " ";
		list += it->second->getNickname();
	}
	return list;
}