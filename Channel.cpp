#include "Channel.hpp"

Channel::Channel(const std::string &name)
	: name(name), topicSet(false), inviteOnly(false),
	  topicRestricted(false), userLimit(0)
{
}

Channel::~Channel()
{
}

const std::string	&Channel::getName() const
{
	return (name);
}

void	Channel::addMember(Client *client)
{
	members.insert(client);
}

void	Channel::removeMember(Client *client)
{
	members.erase(client);
	operators.erase(client);
}

bool	Channel::hasMember(Client *client) const
{
	return (members.find(client) != members.end());
}

bool	Channel::isEmpty() const
{
	return (members.empty());
}

size_t	Channel::getMemberCount() const
{
	return (members.size());
}

const std::set<Client *>	&Channel::getMembers() const
{
	return (members);
}

void	Channel::addOperator(Client *client)
{
	operators.insert(client);
}

void	Channel::removeOperator(Client *client)
{
	operators.erase(client);
}

bool	Channel::isOperator(Client *client) const
{
	return (operators.find(client) != operators.end());
}

void	Channel::setTopic(const std::string &topic)
{
	this->topic = topic;
	this->topicSet = true;
}

const std::string	&Channel::getTopic() const
{
	return (topic);
}

bool	Channel::hasTopic() const
{
	return (topicSet);
}

void	Channel::setKey(const std::string &key)
{
	this->key = key;
}

void	Channel::removeKey()
{
	this->key.clear();
}

bool	Channel::hasKey() const
{
	return (!key.empty());
}

const std::string	&Channel::getKey() const
{
	return (key);
}

void	Channel::setInviteOnly(bool value)
{
	inviteOnly = value;
}

bool	Channel::isInviteOnly() const
{
	return (inviteOnly);
}

void	Channel::setTopicRestricted(bool value)
{
	topicRestricted = value;
}

bool	Channel::isTopicRestricted() const
{
	return (topicRestricted);
}

void	Channel::setUserLimit(size_t limit)
{
	userLimit = limit;
}

void	Channel::removeUserLimit()
{
	userLimit = 0;
}

bool	Channel::hasUserLimit() const
{
	return (userLimit > 0);
}

size_t	Channel::getUserLimit() const
{
	return (userLimit);
}

void	Channel::invite(Client *client)
{
	invited.insert(client);
}

void	Channel::uninvite(Client *client)
{
	invited.erase(client);
}

bool	Channel::isInvited(Client *client) const
{
	return (invited.find(client) != invited.end());
}

std::string	Channel::modeString() const
{
	std::string flags;

	if (inviteOnly)
		flags += "i";
	if (topicRestricted)
		flags += "t";
	if (hasKey())
		flags += "k";
	if (hasUserLimit())
		flags += "l";
	if (flags.empty())
		return ("+");
	return ("+" + flags);
}
