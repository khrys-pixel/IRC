#include "IrcServer.hpp"

void IrcServer::initCommands() {
	_commandMap["PASS"] = &IrcServer::handlePass;
	_commandMap["NICK"] = &IrcServer::handleNick;
	_commandMap["USER"] = &IrcServer::handleUser;
	_commandMap["PING"] = &IrcServer::handlePing;
	_commandMap["JOIN"] = &IrcServer::handleJoin;
	_commandMap["PRIVMSG"] = &IrcServer::handlePrivmsg;
}

void IrcServer::checkAndRegister(Client* client) {
	if (!client->isRegistered() && client->hasGivenPassword() && 
		!client->getNickname().empty() && !client->getUsername().empty()) {
		client->setRegistered(true);
		std::string nick = client->getNickname();

		// Standard RFC 1459 registration sequence
		sendToClientBuffer(client, ":ircserv 001 " + nick + " :Welcome to the ft_irc Network " + nick);
		sendToClientBuffer(client, ":ircserv 002 " + nick + " :Your host is ircserv, running version 1.0");
		sendToClientBuffer(client, ":ircserv 003 " + nick + " :This server was created today");
		sendToClientBuffer(client, ":ircserv 004 " + nick + " ircserv 1.0 io itkol");
	}
}

void	IrcServer::handlePass(Client* client, const std::vector<std::string>& params) {
	if (client->isRegistered()) {
		sendToClientBuffer(client, ":ircserv 462 * :You may not reregister");
		return;
	}
	if (params.empty()) {
		sendToClientBuffer(client, ":ircserv 461 * PASS :Not enough parameters");
		return;
	}
	if (params[0] == _password) {
		client->setHasGivenPassword(true);
	} else {
		sendToClientBuffer(client, ":ircserv 464 * :Password incorrect");
	}
}

void	IrcServer::handleNick(Client* client, const std::vector<std::string>& params) {
	if (params.empty()) {
		sendToClientBuffer(client, ":ircserv 431 * :No nickname given");
		return;
	}
	client->setNickname(params[0]);
	checkAndRegister(client);
}

void	IrcServer::handleUser(Client* client, const std::vector<std::string>& params) {
	if (params.size() < 4) {
		sendToClientBuffer(client, ":ircserv 461 * USER :Not enough parameters");
		return;
	}
	client->setUsername(params[0]);
	client->setRealname(params[3]);
	checkAndRegister(client);
}

void	IrcServer::handlePing(Client* client, const std::vector<std::string>& params) {
	std::string target = params.empty() ? "ircserv" : params[0];
	sendToClientBuffer(client, ":ircserv PONG ircserv :" + target);
}

void IrcServer::handleJoin(Client* client, const std::vector<std::string>& params) {
	if (params.empty()) {
		sendToClientBuffer(client, ":ircserv 461 " + client->getNickname() + " JOIN :Not enough parameters");
		return;
	}

	std::string chanName = params[0];
	if (chanName.empty() || chanName[0] != '#') {
		sendToClientBuffer(client, ":ircserv 403 " + client->getNickname() + " " + chanName + " :No such channel");
		return;
	}

	// Retrieve or create the channel
	if (_channels.find(chanName) == _channels.end()) {
		_channels[chanName] = new Channel(chanName);
	}
	Channel* chan = _channels[chanName];
	chan->addMember(client);

	// Broadcast JOIN notification to channel members
	std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@127.0.0.1 JOIN " + chanName;
	chan->broadcast(joinMsg, NULL);

	// Send Channel Topic (331 or 332)
	if (chan->getTopic().empty()) {
		sendToClientBuffer(client, ":ircserv 331 " + client->getNickname() + " " + chanName + " :No topic is set");
	} else {
		sendToClientBuffer(client, ":ircserv 332 " + client->getNickname() + " " + chanName + " :" + chan->getTopic());
	}

	// Send Member List (353) & End of /NAMES (366)
	sendToClientBuffer(client, ":ircserv 353 " + client->getNickname() + " = " + chanName + " :" + chan->getMemberListString());
	sendToClientBuffer(client, ":ircserv 366 " + client->getNickname() + " " + chanName + " :End of /NAMES list");

}

void	IrcServer::formatAndSend(Client *reciever, Client *sender, Channel *channel, const std::string &msg) {
	std::string prefix = ":" + sender->getNickname();
	std::string	target = (reciever) ? reciever->getNickname() : channel->getName();
	std::string	fullMsg = prefix + " PRIVMSG " + target + msg;
	if (reciever)
		sendToClientBuffer(reciever, fullMsg);
	else {
		const std::map<int, Client*>& channelMembers = channel->getMembers();
		for (std::map<int, Client*>::const_iterator m = channelMembers.begin(); m != channelMembers.end(); ++m) {
			if (m->second != sender)
				sendToClientBuffer(m->second, fullMsg);
		}
	}
}

void	IrcServer::handlePrivmsg(Client* client, const std::vector<std::string>& params) {
	Client  *reciever = NULL;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second->getNickname() == params[0]) {
			reciever = it->second;
			break;
		}
	}
	if (!reciever) {
		if (params[0][0] == '#' && params[0][1])
			handleChannelMsg(client, params);
		return ;
	}

	std::vector<std::string> args(params.begin() + 1, params.end());
	std::string	msg = " ";
	for (std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); ++i) {
		if (i != args.begin())
			msg += " ";
		msg += *i;
	}
	formatAndSend(reciever, client, NULL, msg);
}

void	IrcServer::handleChannelMsg(Client* client, const std::vector<std::string>& params) {
	std::string	channelName = params[0];
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
	if (it == _channels.end()) {
		sendToClientBuffer(client, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel");
		return;
	}

	Channel* channel = it->second;
	std::vector<std::string> args(params.begin() + 1, params.end());
	std::string	msg = " ";
	for (std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); ++i) {
		if (i != args.begin())
			msg += " ";
		msg += *i;
	}
	formatAndSend(NULL, client, channel, msg);
}
