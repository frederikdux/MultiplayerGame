// server/Server.h
#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ClientInfo.h"
#include <enet/enet.h>

#include "../Game/GameInformation.h"


class Server {
public:
    Server(unsigned short tcp_port, unsigned short udp_port, GameInformation& gameInformation);
    ~Server();
    void run();
    static void exceptionHandler();
    static void signalHandler(int signal);

    static Server* getServerInstance() {
        return serverInstance;
    }

    int getServerId() const {
        return serverId;
    }

private:
    std::string dServerAddr = "http://185.117.249.22:8080";
    std::string localAddr = "http://localhost:8080";

    void registerServer();
    void runTcpClient();
    void receiveMessagesAsTCPClient();
    void sendMessageAsTCPClient(const std::string& message);
    void sendUpdatesToDServer();
    void logoutServer();
    void acceptClients();
    void handleClient(ClientInfo* clientInfo);
    void broadcastMessage(const std::string& message, ClientInfo* sender);
    void broadcastNewPlayerJoined(ClientInfo* newPlayer);
    void broadcastPlayerLeft(ClientInfo* player);
    void broadcastPlayerDied(EnemyInformation* enemy);
    void checkForHits();
    void removeClient(ClientInfo* clientInfo);
    void handleMessage(const std::string& message, ClientInfo* clientInfo);
    void sendGameInformation(ClientInfo* clientInfo);
    std::string subString(std::string message, int start, int end);
    std::string extractParameter(const std::string& message, const std::string& parameter);
    //Linux
    void enableRawMode();
    void kbhit();

    void setupENetServer(unsigned short udp_port);
    void processENetEvents();
    void sendBatchedPositionUpdates();
    void handleOutput(const std::string& message);
    void handleError(const std::string& message);
    void handleInput();
    sf::TcpListener listener;
    std::vector<std::unique_ptr<ClientInfo>> clients; // Speicherung von ClientInfo-Objekten
    std::mutex clients_mutex; // Mutex zum Schutz des Clients-Vektors
    ENetAddress address{};
    ENetHost* enet_server = nullptr;
    int nextId = 0;
    int port;
    std::string serverName;
    int serverId = -1;
    static Server *serverInstance;
    std::unordered_map<int, std::string> recentUpdates;
    boolean outputAllowed = true;
    boolean consoleButtonPressed = false;
    int tickrate = 40;
    sf::TcpSocket tcpClientSocket;
    GameInformation& gameInformation;
};
