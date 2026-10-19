#include "IrcServer.hpp"

Channel::t_modeEntry	*Channel::getModeCmds()
{
	static Channel::t_modeEntry	cmds[] = {{'i', &Channel::modeI},
	{'t', &Channel::modeT}, {'k', &Channel::modeK}, {'o', &Channel::modeO},
	{'l', &Channel::modeL}};

	return (cmds);
}

void	Channel::modeI(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params) {
	(void)server;
	(void)client;
	(void)params;
	(cmd.sign == '+') ? this->setInviteOnly(true) : this->setInviteOnly(false);
	// send to buff
}

void	Channel::modeT(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params) {
	(void)server;
	(void)client;
	(void)params;
	(void)cmd;
	// to do
}

void	Channel::modeK(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params) {
	if (cmd.sign == '+'){
		if (params.size() < 3) {
			server.sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " MODE" + ERR_NEEDMOREPARAMS_MSG);
			return ;
		}
		if (!this->getKey().empty()) {
			server.sendToClientBuffer(client, ERR_KEYSET_CODE + this->getName() + ERR_KEYSET_MSG);
			return ;
		}
		this->setKey(params[2]);
	} else {
		if (params.size() < 3) {
			server.sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " MODE" + ERR_NEEDMOREPARAMS_MSG);
			return ;
		}
		if (params[2] != this->getKey()) {
			server.sendToClientBuffer(client, ERR_BADCHANNELKEY_CODE + client->getNickname() + ERR_BADCHANNELKEY_MSG);
			return ;
		}
		this->setKey("");
	}
}

void	Channel::modeO(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params) {
	(void)server;
	(void)params;
	(cmd.sign == '+') ? this->addOperator(client) : this->removeOperator(client->getFd());
}

void	Channel::modeL(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params) {
	if (cmd.sign == '+'){
		if (params.size() < 3) {
			server.sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " MODE" + ERR_NEEDMOREPARAMS_MSG);
			return ;
		}
		std::stringstream	ss;
		ss << params[2];
		int	limit;
		ss >> limit;
		if (limit < 0) {
			server.sendToClientBuffer(client, ERR_NEEDMOREPARAMS_CODE + client->getNickname() + " MODE" + ERR_NEEDMOREPARAMS_MSG);
			return ;
		}
		this->setUserLimit(limit);
	} else
		this->setUserLimit(-1);
}
