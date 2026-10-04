#include "IrcServer.hpp"
#include <iostream>
#include <sstream>
#include <vector>

void IrcServer::parseAndExecute(Client* client, const std::string& commandString) {
    if (!client || commandString.empty())
        return;

    std::string prefix;
    std::string command;
    std::vector<std::string> parameters;
    std::string restOfCommand = commandString;

    // --- Prefix Check ---
    if (!restOfCommand.empty() && restOfCommand[0] == ':') {
        size_t firstSpace = restOfCommand.find(' ');
        if (firstSpace != std::string::npos) {
            prefix = restOfCommand.substr(1, firstSpace - 1);
            restOfCommand = restOfCommand.substr(firstSpace + 1);
        } else {
            return;
        }
    }

    // --- Command extraction (case insensitive) ---
    size_t firstSpace = restOfCommand.find(' ');
    if (firstSpace != std::string::npos) {
        command = restOfCommand.substr(0, firstSpace);
        restOfCommand = restOfCommand.substr(firstSpace + 1);
    } else {
        command = restOfCommand;
        restOfCommand.clear();
    }

    for (size_t i = 0; i < command.length(); ++i) {
        if (command[i] >= 'a' && command[i] <= 'z') {
            command[i] -= ('a' - 'A');
        }
    }

    // --- Parameters extraction ---
    std::stringstream paramStream(restOfCommand);
    std::string param;
    while (paramStream >> param) {
        if (!param.empty() && param[0] == ':') {
            size_t colonPos = restOfCommand.find(':');
            std::string trailing = restOfCommand.substr(colonPos + 1);
            parameters.push_back(trailing);
            break; 
        }
        parameters.push_back(param);
    }

    // --- Command Handlers ---
    if (command == "PASS") {
        if (client->isRegistered()) {
            client->addToOutputBuffer(":ircserv 462 * :You may not reregister");
            return;
        }
        if (parameters.empty()) {
            client->addToOutputBuffer(":ircserv 461 * PASS :Not enough parameters");
            return;
        }
        if (parameters[0] == _password) {
            client->setHasGivenPassword(true);
        } else {
            client->addToOutputBuffer(":ircserv 464 * :Password incorrect");
        }
    } 
    else if (command == "NICK") {
        if (parameters.empty()) {
            client->addToOutputBuffer(":ircserv 431 * :No nickname given");
            return;
        }
        client->setNickname(parameters[0]);
        checkAndRegister(client);
    } 
    else if (command == "USER") {
        if (parameters.size() < 4) {
            client->addToOutputBuffer(":ircserv 461 * USER :Not enough parameters");
            return;
        }
        client->setUsername(parameters[0]);
        client->setRealname(parameters[3]);
        checkAndRegister(client);
    }
    else if (!client->isRegistered()) {
        client->addToOutputBuffer(":ircserv 451 * :You have not registered");
    }
    else if (command == "JOIN") {
        handleJoin(client, parameters);
    }
    else if (command == "PING") {
        std::string target = parameters.empty() ? "ircserv" : parameters[0];
        client->addToOutputBuffer(":ircserv PONG ircserv :" + target);
    }
}

void IrcServer::checkAndRegister(Client* client) {
    if (!client->isRegistered() && client->hasGivenPassword() && 
        !client->getNickname().empty() && !client->getUsername().empty()) 
    {
        client->setRegistered(true);
        std::string nick = client->getNickname();

        // Standard RFC 1459 registration sequence
        client->addToOutputBuffer(":ircserv 001 " + nick + " :Welcome to the ft_irc Network " + nick);
        client->addToOutputBuffer(":ircserv 002 " + nick + " :Your host is ircserv, running version 1.0");
        client->addToOutputBuffer(":ircserv 003 " + nick + " :This server was created today");
        client->addToOutputBuffer(":ircserv 004 " + nick + " ircserv 1.0 io itkol");
    }
}

void IrcServer::handleJoin(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        client->addToOutputBuffer(":ircserv 461 " + client->getNickname() + " JOIN :Not enough parameters");
        return;
    }

    std::string chanName = params[0];
    if (chanName.empty() || chanName[0] != '#') {
        client->addToOutputBuffer(":ircserv 403 " + client->getNickname() + " " + chanName + " :No such channel");
        return;
    }

    // Retrieve or create the channel
    if (_channels.find(chanName) == _channels.end()) {
        _channels[chanName] = new Channel(chanName);
    }
    Channel* chan = _channels[chanName];
    chan->addMember(client);

    // 1. Broadcast JOIN notification to channel members
    std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@127.0.0.1 JOIN " + chanName;
    chan->broadcast(joinMsg);

    // 2. Send Channel Topic (331 or 332)
    if (chan->getTopic().empty()) {
        client->addToOutputBuffer(":ircserv 331 " + client->getNickname() + " " + chanName + " :No topic is set");
    } else {
        client->addToOutputBuffer(":ircserv 332 " + client->getNickname() + " " + chanName + " :" + chan->getTopic());
    }

    // 3. Send Member List (353) & End of /NAMES (366)
    client->addToOutputBuffer(":ircserv 353 " + client->getNickname() + " = " + chanName + " :" + chan->getMemberListString());
    client->addToOutputBuffer(":ircserv 366 " + client->getNickname() + " " + chanName + " :End of /NAMES list");
}
