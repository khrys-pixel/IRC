#include "IrcServer.hpp"

// --- Prefix Check ---
static bool	checkPrefix(std::string *restOfCommand, std::string *prefix, size_t *firstSpace) {
    if (!(*restOfCommand).empty() && (*restOfCommand)[0] == ':') {
        *firstSpace = (*restOfCommand).find(' ');
        if (*firstSpace != std::string::npos) {
            *prefix = (*restOfCommand).substr(1, *firstSpace - 1);
            *restOfCommand = (*restOfCommand).substr(*firstSpace + 1);
        } else {
            return false;
        }
    }

	return true;
}

// --- Command extraction ---
static void	parseCommand(std::string &restOfCommand, size_t firstSpace, std::vector<std::string> *parameters, std::string *command) {
	// --- Command parsing (case insensitive) ---
	firstSpace = restOfCommand.find(' ');
    if (firstSpace != std::string::npos) {
        *command = restOfCommand.substr(0, firstSpace);
        restOfCommand = restOfCommand.substr(firstSpace + 1);
    } else {
        *command = restOfCommand;
        restOfCommand.clear();
    }

    for (size_t i = 0; i < (*command).length(); ++i) {
        if ((*command)[i] >= 'a' && (*command)[i] <= 'z') {
            (*command)[i] -= ('a' - 'A');
        }
    }

	// --- Parameters extraction ---
    std::stringstream paramStream(restOfCommand);
    std::string param;
    while (paramStream >> param) {
        if (!param.empty() && param[0] == ':') {
            size_t colonPos = restOfCommand.find(':');
            std::string trailing = restOfCommand.substr(colonPos + 1);
            (*parameters).push_back(trailing);
            break; 
        }
        (*parameters).push_back(param);
    }
}

void	IrcServer::executeCommand(Client *client, std::string &command, std::vector<std::string> &parameters) {
    std::map<std::string, CommandHandler>::iterator it = _commandMap.find(command);
    if (it == _commandMap.end()) {
    }
	if (it != _commandMap.end()) {
		CommandHandler handler = it->second;
		(this->*handler)(client, parameters);
	} else {
        if (!client->isRegistered()) {
            sendToClientBuffer(client, ":ircserv 451 * :You have not registered");
        }
		// Unknown command handling / send ERR_UNKNOWNCOMMAND (421)
	}
}

void IrcServer::parseAndExecute(Client* client, const std::string& commandString) {
    if (!client || commandString.empty()) {
        return;
	}

	std::string	prefix;
    std::string	restOfCommand = commandString;
	size_t		firstSpace;

	if (!checkPrefix(&restOfCommand, &prefix, &firstSpace))
		return ;

	std::vector<std::string> parameters;
	std::string	command;
	parseCommand(restOfCommand, firstSpace, &parameters, &command);
	executeCommand(client, command, parameters);
}