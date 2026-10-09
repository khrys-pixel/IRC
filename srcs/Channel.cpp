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

const std::map<int, Client*>	&Channel::getOperators() const {
	return _operators;
}

void	Channel::addMember(Client* client) {
	_members[client->getFd()] = client;
}

void	Channel::addOperator(Client* client) {
	_operators[client->getFd()] = client;
}

void	Channel::removeMember(int fd) {
	_members.erase(fd);
	_operators.erase(fd);
}

void	Channel::removeOperator(int fd) {
	_operators.erase(fd);
}

bool	Channel::isEmpty() const {
	return _members.empty();
}

std::string	Channel::getMemberListString() const {
	std::string list = "";
	for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it) {
		if (!list.empty())
			list += " ";
		if (isOperator(it->second))
			list += '@' + it->second->getNickname();
		else
			list += it->second->getNickname();
	}
	return list;
}

bool	Channel::isChannelMember(Client *client) const {
	return _members.count(client->getFd()) > 0;
}

bool	Channel::isOperator(Client *client) const {
	return _operators.count(client->getFd()) > 0;
}
