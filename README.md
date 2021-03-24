# cpp-tcp-des-chat

A TCP chat program that uses DES key to encrypt communication content and RSA public key to encrypt DES key, supporting communication between one server and multiple clients. Boost C++ libraries is also used to implement RSA large number operations.

## Dependencies

- Boost v1.76.0
- Linux environment is required

## Usage

### Command line options

- `-o` or ` --option` : Run as server(s) or client(c) mode. **(Required)**
- `-a` or ` --address` : The IP address of the server. (optional)
- `-p` or ` --port` : The port of the server. (optional)
- `-v` or `--version`: Show the version number.
- `-h` or `--help` : Show the help message.

### Epoll version

#### Compile

```
make epoll
```

#### Run server

```
./epoll_chat -o s [-a <listen-ip>] [-p <listen-port>]
```

#### Run client

```
./epoll_chat -o c [-a <remote-ip>] [-p <remote-port>]
```

## TODO

- [ ] select/poll versions to be implemented
- [ ] Performance/stress test?


