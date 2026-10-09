#ifndef IRCSERVER_HPP
# define IRCSERVER_HPP

# include <string>
# include <cstring>
# include <vector>
# include <map>
# include <poll.h>
# include <netinet/in.h>
# include <sys/socket.h>
#include <arpa/inet.h>
# include <unistd.h>
# include <fcntl.h>

# include <iostream>
# include <sstream>
# include <stdexcept>

# include <cstdio> // For perror
# include <errno.h>

# include "Channel.hpp"
# include "Client.hpp"

# include <stdio.h> //REMOVE LATER

# define ERR_NOSUCHNICK_VAL ":ircserv 401 "
# define ERR_NOSUCHNICK_MSG " :No such nick/channel"
# define ERR_NOSUCHCHANNEL_VAL ":ircserv 403 "
# define ERR_NOSUCHCHANNEL_MSG " :No such channel"
# define ERR_USERNOTINCHANNEL_VAL ":ircserv 441 "
# define ERR_USERNOTINCHANNEL_MSG " :They aren't on that channel"
# define ERR_NEEDMOREPARAMS_VAL ":ircserv 461 "
# define ERR_NEEDMOREPARAMS_MSG " :Not enough parameters"
# define ERR_NOPRIVILEGES_VAL ":ircserv 481 "
# define ERR_NOPRIVILEGES_MSG " :Permission Denied - You're not an operator"
# define ERR_CHANOPRIVSNEEDED_VAL ":ircserv 482 "
# define ERR_CHANOPRIVSNEEDED_MSG " :You're not channel operator"
# define ERR_UMODEUNKNOWNFLAG_VAL ":ircserv 501 "
# define ERR_UMODEUNKNOWNFLAG_MSG " :Unknown MODE flag"



typedef enum	e_op {
	MSG,
	MODE
}	t_op;

class IrcServer {
	private:
		int					_port;
		std::string			_password;
		int					_listenSocketFd;
		struct sockaddr_in	_serverAddr;
		
		std::vector<struct pollfd>		_pollFds;
		std::map<std::string, Channel*>	_channels;
		std::map<int, Client*>			_clients;

		// Internal helpers
		void	initSocket();
		void	setSocketNonBlocking(int fd);
		void	formatAndSend(Client *reciever, Client *sender, Channel *channel, const std::string &msg, e_op op);
		Client	*findClientByNick(const std::string &nick);
		void	makeOperator(Channel *channel, Client *client, bool status);

		// Event handlers
		void	executeCommand(Client *client, std::string &command, std::vector<std::string> &parameters);
		void	acceptNewConnection();
		void	handleClientActivity(int fd, int index);
		void	disconnectClient(int fd, int index);
		void	sendToClientBuffer(Client* client, const std::string& message);

		// Command to handler function mapping
		typedef void (IrcServer::*CommandHandler)(Client* client, const std::vector<std::string>& params);
		std::map<std::string, CommandHandler> _commandMap;

		void	handlePass(Client* client, const std::vector<std::string>& params);
		void	handleNick(Client* client, const std::vector<std::string>& params);
		void	handleUser(Client* client, const std::vector<std::string>& params);
		void	handlePing(Client* client, const std::vector<std::string>& params);
		void	handleJoin(Client* client, const std::vector<std::string>& params);
		void	handlePrivmsg(Client* client, const std::vector<std::string>& params);
		void	handleChannelMsg(Client* client, const std::vector<std::string>& params);
		void	handleMode(Client* client, const std::vector<std::string>& params);
		void	initCommands();

	public:
		IrcServer(int port, const std::string& password);
		~IrcServer();

		void run();
		void parseAndExecute(Client* client, const std::string& commandString);
		void checkAndRegister(Client* client);
};

#endif
