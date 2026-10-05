*This project has been created as part of the 42 curriculum by _merozcan, egokce, gdemirci_.*

# ft_irc

A lightweight **IRC server** written in **C++98**, compatible with standard IRC clients. It serves multiple clients at once over TCP using a single non-blocking `poll()` event loop — no threads, no forking.

## Description

ft_irc implements the core of the IRC protocol. Clients connect over TCP, authenticate with a server password, register a nickname and user identity, join channels, and exchange messages in real time — either directly with another user or through a channel.

The server is fully non-blocking: every socket is watched by a single `poll()` call, and a read or write happens only when that socket is ready. As a result the server never blocks on a slow client and stays responsive to everyone at the same time. Incoming data is buffered and reassembled, so a command split across several packets is handled correctly.

**Features**

- Password authentication (`PASS`), nickname (`NICK`) and user registration (`USER`)
- Private messages and notices (`PRIVMSG`, `NOTICE`) to both users and channels
- Channel operations: `JOIN`, `PART`, `TOPIC`, `INVITE`, `KICK`
- Channel modes: `i` (invite-only), `t` (operator-only topic), `k` (key/password), `o` (operator privilege), `l` (user limit)
- Operator and regular-user roles
- Connection keep-alive (`PING` / `PONG`) and clean disconnect (`QUIT`)
- Standard IRC numeric replies

## Instructions

**Requirements:** a C++98 compiler (`c++`) and `make`. Builds on Linux and macOS.

**Build**

```
make          # build the ircserv executable
make clean    # remove object files
make fclean   # remove object files and the executable
make re       # rebuild from scratch
```

**Run**

```
./ircserv <port> <password>
```

- `<port>` — TCP port the server listens on (1–65535)
- `<password>` — the password every client must send to connect

Example:

```
./ircserv 6667 mypassword
```

**Connect**

Point any standard IRC client (irssi, WeeChat, HexChat) at the server's host and port and provide the server password. You can also test manually with netcat:

```
nc localhost 6667
```

Then register and start chatting:

```
PASS mypassword
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :hello everyone
```

## Team

- **merozcan** — authentication and user commands (registration, messaging)
- **egokce** — non-blocking socket and `poll()` infrastructure
- **gdemirci** — channel and operator logic

## Resources

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- The 42 `ft_irc` subject
- Manual pages for the socket API: `socket`, `setsockopt`, `bind`, `listen`, `accept`, `poll`, `recv`, `send`, `fcntl`

**Use of AI**

AI tools were used as a support resource during this project for code review, debugging assistance, clarifying protocol details, and improving the clarity of documentation.