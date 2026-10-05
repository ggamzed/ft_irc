#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const size_t READ_CHUNK = 4096;

volatile sig_atomic_t	g_stop = 0;

void	handleStopSignal(int)
{
	g_stop = 1;
}

Server::Server(int port, const std::string &password) : listenFd(-1), port(port), password(password)
{
	this->setupSocket();
}

Server::~Server()
{
	std::map<int, Client *>::iterator it;

	it = clients.begin();

	while (it != clients.end())
	{
		delete it->second;
		it++;
	}

	std::map<std::string, Channel *>::iterator cit;

	cit = channels.begin();
	while (cit != channels.end())
	{
		delete cit->second;
		cit++;
	}

	if (this->listenFd >= 0)
		close(this->listenFd);
}

void	Server::setupSocket()
{
	int			opt;
	sockaddr_in	addr;

	this->listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->listenFd < 0)
		throw std::runtime_error(std::string("socket() failed: ") + strerror(errno));

	opt = 1;
	if (setsockopt(this->listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error(std::string("setsockopt() failed: ") + strerror(errno));

	if (fcntl(this->listenFd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error(std::string("fcntl() failed: ") + strerror(errno));

	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(static_cast<unsigned short>(port));

	if (bind(this->listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		throw std::runtime_error(std::string("bind() failed: ") + strerror(errno));

	if (listen(this->listenFd, SOMAXCONN) < 0)
		throw std::runtime_error(std::string("listen() failed: ") + strerror(errno));

	pollfd listenPfd;
	listenPfd.fd = this->listenFd;
	listenPfd.events = POLLIN;
	listenPfd.revents = 0;
	pollfds.push_back(listenPfd);
}

void	Server::updatePollEvents()
{
	size_t	i;

	i = 0;
	while (i < pollfds.size())
	{
		if (pollfds[i].fd == this->listenFd)
		{
			i++;
			continue ;
		}

		std::map<int, Client *>::iterator it = clients.find(pollfds[i].fd);

		if (it == clients.end())
		{
			i++;
			continue ;
		}

		pollfds[i].events = POLLIN;
		if (it->second->hasPendingWrite())
			pollfds[i].events |= POLLOUT;

		i++;
	}
}

void	Server::setupSignals()
{
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handleStopSignal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);
}

void	Server::processReadyFds(std::vector<int> &toClose)
{
	size_t	i;
	short	revents;
	bool	alive;
	int		fd;

	i = 0;
	while (i < pollfds.size())
	{
		revents = pollfds[i].revents;
		if (revents == 0)
		{
			i++;
			continue ;
		}

		fd = pollfds[i].fd;

		if (fd == this->listenFd)
		{
			if (revents & POLLIN)
				this->acceptClient();
			i++;
			continue ;
		}

		alive = true;
		if (revents & (POLLHUP | POLLERR | POLLNVAL))
			alive = false;
		else
		{
			if (revents & POLLIN)
				alive = this->handleClientRead(fd);
			if (alive && (revents & POLLOUT))
				alive = this->handleClientWrite(fd);
		}

		if (!alive)
			toClose.push_back(fd);
		i++;
	}
}

void	Server::run()
{
	int	ready;

	this->setupSignals();

	std::cout << "Listening on port " << port << "..." << std::endl;
	while (!g_stop)
	{
		std::vector<int>	toClose;
		size_t				r;

		this->updatePollEvents();

		ready = poll(&pollfds[0], pollfds.size(), -1);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue ;
			std::cerr << "poll() failed: " << strerror(errno) << std::endl;
			break ;
		}

		this->processReadyFds(toClose);

		r = 0;
		while (r < toClose.size())
		{
			this->closeClient(toClose[r]);
			r++;
		}
	}
	std::cout << "Shutting down, closing " << clients.size() + 1 << " fd(s)..." << std::endl;
}

void	Server::acceptClient()
{
	sockaddr_in	addr;
	socklen_t	len;
	int			fd;

	len = sizeof(addr);

	fd = accept(this->listenFd, (struct sockaddr *)&addr, &len);
	if (fd < 0)
		return ;

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "fcntl() failed: " << strerror(errno) << std::endl;
		close(fd);
		return ;
	}

	clients[fd] = new Client(fd);
	clients[fd]->setHostname(inet_ntoa(addr.sin_addr));

	pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	pollfds.push_back(pfd);

	std::cout << "Client connected: " << inet_ntoa(addr.sin_addr) << " (fd " << fd << ")" << std::endl;
}

bool	Server::handleClientRead(int fd)
{
	std::map<int, Client *>::iterator	it;
	Client 								*client;
	std::string							line;
	ssize_t								n;

	it = clients.find(fd);
	if (it == clients.end())
		return (false);

	client = it->second;

	char buf[READ_CHUNK];

	n = recv(fd, buf, sizeof(buf), 0);

	if (n > 0)
	{
		client->appendToReadBuffer(buf, static_cast<size_t>(n));

		while (!client->wantClose() && client->extractLine(line))
		{
			Message msg = parseMessage(line);
			this->processCommand(*client, msg);
		}

		if (client->wantClose() && !client->hasPendingWrite())
			return (false);
		return (true);
	}

	if (n == 0)
		std::cout << "Client disconnected (fd " << fd << ")" << std::endl;
	return (false);
}

bool	Server::handleClientWrite(int fd)
{
	std::map<int, Client *>::iterator	it;
	Client								*client;
	ssize_t								n;

	it = clients.find(fd);
	if (it == clients.end())
		return (false);
	client = it->second;

	const std::string &out = client->getWriteBuffer();
	if (out.empty())
		return (true);

	n = send(fd, out.data(), out.size(), 0);
	if (n <= 0)
		return (false);

	client->consumeWriteBuffer(static_cast<size_t>(n));

	if (client->wantClose() && !client->hasPendingWrite())
		return (false);
	return (true);
}

void	Server::closeClient(int fd)
{
	size_t	i;

	std::map<int, Client *>::iterator it = clients.find(fd);
	if (it != clients.end())
	{
		removeClientFromAllChannels(*it->second, it->second->getQuitReason());
		delete it->second;
		clients.erase(it);
	}
	i = 0;
	while (i < pollfds.size())
	{
		if (pollfds[i].fd == fd)
		{
			pollfds.erase(pollfds.begin() + i);
			break ;
		}
		i++;
	}
}
