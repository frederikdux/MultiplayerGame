// server/Server.h
#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include "ClientInfo.h"
#include <enet/enet.h>


class Server {
public:
    Server(unsigned short , unsigned short udp_port);
    ~Server();
    void run();

private:
    void acceptClients();
    void handleClient(ClientInfo* clientInfo);
    void broadcastMessage(const std::string& message, ClientInfo* sender);
    void broadcastNewPlayerJoined(ClientInfo* newPlayer);
    void broadcastPlayerLeft(ClientInfo* player);
    void removeClient(ClientInfo* clientInfo);
    void handleMessage(const std::string& message, ClientInfo* clientInfo);
    void sendGameInformation(ClientInfo* clientInfo);
    std::string subString(std::string message, int start, int end);
    std::string extractParameter(const std::string& message, const std::string& parameter);

    void setupENetServer(unsigned short udp_port);
    void processENetEvents();

    sf::TcpListener listener;
    std::vector<std::unique_ptr<ClientInfo>> clients; // Speicherung von ClientInfo-Objekten
    std::mutex clients_mutex; // Mutex zum Schutz des Clients-Vektors
    ENetAddress address;
    ENetHost* enet_server = nullptr;
    int nextId = 0;
};
