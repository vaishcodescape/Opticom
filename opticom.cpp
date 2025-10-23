// Opticom - Multi-Client Chat Server
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>

using namespace std;

class ChatServer {
private:
    int serverSocket;
    int port;
    vector<int> clientSockets;
    mutex clientsMutex;
    bool running;

public:
    ChatServer(int port) : port(port), running(false) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            throw runtime_error("Failed to create socket");
        }
        
        // Allow socket reuse
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw runtime_error("Failed to set socket options");
        }
    }

    ~ChatServer() {
        stop();
        if (serverSocket >= 0) {
            close(serverSocket);
        }
    }

    void start() {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (::bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            throw runtime_error("Failed to bind socket to port " + to_string(port));
        }

        if (listen(serverSocket, 10) < 0) {
            throw runtime_error("Failed to listen on socket");
        }

        running = true;
        cout << "Opticom Chat Server started on port " << port << endl;
        cout << "Waiting for clients to connect..." << endl;

        while (running) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket < 0) {
                if (running) {
                    cerr << "Failed to accept client connection" << endl;
                }
                continue;
            }

            cout << "New client connected from " << inet_ntoa(clientAddr.sin_addr) 
                 << ":" << ntohs(clientAddr.sin_port) << endl;

            // Add client to list
            {
                lock_guard<mutex> lock(clientsMutex);
                clientSockets.push_back(clientSocket);
            }

            // Handle client in separate thread
            thread clientThread(&ChatServer::handleClient, this, clientSocket);
            clientThread.detach();
        }
    }

    void stop() {
        running = false;
        if (serverSocket >= 0) {
            close(serverSocket);
        }
        
        // Close all client connections
        lock_guard<mutex> lock(clientsMutex);
        for (int clientSocket : clientSockets) {
            close(clientSocket);
        }
        clientSockets.clear();
    }

private:
    void handleClient(int clientSocket) {
        char buffer[1024];
        
        while (running) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                // Client disconnected
                cout << "Client disconnected" << endl;
                removeClient(clientSocket);
                close(clientSocket);
                break;
            }

            string message(buffer);
            cout << "Received: " << message << endl;

            // Broadcast message to all other clients
            broadcastMessage(message, clientSocket);
        }
    }

    void broadcastMessage(const string& message, int senderSocket) {
        lock_guard<mutex> lock(clientsMutex);
        
        for (int clientSocket : clientSockets) {
            if (clientSocket != senderSocket) {
                if (send(clientSocket, message.c_str(), message.length(), 0) < 0) {
                    // Remove failed client
                    removeClient(clientSocket);
                }
            }
        }
    }

    void removeClient(int clientSocket) {
        lock_guard<mutex> lock(clientsMutex);
        clientSockets.erase(
            remove(clientSockets.begin(), clientSockets.end(), clientSocket),
            clientSockets.end()
        );
    }
};

// Global server instance for signal handling
ChatServer* serverInstance = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cout << "\nShutting down server..." << endl;
        if (serverInstance) {
            serverInstance->stop();
        }
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    int port = 8080; // Default port
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            cerr << "Invalid port number. Using default port 8080." << endl;
            port = 8080;
        }
    }

    try {
        ChatServer server(port);
        serverInstance = &server;
        
        // Set up signal handlers
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        server.start();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}