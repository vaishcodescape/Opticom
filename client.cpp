#include <iostream> 
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>

using namespace std;

// Simple XOR encryption/decryption (must match server)
namespace Encryption
{
    const string KEY = "OpticomSecureKey2025"; // Shared key (must match server)
    
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

static const char* CLR_RESET = "\033[0m";
static const char* CLR_INFO  = "\033[36m"; // cyan
static const char* CLR_WARN  = "\033[33m"; // yellow
static const char* CLR_ERR   = "\033[31m"; // red
static const char* CLR_PM    = "\033[35m"; // magenta
static const char* CLR_ME    = "\033[32m"; // green

static atomic<bool> g_running(true);

static void printBanner() {
    cout << CLR_INFO;
    cout << "\n================================================\n";
    cout << "                                                \n";
    cout << "             OPTICOM CLIENT                     \n";
    cout << "       This Chat is end to end encrypted        \n";
    cout << "                                                \n";
    cout << "================================================\n";
    cout << CLR_RESET;
    cout << CLR_WARN << "Commands: " << CLR_RESET 
         << "/list /rooms /join /pm /block /help /clear /quit\n\n";
}

static void printPrompt(const string& prompt) {
    cout << CLR_ME << prompt << CLR_RESET << flush;
}

void receiveMessages(int sock, const string promptLabel) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            cout << "\n" << CLR_ERR << "╔═══════════════════════════════╗\n";
            cout << "║ Disconnected from server     ║\n";
            cout << "╚═══════════════════════════════╝" << CLR_RESET << endl;
            close(sock);
            g_running = false;
            return;
        }
        
        // Decrypt message
        string encrypted(buffer, bytes);
        string line = Encryption::decrypt(encrypted);
        
        // Enhanced color heuristics with better formatting
        if (line.find("[SERVER]") != string::npos) {
            cout << "\n" << CLR_INFO << "┃ " << line << CLR_RESET << endl;
        } else if (line.find("[PM from ") != string::npos) {
            cout << "\n" << CLR_PM << "✉ " << line << CLR_RESET << endl;
        } else if (line.find("joined") != string::npos) {
            cout << CLR_INFO << "→ " << line << CLR_RESET << endl;
        } else if (line.find("left") != string::npos) {
            cout << CLR_WARN << "← " << line << CLR_RESET << endl;
        } else if (line.find("Rate limit") != string::npos || line.find("blocked") != string::npos) {
            cout << CLR_ERR << "⚠ " << line << CLR_RESET << endl;
        } else if (line.find("Blocked user") != string::npos || line.find("Unblocked") != string::npos) {
            cout << CLR_WARN << "🚫 " << line << CLR_RESET << endl;
        } else if (line.find("pinned") != string::npos) {
            cout << CLR_INFO << "📌 " << line << CLR_RESET << endl;
        } else if (line.find("Usage:") != string::npos || line.find("Commands:") != string::npos) {
            cout << CLR_WARN << line << CLR_RESET << endl;
        } else {
            cout << line << endl;
        }
        // Repaint prompt
        printPrompt(promptLabel);
    }
}

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    if (argc != 2 && argc != 3) {
        cerr << "Usage: ./client [server_ip] <port>" << endl;
        cerr << "Example (local): ./client 8080" << endl;
        cerr << "Example (remote): ./client 192.168.1.105 8080" << endl;
        return 1;
    }

 
    string serverIp = "127.0.0.1";
    int port;

    try {
        if (argc == 2) {
            port = stoi(argv[1]);
        } else {
            serverIp = argv[1];
            port = stoi(argv[2]);
        }
        
        if (port < 1 || port > 65535) {
            cerr << "Error: Port must be between 1 and 65535" << endl;
            return 1;
        }
    } catch (const exception &e) {
        cerr << "Error: Invalid port number" << endl;
        return 1;
    }

    printBanner();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed to " << serverIp << ":" << port << endl;
        close(sock);
        return 1;
    }

    
    string username;
    cout << "Enter your username: ";
    getline(cin, username);
    if (username.empty()) username = "Anonymous";
    
    // Limit username length to prevent buffer issues (server expects max 63 bytes)
    if (username.length() > 63) {
        username = username.substr(0, 63);
        cout << "Warning: Username truncated to 63 characters" << endl;
    }
    
    send(sock, username.c_str(), username.size(), 0);

    cout << "Connected to server (" << serverIp << ":" << port << ") as " << username << endl;
    cout << "Type messages below. Use /quit to disconnect & /list to see people who are online." << endl;

    string promptLabel = username + " > ";
    thread receiver(receiveMessages, sock, promptLabel);
    receiver.detach();

    string message;
    while (true) {
        printPrompt(promptLabel);
        if (!getline(cin, message)) {
            cout << endl;
            break;
        }

        if (message == "/quit") {
            cout << "Disconnecting..." << endl;
            close(sock);
            return 0;
        }

        if (message == "/clear") {
            cout << "\033[2J\033[H";
            printBanner();
            continue;
        }

        if (message.empty()) continue;

        // Encrypt message before sending
        string encrypted = Encryption::encrypt(message);
        ssize_t sent = send(sock, encrypted.c_str(), encrypted.size(), 0);
        if (sent < 0) {
            cerr << "Send failed." << endl;
            close(sock);
            return 1;
        }
        if (!g_running.load()) break;
    }

    return 0;
}
