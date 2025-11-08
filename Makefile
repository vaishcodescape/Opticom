# =======================================
# Opticom Chat Server & Client Makefile
# =======================================

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread

SERVER_TARGET = opticom
CLIENT_TARGET = client
SERVER_SOURCE = opticom.cpp
CLIENT_SOURCE = client.cpp
HISTORY_DIR = history

# -------------------------------
# Default target: Build both
# -------------------------------
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# -------------------------------
# Build the server executable
# -------------------------------
$(SERVER_TARGET): $(SERVER_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SERVER_SOURCE)

# -------------------------------
# Build the client executable
# -------------------------------
$(CLIENT_TARGET): $(CLIENT_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SOURCE)

# -------------------------------
# Run server on default port (8080)
# Automatically creates history folder/files
# -------------------------------
run: $(SERVER_TARGET)
	mkdir -p $(HISTORY_DIR)
	touch $(HISTORY_DIR)/history_general.txt
	./$(SERVER_TARGET)

# -------------------------------
# Run server on custom port
# Usage: make run-port PORT=9090
# -------------------------------
run-port: $(SERVER_TARGET)
	mkdir -p $(HISTORY_DIR)
	touch $(HISTORY_DIR)/history_general.txt
	./$(SERVER_TARGET) $(PORT)

# -------------------------------
# Run client
# Usage: make run-client IP=127.0.0.1 PORT=8080
# -------------------------------
run-client: $(CLIENT_TARGET)
	@if [ -z "$(PORT)" ]; then echo "Usage: make run-client IP=127.0.0.1 PORT=8080"; exit 1; fi
	./$(CLIENT_TARGET) $(IP) $(PORT)

# -------------------------------
# Clean build artifacts and history
# -------------------------------
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
	rm -rf $(HISTORY_DIR)

# -------------------------------
# Install/Uninstall commands
# -------------------------------
install: $(SERVER_TARGET) $(CLIENT_TARGET)
	sudo cp $(SERVER_TARGET) /usr/local/bin/
	sudo cp $(CLIENT_TARGET) /usr/local/bin/

uninstall:
	sudo rm -f /usr/local/bin/$(SERVER_TARGET)
	sudo rm -f /usr/local/bin/$(CLIENT_TARGET)

# -------------------------------
# Debug build
# -------------------------------
debug: CXXFLAGS += -g -DDEBUG
debug: $(SERVER_TARGET) $(CLIENT_TARGET)

# -------------------------------
# Help
# -------------------------------
help:
	@echo "Available targets:"
	@echo "  all         - Build both server and client (default)"
	@echo "  run         - Run server on default port (8080)"
	@echo "  run-port    - Run server on custom port (make run-port PORT=9090)"
	@echo "  run-client  - Run client (make run-client IP=127.0.0.1 PORT=8080)"
	@echo "  clean       - Remove build artifacts & history files"
	@echo "  install     - Install binaries to /usr/local/bin"
	@echo "  uninstall   - Remove binaries from /usr/local/bin"
	@echo "  debug       - Build with debug symbols"
	@echo "  help        - Show this help message"

.PHONY: all clean install uninstall run run-port run-client debug help
