#include "IrcServer.hpp"

void IrcServer::initCommands() {
	_commandMap["PASS"] = &IrcServer::handlePass;
	_commandMap["NICK"] = &IrcServer::handleNick;
	_commandMap["USER"] = &IrcServer::handleUser;
	_commandMap["PING"] = &IrcServer::handlePing;
	_commandMap["JOIN"] = &IrcServer::handleJoin;
	_commandMap["PRIVMSG"] = &IrcServer::handlePrivmsg;
	_commandMap["MODE"] = &IrcServer::handleMode;
	_commandMap["KICK"] = &IrcServer::handleKick;
	_commandMap["INVITE"] = &IrcServer::handleInvite;
	_commandMap["TOPIC"] = &IrcServer::handleTopic;
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
		sendToClientBuffer(client, std::string(ERR_NEEDMOREPARAMS_CODE) + "* USER" + ERR_NEEDMOREPARAMS_MSG);
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

// Syntax: JOIN #channel
void IrcServer::handleJoin(Client* client, const std::vector<std::string>& params) {
	if (params.empty()) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " JOIN" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}

	std::string chanName = params[0];
	if (chanName.empty() || chanName[0] != '#') {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_CODE + client->getNickname() + " " + chanName + ERR_NOSUCHCHANNEL_MSG);
		return;
	}

	bool	isFirst = false;
	// Retrieve or create the channel
	if (_channels.find(chanName) == _channels.end()) {
		_channels[chanName] = new Channel(chanName);
		isFirst = true;
	}
	Channel* chan = _channels[chanName];
	// Check if user can join channel
	// pass key as param
	std::string	msg = "";
	std::string	optionalKey = (params.size() > 1) ? ((params[1].empty()) ? std::string() : params[1]) : std::string();
	if (!chan->canBeJoined(client, &msg, optionalKey)) {
		sendToClientBuffer(client, msg);
		return ;
	}
	chan->addMember(client);
	if (isFirst)
		chan->addOperator(client);

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

// Syntax: PRIVMSG nickname :msg
void	IrcServer::handlePrivmsg(Client* client, const std::vector<std::string>& params) {
	Client  *reciever = NULL;

	if (params.size() < 2) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " PRIVMSG" + ERR_NEEDMOREPARAMS_MSG);
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
	formatAndSend(reciever, client, NULL, msg, "PRIVMSG");
}

// Syntax: PRIVMSG #channel :msg
void	IrcServer::handleChannelMsg(Client* client, const std::vector<std::string>& params) {
	std::string	channelName = params[0];
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
	if (it == _channels.end()) {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_CODE + client->getNickname() + " " + channelName + ERR_NOSUCHCHANNEL_MSG);
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
	formatAndSend(NULL, client, channel, msg, "PRIVMSG");
}

bool	IrcServer::getInfo(
	Client *client, Channel **channel, Client **target, const std::string &cName, const std::string &tName, bool needOp, bool tMember, bool needTarget
) {
	*channel = NULL;
	*target = NULL;

	std::map<std::string, Channel*>::iterator it = _channels.find(cName);
	if (it == _channels.end()) {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_CODE + client->getNickname() + " " + cName + ERR_NOSUCHCHANNEL_MSG);
		return false;
	}
	Channel	*chan = it->second;
	if (needOp || chan->isInviteOnly()) {
		if (!chan->isOperator(client)) {
			sendToClientBuffer(client, ERR_CHANOPRIVSNEEDED_CODE + client->getNickname() + " " + chan->getName() + ERR_CHANOPRIVSNEEDED_MSG);
			return false;
		}
	}

	Client	*clientTarget = findClientByNick(tName);
	if (!clientTarget && needTarget) {
		sendToClientBuffer(client, ERR_NOSUCHNICK_CODE + client->getNickname() + " " + tName + ERR_NOSUCHNICK_MSG);
		return false;
	}
	if (tMember && needTarget) {
		if (!chan->isChannelMember(clientTarget)) {
			sendToClientBuffer(client, ERR_USERNOTINCHANNEL_CODE + client->getNickname() + " " + clientTarget->getNickname()
								+ " " + chan->getName() + ERR_USERNOTINCHANNEL_MSG);
			return false;
		}
	}

	*channel = chan;
	*target = clientTarget;
	return true;
}

// Syntax: MODE #channel|nickname [[+|-]modechars [parameters]]
void	IrcServer::handleMode(Client* client, const std::vector<std::string>& params) {
	if (params.size() < 2) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " MODE" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}
	
	std::string	op = params[1];
	if (op.size() < 2) {
		sendToClientBuffer(client, ERR_UMODEUNKNOWNFLAG_CODE + op + ERR_UMODEUNKNOWNFLAG_MSG);
		return;
	}
	char		opSign = op[0];
	char		opFlag = op[1];

	Channel* channel;
	Client	*clientTarget;
	if (!getInfo(client, &channel, &clientTarget, params[0], params[2], true, true, opFlag == 'o'))
		return ;

	std::string	msgTarget = " " + op + " " + (clientTarget ? clientTarget->getNickname() : "");
	Channel::t_modeEntry	*modeCmds = Channel::getModeCmds();
	for (int i = 0; modeCmds[i].funct; i++) {
		if (modeCmds[i].c == opFlag) {
			t_modeCmd	cmd = {opSign, opFlag};
			(channel->*modeCmds[i].funct)(*this, client, cmd, params);
			formatAndSend(NULL, client, channel, msgTarget, "MODE");
			return ;
		}
	}
	sendToClientBuffer(client, ERR_UMODEUNKNOWNFLAG_CODE + params[1] + ERR_UMODEUNKNOWNFLAG_MSG);
}

// Syntax: KICK #channel nickname [:comment]
void	IrcServer::handleKick(Client* client, const std::vector<std::string>& params) {
	if (params.size() < 2) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " KICK" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}

	Channel* channel;
	Client	*clientTarget;

	if (!getInfo(client, &channel, &clientTarget, params[0], params[1], true, true, true))
		return ;

	std::string	comment = client->getNickname();
	if (params.size() > 2 && !params[2].empty())
		comment = params[2];

	std::string	msg = " " + clientTarget->getNickname() + " :" + comment;
	formatAndSend(NULL, client, channel, msg, "KICK");
	channel->removeMember(clientTarget->getFd());
}

// Syntax: INVITE nickname #channel
void	IrcServer::handleInvite(Client* client, const std::vector<std::string>& params) {
	if (params.size() < 2) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " INVITE" + ERR_NEEDMOREPARAMS_MSG);
		return;
	}

	Channel	*channel = NULL;
	Client	*targetClient = NULL;
	if (!getInfo(client, &channel, &targetClient, params[1], params[0], false, false, true))
		return ;

	channel->addInvite(targetClient);
	sendToClientBuffer(client, RPL_INVITING_CODE + client->getNickname() + " " + targetClient->getNickname() + " " + channel->getName());
	std::string	msg = " :" + channel->getName();
	formatAndSend(targetClient, client, NULL, msg, " INVITE");
}

// Syntax: TOPIC #channel [newtopic]
void	IrcServer::handleTopic(Client* client, const std::vector<std::string>& params) {
	if (params.size() < 1) {
		sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " TOPIC" + ERR_NEEDMOREPARAMS_MSG);
		return ;
	}

	std::map<std::string, Channel*>::iterator it = _channels.find(params[0]);
	if (it == _channels.end()) {
		sendToClientBuffer(client, ERR_NOSUCHCHANNEL_CODE + client->getNickname() + " " + params[0] + ERR_NOSUCHCHANNEL_MSG);
		return ;
	}
	Channel	*channel = it->second;

	if (params.size() == 1) {
		std::string	topic = channel->getTopic();
		if (topic.empty()) {
			sendToClientBuffer(client, RPL_NOTOPIC_CODE + channel->getName() + RPL_NOTOPIC_MSG);
			return ;
		}
		sendToClientBuffer(client, RPL_TOPIC_CODE + channel->getName() + " :" + topic);
		return ;
	}

	if (channel->isRestrictedTopic() && !channel->isOperator(client)) {
		sendToClientBuffer(client, ERR_CHANOPRIVSNEEDED_CODE + client->getNickname() + " " + channel->getName() + ERR_CHANOPRIVSNEEDED_MSG);
		return ;
	}
	channel->setTopic(params[1]);
	formatAndSend(NULL, client, channel, " :" + params[1], "TOPIC");
}