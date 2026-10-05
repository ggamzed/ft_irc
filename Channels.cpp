#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <cctype>
#include <cstdlib>
#include <vector>
#include <set>

static std::string toLowerAscii(const std::string &str)
{
	std::string result(str);

	for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	return (result);
}

static std::vector<std::string> splitByComma(const std::string &str)
{
	std::vector<std::string>	tokens;
	std::string					cur;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == ',')
		{
			tokens.push_back(cur);
			cur.clear();
		}
		else
			cur += str[i];
	}
	tokens.push_back(cur);
	return (tokens);
}

static bool isValidChannelName(const std::string &name)
{
	if (name.size() < 2 || name.size() > 50)
		return (false);
	if (name[0] != '#' && name[0] != '&')
		return (false);
	for (size_t i = 1; i < name.size(); ++i)
	{
		if (name[i] == ' ' || name[i] == ',' || name[i] == '\x07')
			return (false);
	}
	return (true);
}

Channel *Server::findChannel(const std::string &chanName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(toLowerAscii(chanName));

	if (it == channels.end())
		return (NULL);
	return (it->second);
}

void Server::broadcastToChannel(Channel &channel, const std::string &line, Client *exclude)
{
	const std::set<Client *>			&members = channel.getMembers();
	std::set<Client *>::const_iterator	it;

	for (it = members.begin(); it != members.end(); ++it)
	{
		if (*it != exclude)
			sendRaw(**it, line);
	}
}

void Server::sendMembersList(Client &client, Channel &channel)
{
	const std::set<Client *>			&members = channel.getMembers();
	std::set<Client *>::const_iterator	it;
	std::string							names;

	for (it = members.begin(); it != members.end(); ++it)
	{
		if (!names.empty())
			names += " ";
		if (channel.isOperator(*it))
			names += "@";
		names += (*it)->getNickname();
	}
	sendNumeric(client, "353", "= " + channel.getName() + " :" + names);
	sendNumeric(client, "366", channel.getName() + " :End of /NAMES list");
}

Channel *Server::findValidatedChannel(Client &client, const std::string &chanName)
{
	Channel *channel = findChannel(chanName);

	if (!channel)
		sendNumeric(client, "403", chanName + " :No such channel");
	return (channel);
}

bool Server::validateMembership(Client &client, Channel &channel)
{
	if (!channel.hasMember(&client))
	{
		sendNumeric(client, "442", channel.getName() + " :You're not on that channel");
		return (false);
	}
	return (true);
}

bool Server::validateOperator(Client &client, Channel &channel)
{
	if (!channel.isOperator(&client))
	{
		sendNumeric(client, "482", channel.getName() + " :You're not channel operator");
		return (false);
	}
	return (true);
}

void Server::removeMemberFromChannel(Channel &channel, Client &member)
{
	channel.removeMember(&member);
	channel.uninvite(&member);
	member.leaveChannel(channel.getName());

	if (channel.isEmpty())
	{
		channels.erase(toLowerAscii(channel.getName()));
		delete &channel;
	}
}

void Server::joinSingleChannel(Client &client, const std::string &chanName, const std::string &key)
{
	if (!isValidChannelName(chanName))
	{
		sendNumeric(client, "403", chanName + " :No such channel");
		return ;
	}
	if (client.isInChannel(chanName))
		return ;

	Channel	*channel = findChannel(chanName);
	bool	isNewChan = (channel == NULL);

	if (channel)
	{
		if (channel->isInviteOnly() && !channel->isInvited(&client))
		{
			sendNumeric(client, "473", chanName + " :Cannot join channel (+i)");
			return ;
		}
		if (channel->hasKey() && (channel->getKey() != key))
		{
			sendNumeric(client, "475", chanName + " :Cannot join channel (+k)");
			return ;
		}
		if (channel->hasUserLimit() && (channel->getMemberCount() >= channel->getUserLimit()))
		{
			sendNumeric(client, "471", chanName + " :Cannot join channel (+l)");
			return ;
		}
	}
	else
	{
		channel = new Channel(chanName);
		channels[toLowerAscii(chanName)] = channel;
	}

	channel->addMember(&client);
	if (isNewChan)
		channel->addOperator(&client);
	channel->uninvite(&client);
	client.joinChannel(channel->getName());

	broadcastToChannel(*channel, ":" + client.prefix() + " JOIN :" + channel->getName(), NULL);
	if (channel->hasTopic())
		sendNumeric(client, "332", channel->getName() + " :" + channel->getTopic());
	else
		sendNumeric(client, "331", channel->getName() + " :No topic is set");
	sendMembersList(client, *channel);
}

void Server::handleJoin(Client &client, const Message &msg)
{
	if (msg.params.empty())
	{
		sendNumeric(client, "461", "JOIN :Not enough parameters");
		return ;
	}

	std::vector<std::string>	chanNames = splitByComma(msg.params[0]);
	std::vector<std::string>	keys;

	if (msg.params.size() > 1)
		keys = splitByComma(msg.params[1]);

	for (size_t i = 0; i < chanNames.size(); ++i)
	{
		if (chanNames[i].empty())
			continue ;
		std::string key = (i < keys.size()) ? keys[i] : std::string();
		joinSingleChannel(client, chanNames[i], key);
	}
}

void Server::handlePart(Client &client, const Message &msg)
{
	if (msg.params.empty())
	{
		sendNumeric(client, "461", "PART :Not enough parameters");
		return ;
	}

	std::vector<std::string>	chanNames = splitByComma(msg.params[0]);
	std::string					reason = (msg.params.size() > 1) ? msg.params[1] : client.getNickname();

	for (size_t i = 0; i < chanNames.size(); ++i)
	{
		const std::string	&chanName = chanNames[i];
		Channel				*channel = findValidatedChannel(client, chanName);

		if (!channel)
			continue ;
		if (!validateMembership(client, *channel))
			continue ;
		broadcastToChannel(*channel, ":" + client.prefix() + " PART " + channel->getName() + " :" + reason, NULL);
		removeMemberFromChannel(*channel, client);
	}
}

void Server::removeClientFromAllChannels(Client &client, const std::string &reason)
{
	std::set<std::string>			chanNames = client.getChannels();
	std::set<std::string>::iterator	it;

	for (it = chanNames.begin(); it != chanNames.end(); ++it)
	{
		Channel *chan = findChannel(*it);

		if (!chan)
			continue ;
		broadcastToChannel(*chan, ":" + client.prefix() + " QUIT :" + reason, &client);
		removeMemberFromChannel(*chan, client);
	}
}

void Server::handleKick(Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		sendNumeric(client, "461", "KICK :Not enough parameters");
		return ;
	}

	Channel *channel = findValidatedChannel(client, msg.params[0]);

	if (!channel)
		return ;
	if (!validateMembership(client, *channel))
		return ;
	if (!validateOperator(client, *channel))
		return ;

	Client *target = findClientByNick(msg.params[1]);

	if (!target || !channel->hasMember(target))
	{
		sendNumeric(client, "441", msg.params[1] + " " + channel->getName() + " :They aren't on that channel");
		return ;
	}

	std::string reason = (msg.params.size() > 2) ? msg.params[2] : client.getNickname();

	broadcastToChannel(*channel, ":" + client.prefix() + " KICK " + channel->getName() + " "
		+ target->getNickname() + " :" + reason, NULL);
	removeMemberFromChannel(*channel, *target);
}

void Server::handleInvite(Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		sendNumeric(client, "461", "INVITE :Not enough parameters");
		return ;
	}

	Channel *channel = findValidatedChannel(client, msg.params[1]);

	if (!channel)
		return ;
	if (!validateMembership(client, *channel))
		return ;
	if (channel->isInviteOnly() && !validateOperator(client, *channel))
		return ;

	Client *target = findClientByNick(msg.params[0]);

	if (!target)
	{
		sendNumeric(client, "401", msg.params[0] + " :No such nick/channel");
		return ;
	}
	if (channel->hasMember(target))
	{
		sendNumeric(client, "443", target->getNickname() + " " + channel->getName() + " :is already on channel");
		return ;
	}

	channel->invite(target);
	sendNumeric(client, "341", target->getNickname() + " " + channel->getName());
	sendRaw(*target, ":" + client.prefix() + " INVITE " + target->getNickname() + " :" + channel->getName());
}

void Server::handleTopic(Client &client, const Message &msg)
{
	if (msg.params.empty())
	{
		sendNumeric(client, "461", "TOPIC :Not enough parameters");
		return ;
	}

	Channel *channel = findValidatedChannel(client, msg.params[0]);

	if (!channel)
		return ;
	if (!validateMembership(client, *channel))
		return ;
	if (msg.params.size() < 2)
	{
		if (channel->hasTopic())
			sendNumeric(client, "332", channel->getName() + " :" + channel->getTopic());
		else
			sendNumeric(client, "331", channel->getName() + " :No topic is set");
		return ;
	}
	if (channel->isTopicRestricted() && !validateOperator(client, *channel))
		return ;

	channel->setTopic(msg.params[1]);
	broadcastToChannel(*channel, ":" + client.prefix() + " TOPIC " + channel->getName() + " :" + msg.params[1], NULL);
}

bool Server::checkModeParam(Client &client, const Message &msg, size_t paramIdx)
{
	if (msg.params.size() <= paramIdx)
	{
		sendNumeric(client, "461", "MODE :Not enough parameters");
		return (false);
	}
	return (true);
}

void Server::processModeChar(Client &client, Channel &channel, const Message &msg, char sign, char c,
	size_t &extraParamIdx, std::vector<Mode> &modeChanges)
{
	Mode mode;

	mode.sign = sign;
	mode.flag = c;
	mode.hasExtraParam = false;

	if (c == 'i')
	{
		channel.setInviteOnly(sign == '+');
		modeChanges.push_back(mode);
	}
	else if (c == 't')
	{
		channel.setTopicRestricted(sign == '+');
		modeChanges.push_back(mode);
	}
	else if (c == 'k')
	{
		if (sign == '+')
		{
			if (!checkModeParam(client, msg, extraParamIdx))
				return ;
			channel.setKey(msg.params[extraParamIdx]);
			mode.hasExtraParam = true;
			mode.extraParam = msg.params[extraParamIdx];
			extraParamIdx++;
		}
		else
			channel.removeKey();
		modeChanges.push_back(mode);
	}
	else if (c == 'l')
	{
		if (sign == '+')
		{
			if (!checkModeParam(client, msg, extraParamIdx))
				return ;

			long limit = std::atol(msg.params[extraParamIdx].c_str());

			if (limit <= 0)
			{
				extraParamIdx++;
				return ;
			}
			channel.setUserLimit(static_cast<size_t>(limit));
			mode.hasExtraParam = true;
			mode.extraParam = msg.params[extraParamIdx];
			extraParamIdx++;
		}
		else
			channel.removeUserLimit();
		modeChanges.push_back(mode);
	}
	else if (c == 'o')
	{
		if (!checkModeParam(client, msg, extraParamIdx))
			return ;

		const std::string &targetNick = msg.params[extraParamIdx];

		extraParamIdx++;

		Client *target = findClientByNick(targetNick);
		
		if (!target || !channel.hasMember(target))
		{
			sendNumeric(client, "441", targetNick + " " + channel.getName() + " :They aren't on that channel");
			return ;
		}
		if (sign == '+')
			channel.addOperator(target);
		else
			channel.removeOperator(target);
		mode.hasExtraParam = true;
		mode.extraParam = targetNick;
		modeChanges.push_back(mode);
	}
	else
		sendNumeric(client, "472", std::string(1, c) + " :is unknown mode char to me");
}

std::string Server::modeChangesToString(const std::vector<Mode> &modeChanges)
{
	std::string	modesResult;
	char		prev = 0;

	for (size_t i = 0; i < modeChanges.size(); ++i)
	{
		if (modeChanges[i].sign != prev)
		{
			modesResult += modeChanges[i].sign;
			prev = modeChanges[i].sign;
		}
		modesResult += modeChanges[i].flag;
	}
	for (size_t i = 0; i < modeChanges.size(); ++i)
	{
		if (modeChanges[i].hasExtraParam)
			modesResult += " " + modeChanges[i].extraParam;
	}
	return (modesResult);
}

void Server::handleMode(Client &client, const Message &msg)
{
	if (msg.params.empty())
	{
		sendNumeric(client, "461", "MODE :Not enough parameters");
		return ;
	}

	Channel *channel = findValidatedChannel(client, msg.params[0]);

	if (!channel)
		return ;
	if (msg.params.size() < 2)
	{
		sendNumeric(client, "324", channel->getName() + " " + channel->modeString());
		return ;
	}
	if (!validateMembership(client, *channel))
		return ;
	if (!validateOperator(client, *channel))
		return ;

	const std::string			&modesRaw = msg.params[1];
	size_t						extraParamIdx = 2;
	char						sign = '+';
	std::vector<Mode>			modeChanges;

	for (size_t i = 0; i < modesRaw.size(); ++i)
	{
		char c = modesRaw[i];

		if (c == '+' || c == '-')
		{
			sign = c;
			continue ;
		}
		processModeChar(client, *channel, msg, sign, c, extraParamIdx, modeChanges);
	}
	if (modeChanges.empty())
		return ;
	std::string modesResult = modeChangesToString(modeChanges);
	broadcastToChannel(*channel, ":" + client.prefix() + " MODE " + channel->getName() + " " + modesResult, NULL);
}
