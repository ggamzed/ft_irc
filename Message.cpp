#include "Message.hpp"
#include <cctype>

Message	parseMessage(const std::string &raw)
{
	Message	msg;
	size_t	i;
	size_t	len;
	size_t	start;
	size_t	end;
	size_t	k;

	i = 0;
	len = raw.size();
	while (i < len && raw[i] == ' ')
		i++;
	if (i < len && raw[i] == ':')
	{
		start = i + 1;
		end = raw.find(' ', start);
		if (end == std::string::npos)
		{
			msg.prefix = raw.substr(start);
			i = len;
		}
		else
		{
			msg.prefix = raw.substr(start, end - start);
			i = end;
		}
		while (i < len && raw[i] == ' ')
			i++;
	}
	start = i;
	end = raw.find(' ', start);
	if (end == std::string::npos)
	{
		msg.command = raw.substr(start);
		i = len;
	}
	else
	{
		msg.command = raw.substr(start, end - start);
		i = end;
	}

	k = 0;
	while (k < msg.command.size())
	{
		msg.command[k] = static_cast<char>(std::toupper(static_cast<unsigned char>(msg.command[k])));
		k++;
	}

	while (i < len && raw[i] == ' ')
		i++;

	while (i < len)
	{
		if (raw[i] == ':')
		{
			msg.params.push_back(raw.substr(i + 1));
			break ;
		}

		start = i;
		end = raw.find(' ', start);
		if (end == std::string::npos)
		{
			msg.params.push_back(raw.substr(start));
			break ;
		}
		msg.params.push_back(raw.substr(start, end - start));
		i = end;
		while (i < len && raw[i] == ' ')
			i++;
	}
	return (msg);
}
