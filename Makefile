# Makefile for Opticom Chat Server
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
SERVER_TARGET = opticom
CLIENT_TARGET = client
SERVER_SOURCE = opticom.cpp
CLIENT_SOURCE = client.cpp

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Build the server executable
$(SERVER_TARGET): $(SERVER_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SERVER_SOURCE)

# Build the client executable
$(CLIENT_TARGET): $(CLIENT_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SOURCE)

# Clean build artifacts
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

# Install (copy to /usr/local/bin)
install: $(SERVER_TARGET) $(CLIENT_TARGET)
	sudo cp $(SERVER_TARGET) /usr/local/bin/
	sudo cp $(CLIENT_TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(SERVER_TARGET)
	sudo rm -f /usr/local/bin/$(CLIENT_TARGET)

# Run the server on default port
run: $(SERVER_TARGET)
	./$(SERVER_TARGET)

# Run the server on custom port (usage: make run-port PORT=9090)
run-port: $(SERVER_TARGET)
	./$(SERVER_TARGET) $(PORT)

# Run client (usage: make run-client [IP=127.0.0.1] [PORT=8080])
run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET) $(IP) $(PORT)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(SERVER_TARGET) $(CLIENT_TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build both server and client (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  run        - Run server on default port (8080)"
	@echo "  run-port   - Run server on custom port (make run-port PORT=9090)"
	@echo "  run-client - Run client (make run-client [IP=127.0.0.1] [PORT=8080])"
	@echo "  debug      - Build with debug symbols"
	@echo "  help       - Show this help message"

.PHONY: all clean install uninstall run run-port run-client debug help
