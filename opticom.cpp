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
#include <fstream>   // 🆕 for message history

using namespace std;

struct ClientInfo {
    int socket;
    string name;
    string addr;
    string room = "general";

    ClientInfo(int s, const string& n, const string& a, const string& r)
        : socket(s), name(n), addr(a), room(r) {}
};


class ChatServer {
private:
    int serverSocket;
    int port;
    vector<ClientInfo> clients;
    mutex clientsMutex;
    bool running;

public:
    ChatServer(int port) : port(port), running(false) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) throw runtime_error("Failed to create socket");

        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw runtime_error("Failed to set socket options");
    }

    ~ChatServer() {
        stop();
        if (serverSocket >= 0) close(serverSocket);
    }

    void start() {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (::bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
            throw runtime_error("Failed to bind socket to port " + to_string(port));

        if (listen(serverSocket, 10) < 0)
            throw runtime_error("Failed to listen on socket");

        running = true;
        cout << "Opticom Chat Server started on port " << port << endl;
        cout << "Waiting for clients to connect..." << endl;

        while (running) {
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket < 0) continue;

            string clientIp = inet_ntoa(clientAddr.sin_addr);
            int clientPort = ntohs(clientAddr.sin_port);
            string addrStr = clientIp + ":" + to_string(clientPort);

            thread(&ChatServer::handleClient, this, clientSocket, addrStr).detach();
        }
    }

    void stop() {
        running = false;
        if (serverSocket >= 0) close(serverSocket);
        lock_guard<mutex> lock(clientsMutex);
        for (auto &c : clients) close(c.socket);
        clients.clear();
    }

private:
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

    // 🆕 Save message to file by room
    void saveMessage(const string& room, const string& message) {
        ofstream file("history_" + room + ".txt", ios::app);
        if (file.is_open()) file << message << endl;
    }

    // 🆕 Load message history when user joins
    void sendRoomHistory(int clientSocket, const string& room) {
        ifstream file("history_" + room + ".txt");
        if (!file.is_open()) return;
        string line;
        string header = "---- Chat History for room '" + room + "' ----\n";
        send(clientSocket, header.c_str(), header.size(), 0);
        while (getline(file, line)) {
            send(clientSocket, line.c_str(), line.size(), 0);
            send(clientSocket, "\n", 1, 0);
        }
        string footer = "-------------------------------------------\n";
        send(clientSocket, footer.c_str(), footer.size(), 0);
    }

    void handleClient(int clientSocket, string addrStr) {
        const int USERNAME_MAX = 64;
        char nameBuf[USERNAME_MAX]{};
        ssize_t r = recv(clientSocket, nameBuf, sizeof(nameBuf) - 1, 0);
        if (r <= 0) return;

        string username(nameBuf);
        while (!username.empty() && (username.back() == '\n' || username.back() == '\r')) username.pop_back();
        if (username.empty()) username = "Anonymous";

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back({clientSocket, username, addrStr, "general"});
        }

        string joinMsg = "[" + nowTimestamp() + "] " + username + " joined the chat (room: general)";
        cout << joinMsg << endl;
        broadcastMessage(joinMsg, clientSocket, "general");
        saveMessage("general", joinMsg);
        sendRoomHistory(clientSocket, "general"); // 🆕 send old messages

        char buffer[1024];
        while (running) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) {
                removeClient(clientSocket);
                string leftMsg = "[" + nowTimestamp() + "] " + username + " left the chat";
                broadcastMessage(leftMsg, -1, "general");
                saveMessage("general", leftMsg);
                break;
            }

            string msg(buffer, buffer + bytes);
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) msg.pop_back();
            if (msg.empty()) continue;

            // 🆕 Commands
            if (msg == "/list") {
                string listMsg = "Online users:\n";
                lock_guard<mutex> lock(clientsMutex);
                for (auto &c : clients) listMsg += " - " + c.name + " (room: " + c.room + ")\n";
                send(clientSocket, listMsg.c_str(), listMsg.size(), 0);
                continue;
            }

            if (msg.rfind("/join ", 0) == 0) {
                string newRoom = msg.substr(6);
                if (newRoom.empty()) newRoom = "general";
                {
                    lock_guard<mutex> lock(clientsMutex);
                    for (auto &c : clients)
                        if (c.socket == clientSocket) c.room = newRoom;
                }
                string joined = "[" + nowTimestamp() + "] " + username + " joined room " + newRoom;
                broadcastMessage(joined, clientSocket, newRoom);
                saveMessage(newRoom, joined);
                sendRoomHistory(clientSocket, newRoom);
                continue;
            }

            if (msg.rfind("/pm ", 0) == 0) {
                string rest = msg.substr(4);
                size_t space = rest.find(' ');
                if (space == string::npos) {
                    string err = "Usage: /pm <username> <message>\n";
                    send(clientSocket, err.c_str(), err.size(), 0);
                    continue;
                }
                string targetUser = rest.substr(0, space);
                string privateMsg = rest.substr(space + 1);
                sendPrivateMessage(username, targetUser, privateMsg);
                continue;
            }

            // Normal message
            string formatted = "[" + nowTimestamp() + "] " + username + ": " + msg;
            broadcastMessage(formatted, clientSocket, getClientRoom(clientSocket));
            saveMessage(getClientRoom(clientSocket), formatted);
        }
    }

    // 🆕 Get client room by socket
    string getClientRoom(int sock) {
        lock_guard<mutex> lock(clientsMutex);
        for (auto &c : clients)
            if (c.socket == sock)
                return c.room;
        return "general";
    }

    // 🆕 Send private message
    void sendPrivateMessage(const string& fromUser, const string& toUser, const string& msg) {
        lock_guard<mutex> lock(clientsMutex);
        for (auto &c : clients) {
            if (c.name == toUser) {
                string formatted = "[PM from " + fromUser + "] " + msg + "\n";
                send(c.socket, formatted.c_str(), formatted.size(), 0);
                return;
            }
        }
    }

    void broadcastMessage(const string& message, int senderSocket, const string& room) {
        lock_guard<mutex> lock(clientsMutex);
        for (auto it = clients.begin(); it != clients.end();) {
            if (it->room == room && it->socket != senderSocket) {
                ssize_t sent = send(it->socket, message.c_str(), message.size(), 0);
                if (sent < 0) {
                    close(it->socket);
                    it = clients.erase(it);
                    continue;
                }
                send(it->socket, "\n", 1, 0);
            }
            ++it;
        }
    }

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
        if (serverInstance) serverInstance->stop();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);
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
