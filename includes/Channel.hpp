#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <vector>
#include "Client.hpp"

# define ERR_CHANNELISFULL_CODE ":ircserv 471 "
# define ERR_CHANNELISFULL_MSG " :Cannot join channel (+l)"
# define ERR_INVITEONLYCHAN_CODE ":ircserv 473 "
# define ERR_INVITEONLYCHAN_MSG " :Cannot join channel (+i)"
# define ERR_BADCHANNELKEY_CODE ":ircserv 475 "
# define ERR_BADCHANNELKEY_MSG " :Cannot join channel (+k)"
# define ERR_KEYSET_CODE ":ircserv 467 "
# define ERR_KEYSET_MSG " :Channel key already set"

class IrcServer;

typedef struct	s_modeCmd {
	char	sign;
	char	c;
}	t_modeCmd;

class Channel {
	private:
		std::string				_name;
		std::string				_topic;
		std::map<int, Client*>	_members;
		std::map<int, Client*>	_operators;
		bool					_inviteOnly;
		bool					_topicRestriction;
		int						_userLimit;
		std::map<int, Client*>	_invites;
		std::string				_passkey;

	public:
		Channel(const std::string& name);

		const std::string&				getName() const;
		const std::string&				getTopic() const;
		void							setTopic(const std::string &newTopic);
		const std::map<int, Client*>	&getMembers() const;
		const std::map<int, Client*>	&getOperators() const;

		void	addMember(Client* client);
		void	addOperator(Client *client);
		void	removeMember(int fd);
		void	removeOperator(int fd);
		bool	isChannelMember(Client *client) const;
		bool	isOperator(Client *client) const;
		bool	isEmpty() const;
		bool	isInviteOnly();
		bool	isRestrictedTopic();
		void	setTopicRestriction(bool status);
		void	setInviteOnly(bool status);
		int		getUserLimit();
		void	setUserLimit(int limit);
		bool	canBeJoined(Client *client, std::string *msg, const std::string &key);
		void	addInvite(Client *client);
		void	setKey(const std::string &newKey);
		std::string	&getKey();
		std::string getMemberListString() const;

		// Functions for MODE commands
		typedef void (Channel::*t_modeFunc)(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params);
		typedef struct s_modeEntry {
			char	c;
			t_modeFunc  funct;
		}	t_modeEntry;
		static Channel::t_modeEntry	*getModeCmds();
		void modeI(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params);
		void modeT(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params);
		void modeK(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params);
		void modeO(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params);
		void modeL(IrcServer &server, Client *client, t_modeCmd cmd, const std::vector<std::string>& params);
};

#endif
