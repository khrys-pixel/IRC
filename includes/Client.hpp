#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <iostream>


# include <stdio.h> //REMOVE LATER


class Client {
private:
	int			_fd;
	std::string	_ip;
	int			_port;
	
	std::string _inputBuffer;
	std::string _outputBuffer;

	bool		_isRegistered;
	bool		_isOperator;
	bool		_hasGivenPassword;

	std::string	_nickname;
	std::string	_username;
	std::string	_realname;

public:
	Client(int fd, const std::string& ip, int port);
	~Client();

	// Buffer management
	void addToInputBuffer(const std::string& data);
	bool getNextCommand(std::string& commandOut);
	void addToOutputBuffer(const std::string& message);
	const std::string& getOutputBuffer() const;
	void removeFromOutputBuffer(int bytesSent);

	// Getters & Setters
	int getFd() const;
	const std::string& getIp() const;
	int getPort() const;

	bool hasGivenPassword() const;
	void setHasGivenPassword(bool status);

	bool isRegistered() const;
	void setRegistered(bool status);

	bool isOperator() const;
	void setOperator(bool status);

	const std::string& getNickname() const;
	void setNickname(const std::string& nick);

	const std::string& getUsername() const;
	void setUsername(const std::string& user);

	const std::string& getRealname() const;
	void setRealname(const std::string& real);
};

#endif
