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
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

// To store socket username and address together
struct ClientInfo {
    int socket;
    string name;
    string addr;
};

class ChatServer {
private:
    int serverSocket;
    int port;

    //Updated the vector
    vector<ClientInfo> clients;

    mutex clientsMutex;
    bool running;

public:
    ChatServer(int port) : port(port), running(false) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            throw runtime_error("Failed to create socket");
        }

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

        //Clients get handled in handleclient()
        while (running) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket < 0) {
                if (running) cerr << "Failed to accept client connection" << endl;
                continue;
            }

            // Reads client address
            string clientIp = inet_ntoa(clientAddr.sin_addr);
            int clientPort = ntohs(clientAddr.sin_port);
            string addrStr = clientIp + ":" + to_string(clientPort);
            cout << "Incoming connection from " << addrStr << endl;

            
            thread clientThread(&ChatServer::handleClient, this, clientSocket, addrStr);
            clientThread.detach();
        }
    }

    void stop() {
        running = false;
        if (serverSocket >= 0) {
            close(serverSocket);
            serverSocket = -1;
        }

        lock_guard<mutex> lock(clientsMutex);
        for (auto &c : clients) {
            close(c.socket);
        }
        clients.clear();
    }

private:
    // timestamps in the form [HH:MM:SS]
    static string nowTimestamp() {
        using namespace chrono;
        auto t = system_clock::now();
        auto tt = system_clock::to_time_t(t);
        tm local_tm;
        localtime_r(&tt, &local_tm);
        stringstream ss;
        ss << put_time(&local_tm, "%H:%M:%S");
        return ss.str();
    }

    
    // handleClient() includes:
    //    - username handshake
    //    - join/leave messages
    //    - timestamped broadcasts
    void handleClient(int clientSocket, string addrStr) {
        // --- USERNAME HANDSHAKE 
        const int NAME_MAX = 64;
        char nameBuf[NAME_MAX];
        memset(nameBuf, 0, sizeof(nameBuf));

        ssize_t r = recv(clientSocket, nameBuf, sizeof(nameBuf) - 1, 0);
        if (r <= 0) {
            cerr << "Failed to read username from " << addrStr << endl;
            close(clientSocket);
            return;
        }

        string username(nameBuf);
        while (!username.empty() && (username.back() == '\n' || username.back() == '\r'))
            username.pop_back();
        if (username.empty()) username = "Anonymous";

        // --- ADDS CLIENT 
        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back({clientSocket, username, addrStr});
        }

        // --- ANNOUNCES NEW
        string joinMsg = "[" + nowTimestamp() + "] " + username + " joined the chat";
        cout << joinMsg << " (" << addrStr << ")" << endl;
        broadcastMessage(joinMsg, clientSocket);

        // --- MAIN MESSAGE LOOP ---
        char buffer[1024];
        while (running) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                // Shows people who disconnected
                cout << "[" << nowTimestamp() << "] " << username << " disconnected" << endl;
                removeClient(clientSocket);
                close(clientSocket);
                string leftMsg = "[" + nowTimestamp() + "] " + username + " left the chat";
                broadcastMessage(leftMsg, -1);
                break;
            }

            
            string raw(buffer, buffer + bytesReceived);
            while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r')) raw.pop_back();
            if (raw.empty()) continue;
            
            //handle /list command shows users who are online
            if (raw == "/list") {
             string listMsg = "Online users:\n";
             {
             lock_guard<mutex> lock(clientsMutex);
             for (auto &c : clients)
                listMsg += " - " + c.name + "\n";}
                send(clientSocket, listMsg.c_str(), listMsg.size(), 0);
                continue;
            } // skips broadcasting this command


            string formatted = "[" + nowTimestamp() + "] " + username + ": " + raw;
            cout << "Received: " << formatted << endl;
            broadcastMessage(formatted, clientSocket);
        }
    }

    
    // broadcastMessage():
    //    - Iterates over vector<ClientInfo>
    //    - Removes dead clients safely
    //    - Appends newline to messages
    void broadcastMessage(const string& message, int senderSocket) {
        lock_guard<mutex> lock(clientsMutex);
        for (auto it = clients.begin(); it != clients.end();) {
            int sock = it->socket;
            if (sock == senderSocket) {
                ++it;
                continue;
            }

            ssize_t sent = send(sock, message.c_str(), message.size(), 0);
            if (sent < 0) {
                close(sock);
                it = clients.erase(it);
            } else {
                send(sock, "\n", 1, 0);  // 🔹 NEW: makes messages display neatly
                ++it;
            }
        }
    }

    // 🔹 Slightly updated to work with ClientInfo
    void removeClient(int clientSocket) {
        lock_guard<mutex> lock(clientsMutex);
        clients.erase(remove_if(clients.begin(), clients.end(),
                                [clientSocket](const ClientInfo& c){ return c.socket == clientSocket; }),
                      clients.end());
    }
};


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
    int port = 8080;

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

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        server.start();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

