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
#include <fstream>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

// ANSI Color Codes
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RED     "\033[31m"

// Simple XOR encryption/decryption
namespace Encryption
{
    const string KEY = "OpticomSecureKey2024"; // Shared key
    
    string encrypt(const string &plaintext)
    {
        string result = plaintext;
        for (size_t i = 0; i < result.length(); ++i)
        {
            result[i] ^= KEY[i % KEY.length()];
        }
        return result;
    }
    
    string decrypt(const string &ciphertext)
    {
        return encrypt(ciphertext); // XOR is symmetric
    }
}

struct ClientInfo
{
    int socket;
    string name;
    string addr;
    string room = "general";
    vector<string> blockedUsers; // List of blocked usernames

    chrono::steady_clock::time_point lastMsgTime = chrono::steady_clock::now();
    int msgCount = 0;
    ClientInfo(int s, const string &n, const string &a, const string &r)
        : socket(s), name(n), addr(a), room(r) {}
};

class ChatServer
{
private:
    int serverSocket;
    int port;
    vector<ClientInfo> clients;
    mutex clientsMutex;
    bool running;
    unordered_map<string, int> roomSlowmodeSeconds; // seconds per room

public:
    ChatServer(int port) : port(port), running(false)
    {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0)
            throw runtime_error("Failed to create socket");

        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw runtime_error("Failed to set socket options");
    }

    ~ChatServer()
    {
        stop();
        if (serverSocket >= 0)
            close(serverSocket);
    }

    void start()
    {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (::bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
            throw runtime_error("Failed to bind socket to port " + to_string(port));

        if (listen(serverSocket, 10) < 0)
            throw runtime_error("Failed to listen on socket");

        running = true;
        
        // Display startup banner
        cout << "\n" << COLOR_BOLD << COLOR_CYAN;
        cout << "================================================\n";
        cout << "                                                \n";
        cout << "           " << COLOR_YELLOW << "OPTICOM CHAT SERVER" << COLOR_CYAN << "             \n";
        cout << "                                                \n";
        cout << "================================================\n" << COLOR_RESET;
        cout << COLOR_GREEN << "✓ Server started on port " << port << COLOR_RESET << endl;
        cout << COLOR_BLUE << "⌛ Waiting for clients to connect...\n" << COLOR_RESET;
        cout << COLOR_MAGENTA << "📝 Admin commands: type 'help' for options\n" << COLOR_RESET;
        cout << string(50, '-') << "\n" << endl;

        thread(&ChatServer::adminConsole, this).detach();

        while (running)
        {
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientAddrLen);
            if (clientSocket < 0)
                continue;

            string clientIp = inet_ntoa(clientAddr.sin_addr);
            int clientPort = ntohs(clientAddr.sin_port);
            string addrStr = clientIp + ":" + to_string(clientPort);

            thread(&ChatServer::handleClient, this, clientSocket, addrStr).detach();
        }
    }

    void stop()
    {
        running = false;
        if (serverSocket >= 0)
            close(serverSocket);
        lock_guard<mutex> lock(clientsMutex);
        for (auto &c : clients)
            close(c.socket);
        clients.clear();
    }

private:
    static bool sendAll(int sock, const char* data, size_t len)
    {
        // Encrypt before sending
        string plaintext(data, len);
        string encrypted = Encryption::encrypt(plaintext);
        
        size_t totalSent = 0;
        const char* encData = encrypted.c_str();
        size_t encLen = encrypted.length();
        
        while (totalSent < encLen)
        {
            ssize_t n = send(sock, encData + totalSent, encLen - totalSent, 0);
            if (n <= 0) return false;
            totalSent += static_cast<size_t>(n);
        }
        return true;
    }

    static void ensureHistoryDir()
    {
        struct stat st{};
        if (stat("history", &st) != 0)
        {
            mkdir("history", 0755);
        }
    }

    static string nowTimestamp()
    {
        using namespace chrono;
        auto t = system_clock::now();
        auto tt = system_clock::to_time_t(t);
        tm local_tm;
        localtime_r(&tt, &local_tm);
        stringstream ss;
        ss << put_time(&local_tm, "%H:%M:%S");
        return ss.str();
    }

    void adminConsole()
    {
        string cmd;
        while (running)
        {
            getline(cin, cmd);
            if (!running)
                break;

            if (cmd.rfind("kick ", 0) == 0)
            {
                string user = cmd.substr(5);
                kickUser(user);
            }
            else if (cmd.rfind("say ", 0) == 0)
            {
                string msg = "[SERVER] " + cmd.substr(4);
                broadcastMessage(msg, -1, "general");
            }
            else if (cmd.rfind("slowmode ", 0) == 0)
            {
                // slowmode <room> <seconds>
                istringstream iss(cmd.substr(9));
                string room; int seconds = 0;
                if (iss >> room >> seconds)
                {
                    {
                        lock_guard<mutex> lock(clientsMutex);
                        roomSlowmodeSeconds[room] = max(0, seconds);
                    }
                    string notice = "[SERVER] Slowmode for room '" + room + "' set to " + to_string(seconds) + "s";
                    broadcastMessage(notice, -1, room);
                    cout << notice << endl;
                }
                else
                {
                    cout << "Usage: slowmode <room> <seconds>" << endl;
                }
            }
            else if (cmd == "list")
            {
                lock_guard<mutex> lock(clientsMutex);
                cout << COLOR_CYAN << "\n╔═══ Connected Users ═══╗" << COLOR_RESET << endl;
                for (auto &c : clients)
                    cout << COLOR_GREEN << "  • " << COLOR_RESET << c.name 
                         << COLOR_YELLOW << " (" << c.addr << ")" << COLOR_RESET
                         << COLOR_BLUE << " room=" << c.room << COLOR_RESET << endl;
                cout << COLOR_CYAN << "╚═══════════════════════╝\n" << COLOR_RESET << endl;
            }
            else if (cmd == "help")
            {
                cout << COLOR_MAGENTA << "\n╔═══ Admin Commands ═══╗" << COLOR_RESET << endl;
                cout << COLOR_YELLOW << "  kick <username>" << COLOR_RESET << "       - Kick a user\n";
                cout << COLOR_YELLOW << "  say <message>" << COLOR_RESET << "         - Broadcast to general room\n";
                cout << COLOR_YELLOW << "  slowmode <room> <sec>" << COLOR_RESET << " - Set room slowmode\n";
                cout << COLOR_YELLOW << "  list" << COLOR_RESET << "                  - List online users\n";
                cout << COLOR_YELLOW << "  help" << COLOR_RESET << "                  - Show this help\n";
                cout << COLOR_MAGENTA << "╚═══════════════════════╝\n" << COLOR_RESET << endl;
            }
        }
    }

    void saveMessage(const string &room, const string &message)
    {
        ensureHistoryDir();
        ofstream file("history/history_" + room + ".txt", ios::app);
        if (file.is_open())
            file << message << endl;
    }

    void sendRoomHistory(int clientSocket, const string &room)
    {
        ensureHistoryDir();
        ifstream file("history/history_" + room + ".txt");
        if (!file.is_open())
            return;
        string line;
        string header = "---- Chat History for room '" + room + "' ----\n";
        sendAll(clientSocket, header.c_str(), header.size());
        while (getline(file, line))
        {
            sendAll(clientSocket, line.c_str(), line.size());
            sendAll(clientSocket, "\n", 1);
        }
        string footer = "-------------------------------------------\n";
        sendAll(clientSocket, footer.c_str(), footer.size());
    }

    bool isRateLimited(int clientSocket)
    {
        lock_guard<mutex> lock(clientsMutex);

        for (auto &c : clients)
        {
            if (c.socket == clientSocket)
            {
                auto now = chrono::steady_clock::now();
                auto diff = chrono::duration_cast<chrono::milliseconds>(now - c.lastMsgTime).count();

                if (diff > 1000)
                {
                    c.msgCount = 0;
                    c.lastMsgTime = now;
                }

                c.msgCount++;

                if (c.msgCount > 3)
                    return true;
                return false;
            }
        }
        return false;
    }

    void handleClient(int clientSocket, string addrStr)
    {
        const int USERNAME_MAX = 64;
        char nameBuf[USERNAME_MAX]{};
        ssize_t r = recv(clientSocket, nameBuf, sizeof(nameBuf) - 1, 0);
        if (r <= 0)
        {
            close(clientSocket);
            return;
        }

        string username(nameBuf);
        while (!username.empty() && (username.back() == '\n' || username.back() == '\r'))
            username.pop_back();
        if (username.empty())
            username = "Anonymous";

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back({clientSocket, username, addrStr, "general"});
        }

        string joinMsg = "[" + nowTimestamp() + "] " + username + " joined the chat (room: general)";
        cout << COLOR_GREEN << "→ " << COLOR_RESET << joinMsg << endl;
        broadcastMessage(joinMsg, clientSocket, "general");
        saveMessage("general", joinMsg);
        sendRoomHistory(clientSocket, "general");

        char buffer[1024];
        while (running)
        {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0)
            {
                close(clientSocket);
                removeClient(clientSocket);
                string leftMsg = "[" + nowTimestamp() + "] " + username + " left the chat";
                cout << COLOR_RED << "← " << COLOR_RESET << leftMsg << endl;
                broadcastMessage(leftMsg, -1, "general");
                saveMessage("general", leftMsg);
                break;
            }

            // Decrypt received message
            string encrypted(buffer, buffer + bytes);
            string msg = Encryption::decrypt(encrypted);
            
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
                msg.pop_back();
            if (msg.empty())
                continue;

            if (msg == "/rooms")
            {
                lock_guard<mutex> lock(clientsMutex);

                unordered_map<string, int> roomCount;
                for (auto &c : clients)
                {
                    roomCount[c.room]++;
                }

                string out = "Active Rooms:\n";
                for (auto &r : roomCount)
                {
                    out += " - " + r.first + " (" + to_string(r.second) + " users)\n";
                }

                sendAll(clientSocket, out.c_str(), out.size());
                continue;
            }

            if (msg == "/help")
            {
                string help =
                    "Available Commands:\n"
                    "/list               - List online users\n"
                    "/rooms              - List all active rooms\n"
                    "/join <room>        - Join or create a room\n"
                    "/pm <user> <msg>    - Private message\n"
                    "/block <user>       - Block messages from a user\n"
                    "/unblock <user>     - Unblock a user\n"
                    "/blocklist          - Show your blocked users\n"
                    "/pin <msg>          - Pin a message to the room board\n"
                    "/pins               - Show pinned messages for the room\n"
                    "/unpin <index>      - Remove a pinned message by index\n"
                    "/quit               - Disconnect from server\n"
                    "/help               - Show this help\n";

                help += "\n"; // <- IMPORTANT: ensures it prints immediately
                sendAll(clientSocket, help.c_str(), help.size());
                continue;
            }

            if (msg == "/list")
            {
                string listMsg = "Online users:\n";
                lock_guard<mutex> lock(clientsMutex);
                for (auto &c : clients)
                    listMsg += " - " + c.name + " (room: " + c.room + ")\n";
                sendAll(clientSocket, listMsg.c_str(), listMsg.size());
                continue;
            }

            if (msg.rfind("/join ", 0) == 0)
            {
                string newRoom = msg.substr(6);
                if (newRoom.empty())
                    newRoom = "general";

                string oldRoom;

                {
                    lock_guard<mutex> lock(clientsMutex);
                    for (auto &c : clients)
                    {
                        if (c.socket == clientSocket)
                        {
                            oldRoom = c.room;
                        }
                    }
                }

                if (oldRoom != newRoom)
                {

                    string leftMsg = "[" + nowTimestamp() + "] " + username + " left room " + oldRoom;
                    broadcastMessage(leftMsg, clientSocket, oldRoom);
                    saveMessage(oldRoom, leftMsg);

                    {
                        lock_guard<mutex> lock(clientsMutex);
                        for (auto &c : clients)
                        {
                            if (c.socket == clientSocket)
                            {
                                c.room = newRoom;
                            }
                        }
                    }

                    string joinMsg = "[" + nowTimestamp() + "] " + username + " joined room " + newRoom;
                    broadcastMessage(joinMsg, clientSocket, newRoom);
                    saveMessage(newRoom, joinMsg);

                    sendRoomHistory(clientSocket, newRoom);
                }

                continue;
            }

            if (msg.rfind("/block ", 0) == 0)
            {
                string targetUser = msg.substr(7);
                if (targetUser.empty())
                {
                    string err = "Usage: /block <username>\n";
                    sendAll(clientSocket, err.c_str(), err.size());
                    continue;
                }
                {
                    lock_guard<mutex> lock(clientsMutex);
                    for (auto &c : clients)
                    {
                        if (c.socket == clientSocket)
                        {
                            if (find(c.blockedUsers.begin(), c.blockedUsers.end(), targetUser) != c.blockedUsers.end())
                            {
                                string msg = "User '" + targetUser + "' is already blocked.\n";
                                sendAll(clientSocket, msg.c_str(), msg.size());
                            }
                            else
                            {
                                c.blockedUsers.push_back(targetUser);
                                string msg = "Blocked user '" + targetUser + "'.\n";
                                sendAll(clientSocket, msg.c_str(), msg.size());
                            }
                            break;
                        }
                    }
                }
                continue;
            }

            if (msg.rfind("/unblock ", 0) == 0)
            {
                string targetUser = msg.substr(9);
                if (targetUser.empty())
                {
                    string err = "Usage: /unblock <username>\n";
                    sendAll(clientSocket, err.c_str(), err.size());
                    continue;
                }
                {
                    lock_guard<mutex> lock(clientsMutex);
                    for (auto &c : clients)
                    {
                        if (c.socket == clientSocket)
                        {
                            auto it = find(c.blockedUsers.begin(), c.blockedUsers.end(), targetUser);
                            if (it != c.blockedUsers.end())
                            {
                                c.blockedUsers.erase(it);
                                string msg = "Unblocked user '" + targetUser + "'.\n";
                                sendAll(clientSocket, msg.c_str(), msg.size());
                            }
                            else
                            {
                                string msg = "User '" + targetUser + "' is not blocked.\n";
                                sendAll(clientSocket, msg.c_str(), msg.size());
                            }
                            break;
                        }
                    }
                }
                continue;
            }

            if (msg == "/blocklist")
            {
                lock_guard<mutex> lock(clientsMutex);
                for (auto &c : clients)
                {
                    if (c.socket == clientSocket)
                    {
                        if (c.blockedUsers.empty())
                        {
                            string msg = "You have no blocked users.\n";
                            sendAll(clientSocket, msg.c_str(), msg.size());
                        }
                        else
                        {
                            string msg = "Blocked users:\n";
                            for (const auto &u : c.blockedUsers)
                                msg += " - " + u + "\n";
                            sendAll(clientSocket, msg.c_str(), msg.size());
                        }
                        break;
                    }
                }
                continue;
            }

            if (msg.rfind("/pin ", 0) == 0)
            {
                string room = getClientRoom(clientSocket);
                string text = msg.substr(5);
                if (text.empty())
                {
                    string err = "Usage: /pin <message>\n";
                    sendAll(clientSocket, err.c_str(), err.size());
                    continue;
                }
                ensureHistoryDir();
                string formatted = "📌 [" + nowTimestamp() + "] " + username + ": " + text;
                {
                    ofstream pf("history/pins_" + room + ".txt", ios::app);
                    if (pf.is_open()) pf << formatted << endl;
                }
                string notice = "[" + nowTimestamp() + "] " + username + " pinned a message.";
                broadcastMessage(notice, -1, room);
                sendAll(clientSocket, "Pinned.\n", 8);
                continue;
            }

            if (msg == "/pins")
            {
                string room = getClientRoom(clientSocket);
                ensureHistoryDir();
                ifstream pf("history/pins_" + room + ".txt");
                if (!pf.is_open())
                {
                    string none = "No pins yet.\n";
                    sendAll(clientSocket, none.c_str(), none.size());
                    continue;
                }
                string line;
                string header = "Pinned messages in '" + room + "':\n";
                sendAll(clientSocket, header.c_str(), header.size());
                int idx = 1;
                while (getline(pf, line))
                {
                    string entry = to_string(idx++) + ". " + line + "\n";
                    sendAll(clientSocket, entry.c_str(), entry.size());
                }
                continue;
            }

            if (msg.rfind("/unpin ", 0) == 0)
            {
                string room = getClientRoom(clientSocket);
                string idxStr = msg.substr(7);
                int idx = 0;
                try { idx = stoi(idxStr); } catch (...) { idx = 0; }
                if (idx <= 0)
                {
                    string err = "Usage: /unpin <index>\n";
                    sendAll(clientSocket, err.c_str(), err.size());
                    continue;
                }
                ensureHistoryDir();
                string path = "history/pins_" + room + ".txt";
                ifstream pf(path);
                if (!pf.is_open())
                {
                    string none = "No pins to unpin.\n";
                    sendAll(clientSocket, none.c_str(), none.size());
                    continue;
                }
                vector<string> all;
                string line;
                while (getline(pf, line)) all.push_back(line);
                pf.close();
                if (idx > (int)all.size())
                {
                    string err = "Invalid index.\n";
                    sendAll(clientSocket, err.c_str(), err.size());
                    continue;
                }
                all.erase(all.begin() + (idx - 1));
                ofstream wf(path, ios::trunc);
                for (auto &l : all) wf << l << "\n";
                string ok = "Unpinned #" + to_string(idx) + ".\n";
                sendAll(clientSocket, ok.c_str(), ok.size());
                continue;
            }

            if (msg.rfind("/pm ", 0) == 0)
            {
                string rest = msg.substr(4);
                size_t space = rest.find(' ');
                if (space == string::npos)
                {
                    string err = "Usage: /pm <username> <message>\n";
                    sendAll(clientSocket, err.c_str(), err.size());
                    continue;
                }
                string targetUser = rest.substr(0, space);
                string privateMsg = rest.substr(space + 1);
                sendPrivateMessage(username, targetUser, privateMsg);
                continue;
            }

            if (isRateLimited(clientSocket))
            {
                string warn = "⚠️ Rate limit exceeded. Slow down!\n";
                sendAll(clientSocket, warn.c_str(), warn.size());
                continue;
            }

            // Room slowmode check
            {
                string room = getClientRoom(clientSocket);
                int slowSeconds = 0;
                {
                    lock_guard<mutex> lock(clientsMutex);
                    auto it = roomSlowmodeSeconds.find(room);
                    if (it != roomSlowmodeSeconds.end()) slowSeconds = it->second;
                }
                if (slowSeconds > 0)
                {
                    bool blocked = false;
                    long remaining = 0;
                    {
                        lock_guard<mutex> lock(clientsMutex);
                        for (auto &c : clients)
                        {
                            if (c.socket == clientSocket)
                            {
                                auto now = chrono::steady_clock::now();
                                auto diff = chrono::duration_cast<chrono::seconds>(now - c.lastMsgTime).count();
                                if (diff < slowSeconds)
                                {
                                    blocked = true;
                                    remaining = slowSeconds - diff;
                                }
                                else
                                {
                                    c.lastMsgTime = now; // reuse existing timestamp for slowmode window
                                }
                                break;
                            }
                        }
                    }
                    if (blocked)
                    {
                        string warn = "⌛ Slowmode is on (" + to_string(slowSeconds) + "s). Wait " + to_string(remaining) + "s.\n";
                        sendAll(clientSocket, warn.c_str(), warn.size());
                        continue;
                    }
                }
            }

            string formatted = "[" + nowTimestamp() + "] " + username + ": " + msg;
            cout << COLOR_BLUE << "💬 " << COLOR_RESET << formatted << endl;
            broadcastMessage(formatted, clientSocket, getClientRoom(clientSocket));
            saveMessage(getClientRoom(clientSocket), formatted);
        }
    }

    string getClientRoom(int sock)
    {
        lock_guard<mutex> lock(clientsMutex);
        for (auto &c : clients)
            if (c.socket == sock)
                return c.room;
        return "general";
    }

    void sendPrivateMessage(const string &fromUser, const string &toUser, const string &msg)
    {
        lock_guard<mutex> lock(clientsMutex);
        int fromSock = -1;
        for (auto &c : clients)
        {
            if (c.name == fromUser) fromSock = c.socket;
        }
        for (auto &c : clients)
        {
            if (c.name == toUser)
            {
                // Check if receiver has blocked sender
                bool isBlocked = find(c.blockedUsers.begin(), c.blockedUsers.end(), fromUser) != c.blockedUsers.end();
                
                if (isBlocked)
                {
                    if (fromSock != -1)
                    {
                        string notice = "Cannot send message: user has blocked you.\n";
                        sendAll(fromSock, notice.c_str(), notice.size());
                    }
                    return;
                }
                
                string formatted = "[PM from " + fromUser + "] " + msg + "\n";
                sendAll(c.socket, formatted.c_str(), formatted.size());
                return;
            }
        }
        if (fromSock != -1)
        {
            string notice = "User '" + toUser + "' is not online.\n";
            sendAll(fromSock, notice.c_str(), notice.size());
        }
    }

    void broadcastMessage(const string &message, int senderSocket, const string &room)
    {
        lock_guard<mutex> lock(clientsMutex);
        
        // Get sender's username
        string senderName;
        for (const auto &c : clients)
        {
            if (c.socket == senderSocket)
            {
                senderName = c.name;
                break;
            }
        }
        
        for (auto it = clients.begin(); it != clients.end();)
        {
            if (it->room == room && it->socket != senderSocket)
            {
                // Check if receiver has blocked the sender
                bool isBlocked = find(it->blockedUsers.begin(), it->blockedUsers.end(), senderName) != it->blockedUsers.end();
                
                if (!isBlocked)
                {
                    bool ok = sendAll(it->socket, message.c_str(), message.size()) &&
                              sendAll(it->socket, "\n", 1);
                    if (!ok)
                    {
                        close(it->socket);
                        it = clients.erase(it);
                        continue;
                    }
                }
            }
            ++it;
        }
    }

    void removeClient(int clientSocket)
    {
        lock_guard<mutex> lock(clientsMutex);
        clients.erase(remove_if(clients.begin(), clients.end(),
                                [clientSocket](const ClientInfo &c)
                                { return c.socket == clientSocket; }),
                      clients.end());
    }

    void kickUser(const string &username)
    {
        lock_guard<mutex> lock(clientsMutex);
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->name == username)
            {
                string msg = "[SERVER] You have been kicked by admin.\n";
                send(it->socket, msg.c_str(), msg.size(), 0);
                close(it->socket);
                clients.erase(it);
                cout << COLOR_RED << "⚠ Kicked user: " << COLOR_RESET << username << endl;
                return;
            }
        }
        cout << COLOR_YELLOW << "⚠ No such user: " << COLOR_RESET << username << endl;
    }
};

ChatServer *serverInstance = nullptr;
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        cout << "\n" << COLOR_RED << "⏻ Shutting down server..." << COLOR_RESET << endl;
        if (serverInstance)
            serverInstance->stop();
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    int port = 8080;
    if (argc > 1)
        port = atoi(argv[1]);
    try
    {
        ChatServer server(port);
        serverInstance = &server;
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGPIPE, SIG_IGN);
        server.start();
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
