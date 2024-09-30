// server/ClientInfo.cpp
#include "ClientInfo.h"

ClientInfo::ClientInfo(std::unique_ptr<sf::TcpSocket> socket, int id)
    : socket(std::move(socket)), name(""), id(id) {}

sf::TcpSocket* ClientInfo::getSocket() {
    return socket.get();
}

std::string ClientInfo::getName() const {
    return name;
}

void ClientInfo::setName(const std::string& name) {
    this->name = name;
}

int ClientInfo::getId() const {
    return id;
}
