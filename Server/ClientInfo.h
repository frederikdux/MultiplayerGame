// server/ClientInfo.h
#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <memory>

class ClientInfo {
public:
    ClientInfo(std::unique_ptr<sf::TcpSocket> socket, int id);

    sf::TcpSocket* getSocket();
    std::string getName() const;
    int getId() const;
    void setName(const std::string& name);

private:
    std::unique_ptr<sf::TcpSocket> socket; // Einzigartiger Zeiger auf den Socket
    std::string name;                      // Name des Clients
    int id;
};
