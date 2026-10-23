*This project has been created as part of the 42 curriculum by liovino and chbudila.*

# ft_irc

## Description

**ft_irc** is an IRC (Internet Relay Chat) server written from scratch in C++98.
The goal of the project is to understand how a real network service works under the hood:
TCP sockets, non-blocking I/O, multiplexing many clients in a single process, and
implementing a text-based protocol defined by RFCs.

The server (`ircserv`) accepts connections from standard IRC clients (e.g. irssi, HexChat, WeeChat)
or from raw tools like `nc`, authenticates them with a connection password, and lets them
chat privately or in channels with operator privileges.

### Features

- Single `poll()` loop handling all I/O, with every socket set to non-blocking mode (no forking, no threads)
- Connection password and client registration (`PASS`, `NICK`, `USER`)
- Private messages between users and messages broadcast to channels (`PRIVMSG`)
- Channels: `JOIN`, `TOPIC`, `INVITE`, `KICK`
- Channel modes via `MODE`:
  | Flag | Meaning |
  |------|---------|
  | `i`  | Invite-only channel |
  | `t`  | Only operators can change the topic |
  | `k`  | Channel key (password) |
  | `o`  | Give / take channel operator privilege |
  | `l`  | User limit on the channel |
- Keep-alive with `PING`
- Numeric error replies (401, 403, 441, 461, 481, 482, 501, …) following RFC 2812

### Technical choices

- **`poll()` multiplexing**: one `std::vector<pollfd>` holds the listening socket plus all clients; reads and writes only happen when `poll()` reports the fd as ready.
- **Per-client buffers**: incoming data is buffered until a full `\r\n`-terminated command arrives (handles partial packets); outgoing replies are queued and flushed when the socket is writable.
- **Command dispatch with member-function pointers**: `IrcServer` maps command names (`"JOIN"`, `"KICK"`, …) to handler methods, and `Channel` does the same for `MODE` flags, so adding a command is just one new handler plus one map entry.
- **Project layout**:
  ```
  includes/   IrcServer.hpp, Client.hpp, Channel.hpp
  srcs/       main.cpp, IrcServer*.cpp (networking + commands), Client.cpp, Channel*.cpp (channel state + modes)
  ```

## Instructions

### Requirements

- A C++ compiler supporting C++98 (`c++`, `g++` or `clang++`)
- `make`
- Linux or macOS

### Compilation

```sh
make        # builds ./ircserv
make clean  # removes object files
make fclean # removes object files and the binary
make re     # full rebuild
```

The code is compiled with `-Wall -Wextra -Werror -std=c++98`.

### Execution

```sh
./ircserv <port> <password>
```

- `port`: the TCP port the server listens on (1–65535)
- `password`: the password every client must send with `PASS` before registering

### Connecting

With an IRC client (example: irssi):

```sh
weechat
/server add <name> <pass>
/connect <name>
```

With `nc` (commands must end with `\r\n`; `-C` sends CRLF):

```sh
nc -C 127.0.0.1 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :hello everyone
MODE #general +k secret
TOPIC #general :ft_irc test channel
```

## Resources

### References

- [RFC 2812 – IRC: Client Protocol](https://www.kvirc.net/doc/doc_rfc2812.html)
- [mIRC Help](https://www.mirc.com/help/html/index.html?basic_irc_commands.html)
- [WeeChat User guide](https://weechat.org/files/doc/weechat/stable/weechat_quickstart.en.html#start)
- `man`
- `bircd/`: small reference IRC server in C provided with the subject

### Use of AI

AI tools were used as a support tool for:

- **Understanding the protocol**: clarifying parts of the RFCs, numeric reply formats and edge cases of some commands.
- **Debugging**: explaining compiler errors and reviewing logic in specific handlers.
- **Documentation**: drafting this README, which was then reviewed and edited by the authors.

All design decisions and the server code were written and reviewed by the team members.
