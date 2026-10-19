#include "Channel.hpp"

Channel::Channel(const std::string& name)
	: _name(name), _inviteOnly(false), _topicRestriction(true), _userLimit(-1), _passkey("")
{}

const std::string	&Channel::getName() const { 
	return _name;
}

const std::string	&Channel::getTopic() const {
	return _topic;
}

void	Channel::setTopic(const std::string &newTopic) {
	_topic = newTopic;
}

const std::map<int, Client*>	&Channel::getMembers() const {
	return _members;
}

const std::map<int, Client*>	&Channel::getOperators() const {
	return _operators;
}

int	Channel::getUserLimit() {
	return _userLimit;
}

void	Channel::setUserLimit(int limit) {
	_userLimit = limit;
}

void	Channel::addMember(Client* client) {
	_members[client->getFd()] = client;
}

void	Channel::addOperator(Client* client) {
	_operators[client->getFd()] = client;
}

void	Channel::removeMember(int fd) {
	_members.erase(fd);
	if (_operators.count(fd) > 0)
		_operators.erase(fd);
}

void	Channel::removeOperator(int fd) {
	_operators.erase(fd);
}

bool	Channel::isEmpty() const {
	return _members.empty();
}

bool	Channel::isInviteOnly() {
	return _inviteOnly;
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

bool	Channel::isRestrictedTopic() {
	return _topicRestriction;
}

void	Channel::setTopicRestriction(bool status) {
	_topicRestriction = status;
}

void	Channel::setInviteOnly(bool status) {
	_inviteOnly = status;
}

bool	Channel::canBeJoined(Client *client, std::string *msg, const std::string &key) {
	if ((_userLimit != -1 && _members.size() == static_cast<long unsigned int>(_userLimit)) || _userLimit == 0) {
		*msg = ERR_CHANNELISFULL_CODE + _name + ERR_CHANNELISFULL_MSG;
		return false;
	} else if (_inviteOnly == true && _invites.count(client->getFd()) < 1) {
		*msg = ERR_INVITEONLYCHAN_CODE + _name + ERR_CHANNELISFULL_MSG;
		return false;
	} else if (!_passkey.empty() && key != _passkey) {
		*msg = ERR_BADCHANNELKEY_CODE + _name + ERR_BADCHANNELKEY_MSG;
		return false;
	}
	return true;
}

void	Channel::addInvite(Client *client) {
	_invites[client->getFd()] = client;
}

void	Channel::setKey(const std::string &newKey) {
	_passkey = newKey;
}

std::string	&Channel::getKey() {
	return _passkey;
}