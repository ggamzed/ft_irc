#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cctype>
#include <cstddef>

#define SERVER_NAME		"ircserv"
#define SERVER_VERSION	"ft_irc-1.0"

static const size_t	NICK_MAX = 30;

static bool	isSpecialNickChar(char c)
{
	return (c == '[' || c == ']' || c == '\\' || c == '`'
		|| c == '_' || c == '^' || c == '{' || c == '}' || c == '|');
}

static bool	isValidNick(const std::string &nick)
{
	if (nick.empty() || nick.size() > NICK_MAX)
		return (false);
	if (!std::isalpha(static_cast<unsigned char>(nick[0])) && !isSpecialNickChar(nick[0]))
		return (false);
	for (size_t i = 1; i < nick.size(); ++i)
	{
		char c = nick[i];
		if (!std::isalnum(static_cast<unsigned char>(c)) && !isSpecialNickChar(c) && c != '-')
			return (false);
	}
	return (true);
}

static std::string	toLowerAscii(const std::string &str)
{
	std::string lowerStr(str);
	for (size_t i = 0; i < lowerStr.size(); ++i)
		lowerStr[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowerStr[i])));
	return (lowerStr);
}

static std::vector<std::string>	splitTargets(const std::string &str)
{
	std::vector<std::string>	out;
	std::string					cur;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == ',')
		{
			if (!cur.empty())
				out.push_back(cur);
			cur.clear();
		}
		else
			cur += str[i];
	}
	if (!cur.empty())
		out.push_back(cur);
	return (out);
}

void	Server::sendRaw(Client &client, const std::string &line)
{
	client.queueMessage(line + "\r\n");
}

void	Server::sendNumeric(Client &client, const std::string &code, const std::string &rest)
{
	std::string target = client.getNickname().empty() ? std::string("*") : client.getNickname();
	sendRaw(client, ":" SERVER_NAME " " + code + " " + target + " " + rest);
}

Client	*Server::findClientByNick(const std::string &nick)
{
	std::string							target = toLowerAscii(nick);
	std::map<int, Client *>::iterator	it;

	for (it = clients.begin(); it != clients.end(); ++it)
	{
		if (!it->second->getNickname().empty() && toLowerAscii(it->second->getNickname()) == target)
			return (it->second);
	}
	return (NULL);
}

void	Server::sendWelcome(Client &client)
{
	sendNumeric(client, "001", ":Welcome to the ft_irc Network, " + client.prefix());
	sendNumeric(client, "002", ":Your host is " SERVER_NAME ", running version " SERVER_VERSION);
	sendNumeric(client, "003", ":This server was created " __DATE__ " " __TIME__);
	sendNumeric(client, "004", std::string(SERVER_NAME) + " " SERVER_VERSION " o itkol");
}

void	Server::tryRegister(Client &client)
{
	if (client.isRegistered())
		return ;
	if (client.getNickname().empty() || client.getUsername().empty())
		return ;
	if (!client.hasPassOk())
	{
		sendNumeric(client, "464", ":Password incorrect");
		client.requestClose();
		return ;
	}
	client.setRegistered(true);
	sendWelcome(client);
	std::cout << "Client registered: " << client.prefix() << " (fd " << client.getFd() << ")" << std::endl;
}

void	Server::handleCap(Client &client, const Message &msg)
{
	if (msg.params.empty())
		return ;
	if (msg.params[0] == "LS" || msg.params[0] == "LIST")
		sendRaw(client, ":" SERVER_NAME " CAP * LS :");
	else if (msg.params[0] == "REQ")
		sendRaw(client, ":" SERVER_NAME " CAP * NAK :" + (msg.params.size() > 1 ? msg.params[1] : std::string()));
}

void	Server::handlePass(Client &client, const Message &msg)
{
	if (client.isRegistered())
	{
		sendNumeric(client, "462", ":You may not reregister");
		return ;
	}
	if (msg.params.empty())
	{
		sendNumeric(client, "461", "PASS :Not enough parameters");
		return ;
	}
	client.setPassOk(msg.params[0] == password);
	tryRegister(client);
}

void	Server::broadcastToClientChannels(Client &client, const std::string &line)
{
	std::set<Client *>						notified;
	const std::set<std::string>				&channels = client.getChannels();
	std::set<std::string>::const_iterator	cit;

	sendRaw(client, line);
	notified.insert(&client);
	for (cit = channels.begin(); cit != channels.end(); ++cit)
	{
		Channel *channel = findChannel(*cit);
		if (!channel)
			continue ;

		const std::set<Client *>			&members = channel->getMembers();
		std::set<Client *>::const_iterator	mit;

		for (mit = members.begin(); mit != members.end(); ++mit)
		{
			if (notified.find(*mit) == notified.end())
			{
				notified.insert(*mit);
				sendRaw(**mit, line);
			}
		}
	}
}

void	Server::handleNick(Client &client, const Message &msg)
{
	if (msg.params.empty() || msg.params[0].empty())
	{
		sendNumeric(client, "431", ":No nickname given");
		return ;
	}

	const std::string &newNick = msg.params[0];
	if (!isValidNick(newNick))
	{
		sendNumeric(client, "432", newNick + " :Erroneous nickname");
		return ;
	}

	Client *existing = findClientByNick(newNick);
	if (existing && existing != &client)
	{
		sendNumeric(client, "433", newNick + " :Nickname is already in use");
		return ;
	}

	std::string oldNick = client.getNickname();

	if (client.isRegistered() && !oldNick.empty() && toLowerAscii(oldNick) != toLowerAscii(newNick))
		broadcastToClientChannels(client, ":" + client.prefix() + " NICK :" + newNick);
	client.setNickname(newNick);
	tryRegister(client);
}

void	Server::handleUser(Client &client, const Message &msg)
{
	if (client.isRegistered())
	{
		sendNumeric(client, "462", ":You may not reregister");
		return ;
	}
	if (msg.params.size() < 4 || msg.params[0].empty())
	{
		sendNumeric(client, "461", "USER :Not enough parameters");
		return ;
	}
	client.setUsername(msg.params[0]);
	client.setRealname(msg.params[3]);
	tryRegister(client);
}

void	Server::handlePing(Client &client, const Message &msg)
{
	std::string token = msg.params.empty() ? std::string(SERVER_NAME) : msg.params[0];
	sendRaw(client, ":" SERVER_NAME " PONG " SERVER_NAME " :" + token);
}

void	Server::handleQuit(Client &client, const Message &msg)
{
	std::string reason = msg.params.empty() ? std::string("Client Quit") : msg.params[0];
	client.setQuitReason(reason);
	sendRaw(client, "ERROR :Closing Link: " + client.getHostname() + " (" + reason + ")");
	client.requestClose();
}

void	Server::handlePrivmsg(Client &client, const Message &msg, bool isNotice)
{
	if (msg.params.empty())
	{
		if (!isNotice)
			sendNumeric(client, "411", ":No recipient given (PRIVMSG)");
		return ;
	}
	if (msg.params.size() < 2 || msg.params[1].empty())
	{
		if (!isNotice)
			sendNumeric(client, "412", ":No text to send");
		return ;
	}

	const std::string			&text = msg.params[1];
	const std::string			verb = isNotice ? "NOTICE" : "PRIVMSG";
	std::vector<std::string>	targets = splitTargets(msg.params[0]);
	for (size_t i = 0; i < targets.size(); ++i)
	{
		const std::string &t = targets[i];

		if (!t.empty() && (t[0] == '#' || t[0] == '&'))
		{
			deliverToChannel(client, t, text, isNotice);
			continue;
		}

		Client *dest = findClientByNick(t);
		if (!dest || !dest->isRegistered())
		{
			if (!isNotice)
				sendNumeric(client, "401", t + " :No such nick/channel");
			continue;
		}
		sendRaw(*dest, ":" + client.prefix() + " " + verb + " " + t + " :" + text);
	}
}

void	Server::deliverToChannel(Client &sender, const std::string &channel, const std::string &text, bool isNotice)
{
	Channel *chan = findChannel(channel);

	if (!chan)
	{
		if (!isNotice)
			sendNumeric(sender, "403", channel + " :No such channel");
		return ;
	}
	if (!chan->hasMember(&sender))
	{
		if (!isNotice)
			sendNumeric(sender, "404", channel + " :Cannot send to channel");
		return ;
	}
	const std::string verb = isNotice ? "NOTICE" : "PRIVMSG";
	broadcastToChannel(*chan, ":" + sender.prefix() + " " + verb + " " + chan->getName() + " :" + text, &sender);
}

void	Server::processCommand(Client &client, const Message &msg)
{
	const std::string &cmd = msg.command;

	if (cmd.empty())
		return ;

	typedef void (Server::*Handler)(Client &, const Message &);
	std::map<std::string, Handler> commands;
	commands["CAP"]    = &Server::handleCap;
	commands["PASS"]   = &Server::handlePass;
	commands["NICK"]   = &Server::handleNick;
	commands["USER"]   = &Server::handleUser;
	commands["PING"]   = &Server::handlePing;
	commands["QUIT"]   = &Server::handleQuit;
	commands["JOIN"]   = &Server::handleJoin;
	commands["PART"]   = &Server::handlePart;
	commands["KICK"]   = &Server::handleKick;
	commands["INVITE"] = &Server::handleInvite;
	commands["TOPIC"]  = &Server::handleTopic;
	commands["MODE"]   = &Server::handleMode;

	if (cmd == "PONG")
		return ;

	if (cmd == "PRIVMSG")
	{
		if (!client.isRegistered())
		{
			sendNumeric(client, "451", ":You have not registered");
			return ;
		}

		handlePrivmsg(client, msg, false);
		return ;
	}

	if (cmd == "NOTICE")
	{
		if (!client.isRegistered())
		{
			sendNumeric(client, "451", ":You have not registered");
			return ;
		}

		handlePrivmsg(client, msg, true);
		return ;
	}

	std::map<std::string, Handler>::iterator it = commands.find(cmd);
	if (it == commands.end())
	{
		sendNumeric(client, "421", cmd + " :Unknown command");
		return ;
	}
	if (!client.isRegistered())
	{
		if (cmd != "CAP" && cmd != "PASS" &&
			cmd != "NICK" && cmd != "USER" &&
			cmd != "PING" && cmd != "QUIT")
		{
			sendNumeric(client, "451", ":You have not registered");
			return ;
		}
	}
	(this->*(it->second))(client, msg);
}