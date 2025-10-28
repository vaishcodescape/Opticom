# Opticom - Multi-Client Chat Server

Opticom is a high-performance, multi-client chat server built in C++ using low-latency and fast-optimized methods. This project was developed as part of the IT-227 Computer System Programming Course in the B.Tech ICT Branch.

## Features

- **Multi-client support**: Handle multiple clients simultaneously using threading
- **Real-time messaging**: Broadcast messages to all connected clients
- **Thread-safe**: Uses mutex locks for safe concurrent access
- **Cross-platform**: Works on Linux, macOS, and other Unix-like systems
- **Signal handling**: Graceful shutdown with Ctrl+C
- **Configurable port**: Run on any available port
- **Simple client**: Included test client for easy testing

## Architecture

The server uses:
- **TCP sockets** for reliable communication
- **POSIX threads** for handling multiple clients
- **Mutex locks** for thread-safe client management
- **Signal handling** for graceful shutdown

## Building

### Prerequisites
- C++11 compatible compiler (g++, clang++)
- POSIX-compliant system (Linux, macOS, etc.)
- Make utility

### Compilation
```bash
# Build both server and client
make

# Or build individually
make opticom    # Server only
make client     # Client only

# Debug build
make debug

# Clean build artifacts
make clean
```

## Usage

### Starting the Server
```bash
# Run on default port (8080)
./opticom

# Run on custom port
./opticom 9090

# Or using make
make run
make run-port PORT=9090
```

### Connecting Clients
```bash
# Connect to localhost on default port
./client

# Connect to specific server and port
./client 192.168.1.100 8080

# Or using make
make run-client
make run-client IP=192.168.1.100 PORT=8080
```

### Testing Multiple Clients
1. Start the server: `./opticom`
2. Open multiple terminal windows
3. Run clients in each terminal: `./client`
4. Type messages in any client - they will be broadcast to all others

## Project Structure

```
Opticom/
├── opticom.cpp      # Main server implementation
├── client.cpp       # Test client implementation
├── Makefile         # Build configuration
├── README.md        # This file
└── LICENSE          # License file
```

## Technical Details

### Server Components
- **ChatServer class**: Main server implementation
- **Socket management**: TCP socket creation and binding
- **Client handling**: Thread-per-client architecture
- **Message broadcasting**: Send messages to all connected clients
- **Client management**: Add/remove clients safely

### Client Components
- **ChatClient class**: Simple client implementation
- **Connection management**: Connect to server and maintain connection
- **Message handling**: Send and receive messages
- **Threading**: Separate thread for receiving messages

### Thread Safety
- Uses `std::mutex` to protect shared client list
- `lock_guard` for automatic mutex management
- Thread-safe client addition/removal

## Installation

```bash
# Install to system
make install

# Uninstall
make uninstall
```

## Signal Handling

The server handles SIGINT (Ctrl+C) and SIGTERM signals for graceful shutdown:
- Closes all client connections
- Cleans up resources
- Exits cleanly

## Error Handling

- Socket creation and binding errors
- Client connection failures
- Message sending failures
- Graceful client disconnection handling

## Performance Considerations

- Uses non-blocking socket operations where possible
- Efficient message broadcasting
- Minimal memory allocation
- Thread-per-client model for simplicity

## MIT License

See MIT LICENSE file for details.

## Course Information

This project was developed as part of IT-227 Computer System Programming Course in the B.Tech ICT Branch, focusing on:
- Socket programming
- Multi-threading
- System programming concepts
- Network communication protocols
