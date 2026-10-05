#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

class	Client
{
	private:
		int			fd;
		bool		registered;
		std::string	readBuffer;
		std::string	writeBuffer;
		std::string	nickname;
		std::string	username;
		std::string	realname;
		std::string	hostname;
		bool		passOk;
		bool		closeRequested;
		std::string				quitReason;
		std::set<std::string>	channels;

	public:
		explicit Client(int fd);
		~Client();

		const std::string	&getWriteBuffer() const;
		void				consumeWriteBuffer(size_t n);
		void				appendToReadBuffer(const char *data, size_t len);
		bool				extractLine(std::string &line);
		void				queueMessage(const std::string &data);
		bool				hasPendingWrite() const;
		int					getFd() const;
		const std::string	&getNickname() const;
		void				setNickname(const std::string &nickname);
		const std::string	&getUsername() const;
		void				setUsername(const std::string &username);
		bool				isRegistered() const;
		void				setRegistered(bool registered);
		const std::string	&getRealname() const;
		void				setRealname(const std::string &realname);
		const std::string	&getHostname() const;
		void				setHostname(const std::string &hostname);
		bool				hasPassOk() const;
		void				setPassOk(bool ok);
		void				requestClose();
		bool				wantClose() const;
		std::string			prefix() const;
		void				setQuitReason(const std::string &reason);
		const std::string	&getQuitReason() const;
		void				joinChannel(const std::string &channelName);
		void				leaveChannel(const std::string &channelName);
		bool				isInChannel(const std::string &channelName) const;
		const std::set<std::string>	&getChannels() const;
};

#endif
