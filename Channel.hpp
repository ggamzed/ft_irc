#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class	Client;

class	Channel
{
	private:
		std::string			name;
		std::string			topic;
		bool				topicSet;
		std::string			key;
		bool				inviteOnly;
		bool				topicRestricted;
		size_t				userLimit;
		std::set<Client *>	members;
		std::set<Client *>	operators;
		std::set<Client *>	invited;

	public:
		explicit Channel(const std::string &name);
		~Channel();

		const std::string	&getName() const;

		void				addMember(Client *client);
		void				removeMember(Client *client);
		bool				hasMember(Client *client) const;
		bool				isEmpty() const;
		size_t				getMemberCount() const;
		const std::set<Client *>	&getMembers() const;
		void				addOperator(Client *client);
		void				removeOperator(Client *client);
		bool				isOperator(Client *client) const;
		void				setTopic(const std::string &topic);
		const std::string	&getTopic() const;
		bool				hasTopic() const;
		void				setKey(const std::string &key);
		void				removeKey();
		bool				hasKey() const;
		const std::string	&getKey() const;
		void				setInviteOnly(bool value);
		bool				isInviteOnly() const;
		void				setTopicRestricted(bool value);
		bool				isTopicRestricted() const;
		void				setUserLimit(size_t limit);
		void				removeUserLimit();
		bool				hasUserLimit() const;
		size_t				getUserLimit() const;
		void				invite(Client *client);
		void				uninvite(Client *client);
		bool				isInvited(Client *client) const;
		std::string			modeString() const;
};

#endif
