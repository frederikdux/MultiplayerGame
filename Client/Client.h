// client/Client.h
#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <enet/enet.h>

#include "Game/GameInformation.h"

class Client {
public:
    Client(const std::string& server_ip, unsigned short tcp_port, unsigned short udp_port, GameInformation &gameInformation);
    ~Client();

    void sendMessage(const std::string& message);
    void handleTCPMessage(const std::string& socketMessage);
    void handleUDPMessage(const std::string& socketMessage);
    void sendPositionUpdate(float x, float y);

    [[nodiscard]] sf::TcpSocket &getSocket() {
        return socket;
    }

    [[nodiscard]] int getId() const {
        return id;
    }

    std::string getName() const {
        return name;
    }

    int getHealth() const {
        return health;
    }

private:
    void receiveMessages();
    void setupENetClient(const std::string& server_ip, unsigned short udp_port);
    void processENetEvents();
    void requestGameUpdate();
    static std::string extractParameter(const std::string& message, const std::string& parameter);
    static std::string subString(std::string message, int start, int end);

    sf::TcpSocket socket;
    ENetHost* enet_client = nullptr;
    ENetPeer* enet_peer = nullptr;
    GameInformation& gameInformation;
    int id = -1;
    std::string name;
    int health = 100;
};
