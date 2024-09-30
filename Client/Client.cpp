// client/Client.cpp
#include "Client.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <variant>
#include <enet/enet.h>

#include "Game/GameInformation.h"


Client::Client(const std::string& server_ip, unsigned short tcp_port, unsigned short udp_port, GameInformation &gameInformation) : gameInformation(gameInformation){
    if (socket.connect(server_ip, tcp_port) != sf::Socket::Done) {
        std::cerr << "Error: Unable to connect to the server!" << std::endl;
    } else {
        std::cout << "Connected to the server at " << server_ip << ":" << tcp_port << std::endl;

        // Frage den Benutzer nach einem Namen
        std::cout << "Enter your name: ";
        std::string name;
        std::getline(std::cin, name);
        this->name = name;
        sendMessage("0000 -- " + name + "|"); // Sende den Namen an den Server

        // Starte einen separaten Thread, um Nachrichten vom Server zu empfangen
        std::thread(&Client::receiveMessages, this).detach();
    }

    setupENetClient(server_ip, udp_port);
}

void Client::setupENetClient(const std::string& server_ip, unsigned short udp_port) {
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet." << std::endl;
        exit(EXIT_FAILURE);
    }

    enet_client = enet_host_create(nullptr, 1, 1, 0, 0);
    if (!enet_client) {
        std::cerr << "An error occurred while trying to create an ENet client host." << std::endl;
        exit(EXIT_FAILURE);
    }

    ENetAddress address;
    enet_address_set_host(&address, server_ip.c_str());
    address.port = udp_port;

    enet_peer = enet_host_connect(enet_client, &address, 1, 0);
    if (!enet_peer) {
        std::cerr << "No available peers for initiating an ENet connection." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::thread(&Client::processENetEvents, this).detach();
}


Client::~Client() {
    if (enet_client) {
        enet_host_destroy(enet_client);
        enet_deinitialize();
    }
}

void Client::sendMessage(const std::string& message) {
    if (socket.send(message.c_str(), message.size()) != sf::Socket::Done) {
        std::cerr << "Error: Unable to send message." << std::endl;
    }
}

void Client::receiveMessages() {
    char buffer[1024];
    std::size_t received;
    while (true) {
        sf::Socket::Status status = socket.receive(buffer, sizeof(buffer), received);
        if (status == sf::Socket::Done) {
            handleTCPMessage(std::string(buffer, received));
        } else if (status == sf::Socket::Disconnected) {
            std::cout << "Disconnected from server." << std::endl;
            break;
        }
    }
}


void Client::sendPositionUpdate(float x, float y) {
    std::string positionUpdate = "0004 -- id=" + std::to_string(id) + "," + std::to_string(x) + "," + std::to_string(y) + "|" ;
    ENetPacket* packet = enet_packet_create(positionUpdate.c_str(), positionUpdate.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(enet_peer, 0, packet);
    enet_host_flush(enet_client);

    //std::cout << "Enemies: " << gameInformation.getEnemies().size() << std::endl;
}

void Client::processENetEvents() {
    while (true) {
        ENetEvent event;
        while (enet_host_service(enet_client, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    //std::cout << "Received UDP message: " << reinterpret_cast<char*>(event.packet->data) << std::endl;
                    handleUDPMessage(reinterpret_cast<char*>(event.packet->data));
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            }
        }
    }
}


void Client::handleTCPMessage(const std::string& socketMessage) {
    std::cout << std::endl << "-----------------------------------" << std::endl;
    std::cout << "Received TCP message: " << socketMessage << std::endl;

    if(socketMessage.find(" -- ") == std::variant_npos){
        std::cout<<"Error: Invalid message format"<<std::endl;
        return;
    }
    int endOfMessageType = socketMessage.find(" -- ");
    std::string type = socketMessage.substr(0, endOfMessageType);
    std::cout << "Type received: " << type << std::endl;
    //Rest der Message wird in neuen String verpackt
    int endOfMessage = socketMessage.find('|');
    std::string message = subString(socketMessage, endOfMessageType + 4, endOfMessage);

    if(type == "0001") {
        std::cout << message << std::endl;
    }
    else if(type == "0002") {
        std::cout << "Received ID: " << message << std::endl;
        id = std::stoi(message);
    }
    else if(type == "0003") {
        std::string id = extractParameter(message, "id");
        std::cout << "id: " << id  << std::endl;
        std::string name = extractParameter(message, "name");
        std::cout << "name: " << name << std::endl;
        if(!gameInformation.enemyExists(std::stoi(id))) {
            gameInformation.addEnemy(EnemyInformation(std::stoi(id), name, sf::Vector2f(0, 0), sf::Vector2f(0, 0)));
        }
    }
    else if(type == "0005") {
        std::string id = extractParameter(message, "id");
        std::cout << "Received remove Enemy: " << id << std::endl;
        gameInformation.removeEnemy(std::stoi(id));
    }
    else if(type == "0006") {
        std::string id = extractParameter(message, "id");
        std::string name = extractParameter(message, "name");
        try {
            EnemyInformation enemy = gameInformation.getEnemy(std::stoi(id));
            enemy.setName(name);
            gameInformation.updateEnemy(enemy);
        } catch(const std::runtime_error& e) {
            std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
            requestGameUpdate();
        }
    }
    else if(type == "0007") {
        std::string id = extractParameter(message, "id");
        std::string health = extractParameter(message, "health");
        try {
            EnemyInformation enemy = gameInformation.getEnemy(std::stoi(id));
            enemy.setHealth(std::stoi(health));
            gameInformation.updateEnemy(enemy);
        } catch(const std::runtime_error& e) {
            std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
            requestGameUpdate();
        }
    }
    else {
        std::cout<<"Error: Unknown message type"<<std::endl;
        std::cout<<socketMessage<<std::endl;
    }

    std::cout << "-----------------------------------" << std::endl;

    if(socketMessage.substr(endOfMessage + 1).find(" -- ") != std::variant_npos) {
        handleTCPMessage(socketMessage.substr(endOfMessage + 1));
    }
}

void Client::handleUDPMessage(const std::string &socketMessage) {
    if(socketMessage.find(" -- ") == std::variant_npos){
        std::cout<<"Error: Invalid message format"<<std::endl;
        return;
    }
    int endOfMessageType = socketMessage.find(" -- ");
    std::string type = socketMessage.substr(0, endOfMessageType);
    //Rest der Message wird in neuen String verpackt
    std::string message = socketMessage.substr(endOfMessageType + 4);

    if(type == "0004") {

        int endOfId = message.find(',');
        std::string id = std::string(&message[message.find("id=") + 3], &message[endOfId]);

        std::string position = message.substr(endOfId + 1);

        sf::Vector2f positionVector = sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1)));

        try {
            gameInformation.updateEnemyPosition(std::stoi(id), positionVector);
        } catch(const std::runtime_error& e) {
            std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
            requestGameUpdate();
        }
    }
    else {
        std::cout<<"Error: Unknown message type"<<std::endl;
        std::cout<<socketMessage<<std::endl;
    }
}

std::string Client::subString(std::string str, int start, int end) {
    return std::string(&str[start], &str[end]);
}

std::string Client::extractParameter(const std::string &message, const std::string &parameter) {
    //extract Parameter Value from message "parameter=value"
    int start = message.find(parameter) + parameter.length() + 1;
    int end = message.find(';', start);
    return message.substr(start, end - start);
}

void Client::requestGameUpdate() {
    sendMessage("1000 -- |");
    std::cout << "Requested game update." << std::endl;
}
