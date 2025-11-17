# Opticom - Multi-Client Chat Server

Opticom is a high-performance, multi-client chat server built in C++ using low-latency and fast-optimized methods. This project was developed as part of the IT-227 Computer System Programming Course in the B.Tech ICT Branch.

## Features

### Core Features
* **Multi-client support**: Handle multiple clients simultaneously using threading
* **Real-time messaging**: Broadcast messages to all connected clients
* **Thread-safe**: Uses mutex locks for safe concurrent access
* **Cross-platform**: Works on Linux, macOS, and other Unix-like systems
* **Signal handling**: Graceful shutdown with Ctrl+C
* **Configurable port**: Run on any available port
* **Enhanced client UI**: Colorful terminal interface with better message formatting

### Security Features
* **Message Encryption**: XOR-based encryption for secure communication between clients and server
* **User Blocking**: Block/unblock users to prevent receiving messages from them
* **Privacy Protection**: Blocked users cannot send private messages to you

### Chat Features
* **Rooms**: Create and join different chat rooms
* **Private Messages**: Send direct messages to specific users
* **Message History**: Persistent chat history per room saved to files
* **Pinned Messages**: Pin important messages to room boards
* **Rate Limiting**: Built-in spam protection (3 messages per second)
* **Room Slowmode**: Admin-controlled message cooldown per room
* **User List**: View online users and their current rooms

### Server Administration
* **Admin Console**: Built-in server console with commands
* **User Management**: Kick users, broadcast server messages
* **Room Management**: Set slowmode for specific rooms
* **Colorful Logging**: Server console displays colored logs for easy monitoring

## Architecture

The server uses:

* **TCP sockets** for reliable communication
* **POSIX threads** for handling multiple clients
* **Mutex locks** for thread-safe client management
* **Signal handling** for graceful shutdown

## Building

### Prerequisites

* C++11 compatible compiler (g++, clang++)
* POSIX-compliant system (Linux, macOS, etc.)
* Make utility

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
4. Type messages in any client — they will be broadcast to all others

## Example Usage

### Basic Chat Session

```bash
# Terminal 1: Start server
./opticom

# Terminal 2: Connect as Alice
./client 8080
# Enter username: Alice
# Type: Hello everyone!

# Terminal 3: Connect as Bob  
./client 8080
# Enter username: Bob
# You'll see: [12:34:56] Alice: Hello everyone!
# Type: Hi Alice!
```

### Using Security Features

```bash
# Block a user
/block SpamBot

# Check your block list
/blocklist

# Unblock later
/unblock SpamBot

# All messages are automatically encrypted in transit
```

### Using Rooms

```bash
# Join a room
/join gaming

# Pin a message
/pin Check out the new update!

# View pinned messages
/pins

# List all rooms
/rooms
```

## Client Commands

### Basic Commands
* `/help` - Show all available commands
* `/list` - List all online users and their rooms
* `/rooms` - Show all active rooms with user counts
* `/quit` - Disconnect from the server

### Room Commands
* `/join <room>` - Join or create a room (e.g., `/join gaming`)
* `/pin <message>` - Pin a message to the current room's board
* `/pins` - View all pinned messages for the current room
* `/unpin <index>` - Remove a pinned message by its number

### Messaging Commands
* `/pm <username> <message>` - Send a private message (e.g., `/pm Alice Hello!`)
* `message` - Just type normally to send to your current room

### Security Commands
* `/block <username>` - Block messages from a user
* `/unblock <username>` - Unblock a previously blocked user
* `/blocklist` - Show your list of blocked users

### UI Commands
* `/clear` - Clear the client screen (local only)

## Server Admin Commands

While the server is running, you can type these commands in the server console:

* `list` - Show all connected users with their IPs and rooms
* `kick <username>` - Disconnect and kick a user
* `say <message>` - Broadcast a message to all users in general room
* `slowmode <room> <seconds>` - Set slowmode for a room (e.g., `slowmode general 10`)
* `help` - Show admin commands

## Security & Encryption

### Message Encryption
All messages sent between clients and the server are encrypted using XOR-based encryption. This provides:
- **Basic protection** against casual packet sniffing
- **Obfuscation** of message content in transit
- **Shared key** encryption for simplicity

**Note**: This is a simple encryption scheme suitable for educational purposes. For production use, consider implementing TLS/SSL for stronger security.

### User Blocking
The blocking system allows you to:
- Block users who are bothering you
- Prevent blocked users from sending you messages
- Block private messages from blocked users
- View and manage your block list

**How it works:**
1. Use `/block <username>` to block someone
2. Blocked users' messages won't appear in your chat
3. They cannot send you private messages
4. Use `/unblock <username>` to unblock them
5. Use `/blocklist` to see who you've blocked

## Enhanced UI Features

### Client Interface
The client features a colorful, modern terminal UI with:
- **Color-coded messages**: Different colors for system messages, PMs, errors
- **Visual indicators**: Icons for joins (→), leaves (←), PMs (✉), warnings (⚠)
- **Better formatting**: Clean message bubbles and organized display
- **Encrypted badge**: Displays encryption status in banner

### Server Console
The server console includes:
- **Startup banner**: Beautiful ASCII art banner on server start
- **Colorful logs**: 
  - 💬 Blue for chat messages
  - → Green for user joins
  - ← Red for user disconnects
  - ⚠ Yellow/red for warnings and errors
- **Formatted boxes**: Clean boxes for user lists and help commands

## Project Structure

```
Opticom/
├── opticom.cpp      # Main server implementation
├── client.cpp       # Enhanced client with encryption and UI
├── Makefile         # Build configuration
├── README.md        # This file
├── LICENSE          # License file
└── history/         # Auto-generated directory for chat logs
    ├── history_<room>.txt   # Chat history per room
    └── pins_<room>.txt      # Pinned messages per room
```

## Technical Details

### Server Components

* **ChatServer class**: Main server implementation
* **Socket management**: TCP socket creation and binding
* **Client handling**: Thread-per-client architecture
* **Message broadcasting**: Send messages to all connected clients with blocking support
* **Client management**: Add/remove clients safely
* **Encryption module**: XOR-based encryption for all messages
* **Blocking system**: Per-user block lists stored in client info
* **Room management**: Support for multiple chat rooms with history
* **Rate limiting**: Per-client message rate tracking

### Client Components

* **Enhanced client**: Modern terminal UI with colors and formatting
* **Connection management**: Connect to server and maintain connection
* **Message handling**: Encrypt/decrypt messages automatically
* **Threading**: Separate thread for receiving messages
* **UI formatting**: Color-coded messages based on type and content
* **Local commands**: `/clear` for screen management

### Security Implementation

* **Encryption**: XOR-based encryption with shared key
  - All messages encrypted before transmission
  - Automatic decryption on reception
  - Symmetric key encryption (encrypt = decrypt for XOR)
* **Blocking**: Server-side filtering
  - Block list stored per client
  - Filtered during broadcast and private messages
  - Prevents blocked users from contacting you

### Thread Safety

* Uses `std::mutex` to protect shared client list
* `lock_guard` for automatic mutex management
* Thread-safe client addition/removal
* Safe access to block lists during message broadcasting

## Installation

```bash
# Install to system
make install

# Uninstall
make uninstall
```

## Signal Handling

The server handles SIGINT (Ctrl+C) and SIGTERM signals for graceful shutdown:

* Closes all client connections
* Cleans up resources
* Exits cleanly

## Error Handling

* Socket creation and binding errors
* Client connection failures
* Message sending failures
* Graceful client disconnection handling

## Performance Considerations

* Efficient message broadcasting with filtering
* Minimal memory allocation
* Thread-per-client model for simplicity
* Encryption overhead is minimal (XOR operations)
* Block list checks are O(n) but efficient for typical use
* Message history saved asynchronously to disk

---

## 🪟 Running on Windows (via Remote SSH or WSL)

Opticom is designed for Unix-like systems (Linux, macOS).
If you're on **Windows**, you can still build, run, and test it easily using one of the following methods.

### Option 1: Using VS Code Remote - SSH (Recommended)

1. **Install** [Visual Studio Code](https://code.visualstudio.com/) and the **Remote - SSH** extension (blue `><` icon).
2. **Set up a Linux VM** (Ubuntu recommended) in [VirtualBox](https://www.virtualbox.org/) or [VMware Workstation Player](https://www.vmware.com/products/workstation-player.html).
3. Inside the VM:

   ```bash
   sudo apt update
   sudo apt install openssh-server -y
   sudo systemctl enable ssh
   sudo systemctl start ssh
   ```
4. Find your VM’s IP:

   ```bash
   ip a
   ```

   Look for a line like `inet 192.168.x.x` under `eth0`.
5. On Windows, open VS Code → **Ctrl + Shift + P** → **Remote-SSH: Connect to Host...**

   ```
   ssh username@192.168.x.x
   ```
6. Once connected, open the Opticom folder and build normally:

   ```bash
   make
   ./opticom 8080
   ./client localhost 8080
   ```

### Option 2: Using Windows Subsystem for Linux (WSL)

1. Enable WSL and install Ubuntu:

   ```powershell
   wsl --install
   ```
2. Launch Ubuntu:

   ```bash
   sudo apt update
   sudo apt install g++ make -y
   ```
3. Clone and build:

   ```bash
   git clone https://github.com/vaishcodescape/Opticom.git
   cd Opticom
   make
   ./opticom
   ./client
   ```
4. WSL behaves like native Linux — no VM setup required.

### Option 3: (Optional) Compile with MinGW or Cygwin

You can compile directly on Windows with MinGW or Cygwin, but you’ll need to:

* Replace Unix headers (`sys/socket.h`, `netinet/in.h`, etc.) with Windows equivalents (`winsock2.h`, `ws2tcpip.h`).
* Initialize Winsock with `WSAStartup()`.
  However, using a Unix environment (VM or WSL) is **strongly recommended**.

---

## MIT License

See MIT LICENSE file for details.

## Course Information

This project was developed as part of IT-227 Computer System Programming Course in the B.Tech ICT Branch, focusing on:

* Socket programming
* Multi-threading
* System programming concepts
* Network communication protocols

---
