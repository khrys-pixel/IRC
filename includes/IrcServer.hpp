#ifndef IRCSERVER_HPP
# define IRCSERVER_HPP

# include <string>
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

class IrcServer {
	private:
		int					_port;
		std::string			_password;
		int					_listenSocketFd;
		struct sockaddr_in	_serverAddr;
		
		std::vector<struct pollfd>		_pollFds;
		std::map<std::string, Channel*>	_channels;
		std::map<int, Client*>			_clients;

		// Internal initialization & socket helpers
		void initSocket();
		void setSocketNonBlocking(int fd);

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
		void	initCommands();

	public:
		IrcServer(int port, const std::string& password);
		~IrcServer();

		void run();
		void parseAndExecute(Client* client, const std::string& commandString);
		void checkAndRegister(Client* client);
};

#endif
