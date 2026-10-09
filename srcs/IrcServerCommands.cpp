#include "IrcServer.hpp"

void IrcServer::initCommands() {
	_commandMap["PASS"] = &IrcServer::handlePass;
	_commandMap["NICK"] = &IrcServer::handleNick;
	_commandMap["USER"] = &IrcServer::handleUser;
	_commandMap["PING"] = &IrcServer::handlePing;
	_commandMap["JOIN"] = &IrcServer::handleJoin;
	_commandMap["PRIVMSG"] = &IrcServer::handlePrivmsg;
	_commandMap["MODE"] = &IrcServer::handleMode;
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
		sendToClientBuffer(client, std::string(ERR_NEEDMOREPARAMS_VAL) + "* USER" + ERR_NEEDMOREPARAMS_MSG);
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
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_VAL + client->getNickname() + " JOIN" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}

	std::string chanName = params[0];
	if (chanName.empty() || chanName[0] != '#') {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_VAL + client->getNickname() + " " + chanName + ERR_NOSUCHCHANNEL_MSG);
		return;
	}

	bool	isFirst = false;
	// Retrieve or create the channel
	if (_channels.find(chanName) == _channels.end()) {
		_channels[chanName] = new Channel(chanName);
		isFirst = true;
	}
	Channel* chan = _channels[chanName];
	chan->addMember(client);
	if (isFirst)
		makeOperator(chan, client, true);

	// Broadcast JOIN notification to channel members
	std::string joinMsg = ":" + client->getHostmask() + " JOIN " + chanName;
	const std::map<int, Client*>& members = chan->getMembers();
	for (std::map<int, Client*>::const_iterator m = members.begin(); m != members.end(); ++m)
		sendToClientBuffer(m->second, joinMsg);

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

void	IrcServer::handlePrivmsg(Client* client, const std::vector<std::string>& params) {
	Client  *reciever = NULL;

	if (params.size() < 2) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_VAL + client->getNickname() + " PRIVMSG" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}
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
	formatAndSend(reciever, client, NULL, msg, MSG);
}

void	IrcServer::handleChannelMsg(Client* client, const std::vector<std::string>& params) {
	std::string	channelName = params[0];
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
	if (it == _channels.end()) {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_VAL + client->getNickname() + " " + channelName + ERR_NOSUCHCHANNEL_MSG);
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
	formatAndSend(NULL, client, channel, msg, MSG);
}

void	IrcServer::handleMode(Client* client, const std::vector<std::string>& params) {
	if (params.size() < 3) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_VAL + client->getNickname() + " MODE" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}
	std::string	channelName = params[0];
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
	if (it == _channels.end()) {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_VAL + client->getNickname() + " " + channelName + ERR_NOSUCHCHANNEL_MSG);
		return;
	}
	Channel* channel = it->second;
	if (!channel->isOperator(client)) {
		std::string	errMsg = ERR_CHANOPRIVSNEEDED_VAL + client->getNickname() + " " + channel->getName() + ERR_CHANOPRIVSNEEDED_MSG;
		sendToClientBuffer(client, errMsg);
		return ;
	}

	Client	*clientTarget = findClientByNick(params[2]);
	if (!clientTarget) {
		std::string	errMsg = ERR_NOSUCHNICK_VAL + client->getNickname() + " " + channel->getName() + ERR_NOSUCHNICK_VAL;
		sendToClientBuffer(client, errMsg);
		return ;
	}
	if (!channel->isChannelMember(clientTarget)) {
		std::string	errMsg = ERR_USERNOTINCHANNEL_VAL + client->getNickname() + " " + clientTarget->getNickname() 
							+ " " + channel->getName() + ERR_USERNOTINCHANNEL_MSG;
		sendToClientBuffer(client, errMsg);
		return ;
	}
	std::string	op = params[1];
	std::string	msg = " " + op + " " + clientTarget->getNickname();
	if (op == "+o") {
		makeOperator(channel, clientTarget, true);
	} else if (op == "-o") {
		makeOperator(channel, clientTarget, false);
	} else {
		sendToClientBuffer(client, ERR_UMODEUNKNOWNFLAG_VAL + op + ERR_UMODEUNKNOWNFLAG_MSG);
		return ;
	}
	formatAndSend(NULL, client, channel, msg, MODE);
}
