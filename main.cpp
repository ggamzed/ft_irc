#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <stdexcept>

static bool	parsePort(const std::string &s, int &outPort)
{
	size_t	i;
	long	value;

	if (s.empty())
		return (false);
	i = 0;
	while (i < s.size())
	{
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			return (false);
		i++;
	}
	value = std::atol(s.c_str());
	if (value < 1 || value > 65535)
		return (false);
	outPort = static_cast<int>(value);
	return (true);
}

int	main(int ac, char **av)
{
	int			port;
	std::string	password;

	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return (1);
	}
	if (!parsePort(av[1], port))
	{
		std::cerr << "Invalid port: " << av[1] << std::endl;
		return (1);
	}
	password = av[2];
	if (password.empty())
	{
		std::cerr << "Password cannot be empty" << std::endl;
		return (1);
	}

	try
	{
		Server server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
