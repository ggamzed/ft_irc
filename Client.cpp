#include "Client.hpp"
#include <unistd.h>

Client::Client(int fd) : fd(fd), registered(false), passOk(false), closeRequested(false),
	quitReason("Connection closed")
{
}

Client::~Client()
{
	close(fd);
}

int	Client::getFd() const
{
	return (fd);
}

void	Client::appendToReadBuffer(const char *data, size_t len)
{
	readBuffer.append(data, len);
}

bool	Client::extractLine(std::string &line)
{
	size_t pos = readBuffer.find('\n');
	if (pos == std::string::npos)
		return (false);

	std::string extracted = readBuffer.substr(0, pos);
	if (!extracted.empty() && extracted[extracted.size() - 1] == '\r')
		extracted.erase(extracted.size() - 1);

	readBuffer.erase(0, pos + 1);
	line = extracted;
	return (true);
}

void	Client::queueMessage(const std::string &data)
{
	writeBuffer.append(data);
}

bool	Client::hasPendingWrite() const
{
	return (!writeBuffer.empty());
}

const std::string	&Client::getWriteBuffer() const
{
	return (writeBuffer);
}

void	Client::consumeWriteBuffer(size_t n)
{
	writeBuffer.erase(0, n);
}

const std::string	&Client::getNickname() const
{
	return (nickname);
}

void	Client::setNickname(const std::string &nickname)
{
	this->nickname = nickname;
}

const std::string	&Client::getUsername() const
{
	return (username);
}

void	Client::setUsername(const std::string &username)
{
	this->username = username;
}

bool	Client::isRegistered() const
{
	return (registered);
}

void	Client::setRegistered(bool registered)
{
	this->registered = registered;
}

const std::string	&Client::getRealname() const
{
	return (realname);
}

void	Client::setRealname(const std::string &realname)
{
	this->realname = realname;
}

const std::string	&Client::getHostname() const
{
	return (hostname);
}

void	Client::setHostname(const std::string &hostname)
{
	this->hostname = hostname;
}

bool	Client::hasPassOk() const
{
	return (passOk);
}

void	Client::setPassOk(bool ok)
{
	this->passOk = ok;
}

void	Client::requestClose()
{
	this->closeRequested = true;
}

bool	Client::wantClose() const
{
	return (closeRequested);
}

std::string	Client::prefix() const
{
	return (nickname + "!" + username + "@" + hostname);
}

void	Client::setQuitReason(const std::string &reason)
{
	this->quitReason = reason;
}

const std::string	&Client::getQuitReason() const
{
	return (quitReason);
}

void	Client::joinChannel(const std::string &channelName)
{
	channels.insert(channelName);
}

void	Client::leaveChannel(const std::string &channelName)
{
	channels.erase(channelName);
}

bool	Client::isInChannel(const std::string &channelName) const
{
	return (channels.find(channelName) != channels.end());
}

const std::set<std::string>	&Client::getChannels() const
{
	return (channels);
}
