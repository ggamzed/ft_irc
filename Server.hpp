#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>

class	Client;
class	Channel;
struct	Message;

class	Server
{
	private:
		int						listenFd;
		int						port;
		std::string				password;
		std::vector<pollfd>		pollfds;
		std::map<int, Client *>	clients;

		std::map<std::string, Channel *>	channels;

		void	setupSocket();
		void	updatePollEvents();
		void	setupSignals();
		void	processReadyFds(std::vector<int> &toClose);
		void	acceptClient();
		bool	handleClientRead(int fd);
		bool	handleClientWrite(int fd);
		void	closeClient(int fd);
		void	processCommand(Client &client, const Message &msg);

		void	sendRaw(Client &client, const std::string &line);
		void	sendNumeric(Client &client, const std::string &code, const std::string &rest);
		Client	*findClientByNick(const std::string &nick);
		void	tryRegister(Client &client);
		void	sendWelcome(Client &client);
		void	broadcastToClientChannels(Client &client, const std::string &line);
		void	handleCap(Client &client, const Message &msg);
		void	handlePass(Client &client, const Message &msg);
		void	handleNick(Client &client, const Message &msg);
		void	handleUser(Client &client, const Message &msg);
		void	handlePing(Client &client, const Message &msg);
		void	handleQuit(Client &client, const Message &msg);
		void	handlePrivmsg(Client &client, const Message &msg, bool isNotice);
		void	deliverToChannel(Client &sender, const std::string &channel, const std::string &text, bool isNotice);

		struct Mode
		{
			char		sign;
			char		flag;
			bool		hasExtraParam;
			std::string	extraParam;
		};

		Channel	*findChannel(const std::string &name);
		void	broadcastToChannel(Channel &channel, const std::string &line, Client *exclude);
		void	sendMembersList(Client &client, Channel &channel);
		void	joinSingleChannel(Client &client, const std::string &name, const std::string &key);
		void	removeClientFromAllChannels(Client &client, const std::string &reason);
		void	handleJoin(Client &client, const Message &msg);
		void	handlePart(Client &client, const Message &msg);
		void	handleKick(Client &client, const Message &msg);
		void	handleInvite(Client &client, const Message &msg);
		void	handleTopic(Client &client, const Message &msg);
		void	handleMode(Client &client, const Message &msg);

		Channel		*findValidatedChannel(Client &client, const std::string &chanName);
		bool		validateMembership(Client &client, Channel &channel);
		bool		validateOperator(Client &client, Channel &channel);
		void		removeMemberFromChannel(Channel &channel, Client &member);
		bool		checkModeParam(Client &client, const Message &msg, size_t extraParamIdx);
		void		processModeChar(Client &client, Channel &channel, const Message &msg, char sign, char c,
						size_t &extraParamIdx, std::vector<Mode> &modeChanges);
		std::string	modeChangesToString(const std::vector<Mode> &modeChanges);

	public:
		Server(int port, const std::string &password);
		~Server();

		void	run();
};

#endif
