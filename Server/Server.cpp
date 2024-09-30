// server/Server.cpp
#include "Server.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <variant>

Server::Server(unsigned short tcp_port, unsigned short udp_port) : listener() {
    if (listener.listen(tcp_port) != sf::Socket::Done) {
        std::cerr << "Error: Cannot listen on port " << tcp_port << std::endl;
        return;
    }
    std::cout << "Server is listening on port " << tcp_port << "..." << std::endl;

    setupENetServer(udp_port);
}

Server::~Server() {
    if (enet_server) {
        enet_host_destroy(enet_server);
        enet_deinitialize();
    }
}

void Server::run() {
    std::thread accept_thread(&Server::acceptClients, this);
    accept_thread.detach(); // Startet den Thread für die Akzeptanz von Clients

    std::thread enet_thread(&Server::processENetEvents, this);
    enet_thread.detach();

    // Hauptthread bleibt aktiv, um Nachrichten zu verarbeiten
    while (true) {
        sf::sleep(sf::milliseconds(100)); // Vermeidet Überlastung der CPU
    }
}

void Server::acceptClients() {
    while (true) {
        auto socket = std::make_unique<sf::TcpSocket>();
        if (listener.accept(*socket) == sf::Socket::Done) {
            std::cout << "New client connected: " << socket->getRemoteAddress() <<" id=" << nextId << std::endl;
            ++nextId;

            std::unique_lock<std::mutex> lock(clients_mutex);
            auto clientInfo = std::make_unique<ClientInfo>(std::move(socket), nextId - 1);
            clients.push_back(std::move(clientInfo));


            auto& newClientInfo = clients.back(); // Der gerade hinzugefügte ClientInfo
            // Starte einen neuen Thread, um Nachrichten von diesem Client zu verarbeiten
            std::thread(&Server::handleClient, this, newClientInfo.get()).detach();
        }
    }
}

void Server::handleClient(ClientInfo* clientInfo) {
    char buffer[1024];
    std::size_t received;

    broadcastNewPlayerJoined(clientInfo);

    std::string setIdMessage = "0002 -- " + std::to_string(clientInfo->getId()) + "|";
    clientInfo->getSocket()->send(setIdMessage.c_str(), setIdMessage.size());

    sendGameInformation(clientInfo);

    while (true) {
        sf::Socket::Status status = clientInfo->getSocket()->receive(buffer, sizeof(buffer), received);
        if (status == sf::Socket::Done) {
            std::string message(buffer, received);
            std::cout << "Received message from client: " << message << std::endl;
            handleMessage(message, clientInfo);
        } else if (status == sf::Socket::Disconnected) {
            std::cout << "Client disconnected: " << clientInfo->getSocket()->getRemoteAddress() << std::endl;
            broadcastPlayerLeft(clientInfo);
            removeClient(clientInfo);
            break;
        }
    }
}

void Server::broadcastMessage(const std::string& message, ClientInfo* sender) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if (client.get() != sender && !client->getName().empty()) { // Sende nicht an den Sender selbst
            if (client->getSocket()->send(message.c_str(), message.size()) != sf::Socket::Done) {
                std::cerr << "Error: Could not send message to a client." << std::endl;
            }
        }
    }
}

void Server::removeClient(ClientInfo* clientInfo) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    clients.erase(std::remove_if(clients.begin(), clients.end(),
                                 [clientInfo](const std::unique_ptr<ClientInfo>& c) {
                                     return c.get() == clientInfo;
                                 }),
                  clients.end());

}

void Server::handleMessage(const std::string& socketMessage, ClientInfo* clientInfo) {
    if(socketMessage.find(" -- ") == std::variant_npos){
        std::cout<<"Error: Invalid message format"<<std::endl;
        return;
    }
    int endOfMessageType = socketMessage.find(" -- ");
    std::string type = socketMessage.substr(0, endOfMessageType);
    int endOfMessage = socketMessage.find("|");
    std::string message = subString(socketMessage, endOfMessageType + 4, endOfMessage);

    if(type == "0000") {
        clientInfo->setName(message); // Setze den Namen des Clients
        broadcastMessage("0006 -- id=" + std::to_string(clientInfo->getId()) + ";name=" + clientInfo->getName() + ";|", clientInfo);
        std::cout << "Client set name to: " << clientInfo->getName() << std::endl;
    }
    else if(type == "0001") {
        broadcastMessage("0001 -- " + clientInfo->getName() + ": " + message, clientInfo);
        std::cout << clientInfo->getName() << " send Message: " << message << std::endl;
    }
    else if(type == "1000") {
        sendGameInformation(clientInfo);
    }
    else {
        std::cout<<"Error: Unknown message type"<<std::endl;
        std::cout<<socketMessage<<std::endl;
    }
}

void Server::setupENetServer(unsigned short udp_port) {
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet." << std::endl;
        exit(EXIT_FAILURE);
    }

    address.host = ENET_HOST_ANY;
    address.port = udp_port;

    enet_server = enet_host_create(&address, 32, 1, 0, 0);
    if (!enet_server) {
        std::cerr << "An error occurred while trying to create an ENet server host." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "ENet server is listening on UDP port " << udp_port << "..." << std::endl;
}

void Server::processENetEvents() {
    while (true) {
        ENetEvent event;
        while (enet_host_service(enet_server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    std::string receivedData(reinterpret_cast<char*>(event.packet->data), event.packet->dataLength);
                    //std::cout << "Received UDP message: " << receivedData << std::endl;

                    // Broadcast the received data to all connected clients
                    for (size_t i = 0; i < enet_server->peerCount; ++i) {
                        ENetPeer* currentPeer = &enet_server->peers[i];
                        if (currentPeer->state == ENET_PEER_STATE_CONNECTED && currentPeer != event.peer) {
                            ENetPacket* packet = enet_packet_create(receivedData.c_str(), receivedData.length() + 1, ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(currentPeer, 0, packet);
                        }
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }

                default:
                    break;
            }
        }
    }
}

void Server::sendGameInformation(ClientInfo *clientInfo) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(client->getId() != clientInfo->getId() && !client->getName().empty()) {
            std::string message = "0003 -- id=" + std::to_string(client->getId()) + ";name=" + client->getName() + ";|" ;
            clientInfo->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::broadcastNewPlayerJoined(ClientInfo *newPlayer) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(client->getId() != newPlayer->getId() && !client->getName().empty()) {
            std::string message = "0003 -- id=" + std::to_string(newPlayer->getId()) + ";name= " + newPlayer->getName() + ";|";
            client->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::broadcastPlayerLeft(ClientInfo *player) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(client->getId() != player->getId() && !client->getName().empty()) {
            std::string message = "0005 -- id=" + std::to_string(player->getId()) +"|";
            client->getSocket()->send(message.c_str(), message.size());
        }
    }
}

std::string Server::subString(std::string str, int start, int end) {
    return std::string(&str[start], &str[end]);
}

std::string Server::extractParameter(const std::string &message, const std::string &parameter) {
    //extract Parameter Value from message "parameter=value"
    int start = message.find(parameter) + parameter.length() + 1;
    int end = message.find(';', start);
    return message.substr(start, end - start);
}
