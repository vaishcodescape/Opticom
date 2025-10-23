// Simple client for testing Opticom Chat Server
#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class ChatClient {
private:
    int clientSocket;
    string serverIP;
    int port;
    bool connected;

public:
    ChatClient(const string& ip, int port) : serverIP(ip), port(port), connected(false) {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            throw runtime_error("Failed to create socket");
        }
    }

    ~ChatClient() {
        disconnect();
    }

    bool connect() {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            cerr << "Invalid server address" << endl;
            return false;
        }

        if (::connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Failed to connect to server" << endl;
            return false;
        }

        connected = true;
        cout << "Connected to chat server at " << serverIP << ":" << port << endl;
        return true;
    }

    void disconnect() {
        if (connected) {
            close(clientSocket);
            connected = false;
            cout << "Disconnected from server" << endl;
        }
    }

    void sendMessage(const string& message) {
        if (!connected) {
            cerr << "Not connected to server" << endl;
            return;
        }

        if (send(clientSocket, message.c_str(), message.length(), 0) < 0) {
            cerr << "Failed to send message" << endl;
            connected = false;
        }
    }

    void receiveMessages() {
        char buffer[1024];
        
        while (connected) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                cout << "Server disconnected" << endl;
                connected = false;
                break;
            }

            cout << "Received: " << buffer << endl;
        }
    }

    bool isConnected() const {
        return connected;
    }
};

int main(int argc, char* argv[]) {
    string serverIP = "127.0.0.1";
    int port = 8080;

    if (argc > 1) {
        serverIP = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }

    try {
        ChatClient client(serverIP, port);
        
        if (!client.connect()) {
            return 1;
        }

        // Start receiving messages in a separate thread
        thread receiveThread(&ChatClient::receiveMessages, &client);

        cout << "Type messages to send (type 'quit' to exit):" << endl;
        
        string message;
        while (client.isConnected()) {
            getline(cin, message);
            
            if (message == "quit") {
                break;
            }
            
            client.sendMessage(message);
        }

        client.disconnect();
        receiveThread.join();
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
