# Opticom - Multi-Client Chat Server

Opticom is a high-performance, multi-client chat server built in C++ using low-latency and fast-optimized methods. This project was developed as part of the IT-227 Computer System Programming Course in the B.Tech ICT Branch.

## Features

* **Multi-client support**: Handle multiple clients simultaneously using threading
* **Real-time messaging**: Broadcast messages to all connected clients
* **Thread-safe**: Uses mutex locks for safe concurrent access
* **Cross-platform**: Works on Linux, macOS, and other Unix-like systems
* **Signal handling**: Graceful shutdown with Ctrl+C
* **Configurable port**: Run on any available port
* **Simple client**: Included test client for easy testing

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

* **ChatServer class**: Main server implementation
* **Socket management**: TCP socket creation and binding
* **Client handling**: Thread-per-client architecture
* **Message broadcasting**: Send messages to all connected clients
* **Client management**: Add/remove clients safely

### Client Components

* **ChatClient class**: Simple client implementation
* **Connection management**: Connect to server and maintain connection
* **Message handling**: Send and receive messages
* **Threading**: Separate thread for receiving messages

### Thread Safety

* Uses `std::mutex` to protect shared client list
* `lock_guard` for automatic mutex management
* Thread-safe client addition/removal

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

* Uses non-blocking socket operations where possible
* Efficient message broadcasting
* Minimal memory allocation
* Thread-per-client model for simplicity

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

✅ That’s the full, ready-to-use version — no formatting or syntax issues, and it’ll render beautifully on GitHub.

