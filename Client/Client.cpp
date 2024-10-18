// client/Client.cpp
#include "Client.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <variant>
#include <zlib.h>
#include <enet/enet.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "../Game/GameInformation.h"
#include "../Game/MessageType.h"
#include "../Server/Server.h"


enum class MessageType;
enum class MessageTypes;

Client::Client(GameInformation &gameInformation) : gameInformation(gameInformation){
    std::vector<ServerData> serverlist = requestServerList();

    std::cout << "Available servers:" << std::endl;
    for(int i = 0; i < serverlist.size(); i++) {
        std::cout << i << ": " << serverlist[i].getName() << " - " << serverlist[i].getIp() << ":" << serverlist[i].getPort() << std::endl;
    }

    int serverId = -1;
    while(serverId < 0 || serverId >= serverlist.size()) {
        std::cout << "Enter the ID of the server you want to connect to: ";
        std::cin >> serverId;
        std::string dummy;
        std::getline(std::cin, dummy);
    }
    currentServer = serverlist[serverId];
    currentServer = ServerData(currentServer.getId(), "localhost", currentServer.getPort(), currentServer.getName());


    if (socket.connect(currentServer.getIp(), currentServer.getPort()) != sf::Socket::Done) {
        std::cerr << "Error: Unable to connect to the server!" << std::endl;
    } else {
        std::cout << "Connected to the server at " << serverlist[serverId].getIp() << ":" << serverlist[serverId].getPort() << std::endl;

        while(name.empty()) {
            std::cout << "Enter your name: ";
            std::string name;
            std::getline(std::cin, name);
            if(name.length() > 20) {
                std::cout << "Name cannot be longer than 20 characters!" << std::endl;
                continue;
            }
            if(name.empty()) {
                std::cout << "Name cannot be empty!" << std::endl;
                continue;
            }
            if(name.find('|') != std::variant_npos || name.find(';') != std::variant_npos ||
                name.find('-') != std::variant_npos || name.find(' ') != std::variant_npos ||
                name.find('\\') != std::variant_npos || name.find(',') != std::variant_npos ||
                name.find('.') != std::variant_npos || name.find(':') != std::variant_npos) {
                std::cout << "Name cannot contain the characters | ; - \\ , . : & % / and whitespaces!" << std::endl;
                continue;
            }
            this->name = name;
            sendMessage(messageTypeToString(MessageType::SendName) + " -- " + name + "|");
        }

        std::thread(&Client::receiveMessages, this).detach();
    }

    setupENetClient(currentServer.getIp(), currentServer.getPort()+1);
}

void Client::setupENetClient(const std::string& server_ip, unsigned short udp_port) {
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet." << std::endl;
        exit(EXIT_FAILURE);
    }

    enet_client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!enet_client) {
        std::cerr << "An error occurred while trying to create an ENet client host." << std::endl;
        exit(EXIT_FAILURE);
    }

    ENetAddress address;
    enet_address_set_host(&address, server_ip.c_str());
    address.port = udp_port;

    enet_peer = enet_host_connect(enet_client, &address, 2, 0);
    if (!enet_peer) {
        std::cerr << "No available peers for initiating an ENet connection." << std::endl;
        exit(EXIT_FAILURE);
    }

    enet_peer_timeout(enet_peer, 5000, 10000, 0);  // Beispiel-Timeout-Werte: 5000 ms bis zum Timeout, 10000 ms bis zur Trennung.

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
    try {
        char buffer[1024];
        std::size_t received;
        while (true) {
            sf::Socket::Status status = socket.receive(buffer, sizeof(buffer), received);
            if (status == sf::Socket::Done) {
                handleTCPMessage(std::string(buffer, received));
            } else if (status == sf::Socket::Disconnected) {
                std::cout << "Disconnected from server." << std::endl;
                std::cout << "Trying to reconnect..." << std::endl;
                reconnectToServer();
                break;
            }
        }
    }catch(...) {
        std::cout << "Error: Exception in receiveMessages" << std::endl;
    }

}

void Client::reconnectToServer() {
    if (socket.connect(currentServer.getIp(), currentServer.getPort()) != sf::Socket::Done) {
        std::cerr << "Error: Unable to connect to the server!" << std::endl;
    } else {
        std::cout << "Connected to the server at " << currentServer.getIp() << ":" << currentServer.getPort() << std::endl;

        std::thread(&Client::receiveMessages, this).detach();
    }

    setupENetClient(currentServer.getIp(), currentServer.getPort()+1);
}

void Client::sendPositionUpdate(float x, float y) {
    try {
        std::string positionUpdate = messageTypeToString(MessageType::UpdateEnemyPosition) + " -- id=" + std::to_string(id) + ";position=" + std::to_string(x) + "," + std::to_string(y) + ";|" ;
        positionUpdate = positionUpdate + calculate_crc32(positionUpdate) + "|";
        //std::cout << "Sending UDP message: " << positionUpdate << std::endl;
        ENetPacket* packet = enet_packet_create(positionUpdate.c_str(), positionUpdate.length(), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(enet_peer, 0, packet);
        enet_host_flush(enet_client);
    }catch(...) {
        std::cout << "Error: Exception in sendPosition" << std::endl;
    }

    //std::cout << "Enemies: " << gameInformation.getEnemies().size() << std::endl;
}

void Client::sendPositionAndVelocityUpdate(float x, float y, float vx, float vy) {
    try {
        std::string posAndVelUpdate = messageTypeToString(MessageType::UpdateEnemyPositionAndVelocity) + " -- id=" + std::to_string(id) + ";position=" + std::to_string(x) + "," + std::to_string(y) + ";velocity=" + std::to_string(vx) + "," + std::to_string(vy) + ";|" ;
        posAndVelUpdate = posAndVelUpdate + calculate_crc32(posAndVelUpdate) + "|";
        //std::cout << "Sending UDP message: " << posAndVelUpdate << std::endl;
        ENetPacket* packet = enet_packet_create(posAndVelUpdate.c_str(), posAndVelUpdate.length(), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(enet_peer, 0, packet);
        enet_host_flush(enet_client);
    }catch(...) {
        std::cout << "Error: Exception in sendPosition" << std::endl;
    }

    //std::cout << "Enemies: " << gameInformation.getEnemies().size() << std::endl;
}

void Client::sendPlayerUpdate(float x, float y, float vx, float vy, float rotation) {
    try {
        std::string playerUpdate = messageTypeToString(MessageType::UpdateEnemyPosVelRot) + " -- id=" + std::to_string(id) + ";position=" + std::to_string(x) + "," + std::to_string(y) + ";velocity=" + std::to_string(vx) + "," + std::to_string(vy) + ";rotation=" + std::to_string(rotation) + ";|" ;
        playerUpdate = playerUpdate + calculate_crc32(playerUpdate) + "|";
        //std::cout << "Sending UDP message: " << playerUpdate << std::endl;
        ENetPacket* packet = enet_packet_create(playerUpdate.c_str(), playerUpdate.length(), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(enet_peer, 0, packet);
        enet_host_flush(enet_client);
    }catch(...) {
        std::cout << "Error: Exception in sendPosition" << std::endl;
    }

    //std::cout << "Enemies: " << gameInformation.getEnemies().size() << std::endl;
}

void Client::sendBulletShot(int clientId, int bulletId, float x, float y, float vx, float vy) {
    std::string message = messageTypeToString(MessageType::NewBullet) + " -- playerId=" + std::to_string(id) + ";bulletId=" + std::to_string(bulletId) + ";position=" + std::to_string(x) +"," + std::to_string(y) + ";velocity=" + std::to_string(vx) + "," + std::to_string(vy) + ";|";
    if (socket.send(message.c_str(), message.size() + 1) != sf::Socket::Done) {
        std::cerr << "Error: Unable to send message." << std::endl;
    }
}


void Client::processENetEvents() {
    try {
        bool stopThread = false;
        int test = 0;
        while (!stopThread) {
            ENetEvent event;
            while (enet_host_service(enet_client, &event, 1000) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_RECEIVE:
                        //std::cout << "Received UDP message: " << reinterpret_cast<char*>(event.packet->data) << std::endl;
                        handleUDPMessage(reinterpret_cast<char*>(event.packet->data));
                        enet_packet_destroy(event.packet);
                    break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        std::cout << "Disconnected from server." << std::endl;
                        setupENetClient(currentServer.getIp(), currentServer.getPort()+1);
                        enet_packet_destroy(event.packet);
                        stopThread = true;
                        break;
                    case ENET_EVENT_TYPE_CONNECT:
                        std::cout << "Connected to server." << std::endl;
                        enet_packet_destroy(event.packet);
                        break;
                    default:
                        std::cout << "Unknown event type: " << event.type << std::endl;
                        enet_packet_destroy(event.packet);
                        break;
                }
            }
        }
    }catch (...) {
        std::cout << "Error: Exception in processENetEvents" << std::endl;
    }
}


void Client::handleTCPMessage(const std::string& socketMessage) {
    try {
        std::cout << std::endl << "-----------------------------------" << std::endl;
        std::cout << "Received TCP message: " << socketMessage << std::endl;

        if(socketMessage.find(" -- ") == std::variant_npos){
            std::cout<<"Error: Invalid message format - " << socketMessage <<std::endl;
            return;
        }
        int endOfMessageType = socketMessage.find(" -- ");
        //Rest der Message wird in neuen String verpackt
        int endOfMessage = socketMessage.find('|');
        std::string type = subString(socketMessage, 0, endOfMessageType);
        MessageType messageType = stringToMessageType(type);
        std::cout << "Type received: " << type << std::endl;

        std::string message = subString(socketMessage, endOfMessageType + 4, endOfMessage);

        if(messageType == MessageType::Message) {
            std::cout << message << std::endl;
        }
        else if(messageType == MessageType::Id) {
            std::cout << "Received ID: " << message << std::endl;
            id = std::stoi(message);
        }
        else if(messageType == MessageType::NewEnemy) {
            std::string id = extractParameter(message, "id");
            std::cout << "id: " << id  << std::endl;
            std::string name = extractParameter(message, "name");
            std::cout << "name: " << name << std::endl;
            if(!gameInformation.enemyExists(std::stoi(id))) {
                gameInformation.addEnemy(EnemyInformation(std::stoi(id), name, sf::Vector2f(0, 0), sf::Vector2f(0, 0), 0));
            }
        }
        else if(messageType == MessageType::RemoveEnemy) {
            std::string id = extractParameter(message, "id");
            std::cout << "Received remove Enemy: " << id << std::endl;
            gameInformation.removeEnemy(std::stoi(id));
        }
        else if(messageType == MessageType::UpdateEnemyName) {
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
        else if(messageType == MessageType::UpdateEnemyHealth) {
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
        else if(messageType == MessageType::EnemyDied) {//player/enemy died
            std::string id = extractParameter(message, "id");
            if(this->id == std::stoi(id)) {
                gameInformation.setPlayerHealth(0);
                gameInformation.setPlayerPosition(sf::Vector2f(-1000, -1000));
                sendPlayerUpdate(-1000, -1000, 0, 0, 0);
                std::cout << "Player died!" << std::endl;
            } else {
                try {
                    EnemyInformation enemy = gameInformation.getEnemy(std::stoi(id));
                    enemy.setHealth(0);
                    enemy.setPosition(sf::Vector2f(-1000, -1000));
                    gameInformation.updateEnemy(enemy);
                } catch(const std::runtime_error& e) {
                    std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
                    requestGameUpdate();
                }
            }
        }
        else if(messageType == MessageType::NewBullet) {
            std::string position = extractParameter(message, "position");
            std::string velocity = extractParameter(message, "velocity");
            int enemyId = stoi(extractParameter(socketMessage, "playerId"));
            int bulletId = stoi(extractParameter(socketMessage, "bulletId"));

            try {
                Bullet bullet = Bullet(enemyId, bulletId, sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1))),
                                       sf::Vector2f(std::stof(velocity.substr(0, velocity.find(','))), std::stof(velocity.substr(velocity.find(',') + 1))),0 , 100);
                gameInformation.addBullet(bullet);
            } catch(const std::runtime_error& e) {
                std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
            }
        }
        else if(messageType == MessageType::RemoveBullet) {
            int enemyId = stoi(extractParameter(socketMessage, "playerId"));
            int bulletId = stoi(extractParameter(socketMessage, "bulletId"));

            try {
                gameInformation.removeBullet(enemyId, bulletId);
            } catch(const std::runtime_error& e) {
                std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
            }
        }
        else if(messageType == MessageType::EnemyRespawned) {
            std::string id = extractParameter(message, "id");
            std::string position = extractParameter(message, "position");
            try {
                if(stoi(id) == this->id) {
                    gameInformation.setPlayerVelocity(sf::Vector2f(0, 0));
                    gameInformation.setPlayerHealth(100);
                    gameInformation.setPlayerPosition(sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1))));
                    gameInformation.setRespawnRequested(false);
                } else {
                    EnemyInformation enemy = gameInformation.getEnemy(stoi(id));
                    enemy.setVelocity(sf::Vector2f(0, 0));
                    enemy.setHealth(100);
                    enemy.setPosition(sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1))));
                    gameInformation.updateEnemy(enemy);
                }
            } catch(const std::runtime_error& e) {
                std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
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
    }catch(...){
        std::cout << "Error: Exception in handleTCPMessage" << std::endl;
    }
}

void Client::handleUDPMessage(const std::string &socketMessage) {
    try {
        //Checking if message is valid
        if(socketMessage.find(" -- ") == std::variant_npos) {
            std::cout<<"Error: Invalid message format - " << socketMessage << std::endl;
            return;
        }
        if(socketMessage.find('|') == std::variant_npos) {
            std::cout<<"Error: Invalid message format - " << socketMessage << std::endl;
            return;
        }
        int endOfMessagePart = socketMessage.find('|');
        std::string checksum = socketMessage.substr(endOfMessagePart + 1, 8);
        std::string messagePart = socketMessage.substr(0, endOfMessagePart + 1);

        if(calculate_crc32(messagePart) != checksum) {
            std::cout << "Error: Checksum does not match!" << std::endl;
            std::cout << messagePart << " Expected: " << calculate_crc32(messagePart) << " - Received: " << checksum << std::endl << std::endl;
            int endOfMessage = socketMessage.find('|');
            if(socketMessage.substr(endOfMessage + 1).find(" -- ") != std::variant_npos) {
                handleUDPMessage(socketMessage.substr(endOfMessage + 10));
            }
            return;
        }
        //std::cout << messagePart << "  -  Checksum correct." << std::endl;

        int endOfMessageType = socketMessage.find(" -- ");
        std::string type = subString(socketMessage, 0, endOfMessageType);
        MessageType messageType = stringToMessageType(type);

        std::string message = subString(socketMessage, endOfMessageType + 4, endOfMessagePart);


        if(messageType == MessageType::UpdateEnemyPosition) {
            std::string id = extractParameter(message, "id");
            if(id != std::to_string(this->id)) {
                std::string position = extractParameter(message, "position");

                sf::Vector2f positionVector = sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1)));

                try {
                    gameInformation.updateEnemyPosition(std::stoi(id), positionVector);
                } catch(const std::runtime_error& e) {
                    std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
                    requestGameUpdate();
                }
            }
        }
        if(messageType == MessageType::UpdateEnemyPositionAndVelocity) {
            std::string id = extractParameter(message, "id");
            if(id != std::to_string(this->id)) {
                std::string position = extractParameter(message, "position");
                std::string velocity = extractParameter(message, "velocity");
                //std::cout << "Received position and velocity update: " << position << " " << velocity << std::endl;
                sf::Vector2f positionVector = sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1)));
                sf::Vector2f velocityVector = sf::Vector2f(std::stof(velocity.substr(0, velocity.find(','))), std::stof(velocity.substr(velocity.find(',') + 1)));
                try {
                    gameInformation.updateEnemyPositionAndVelocity(std::stoi(id), positionVector, velocityVector);
                } catch(const std::runtime_error& e) {
                    std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
                    requestGameUpdate();
                }
            }
        }
        if(messageType == MessageType::UpdateEnemyPosVelRot) {
            std::string id = extractParameter(message, "id");
            if(id != std::to_string(this->id)) {
                std::string position = extractParameter(message, "position");
                std::string velocity = extractParameter(message, "velocity");
                std::string rotation = extractParameter(message, "rotation");
                //std::cout << "Received position and velocity update: " << position << " " << velocity << std::endl;
                sf::Vector2f positionVector = sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1)));
                sf::Vector2f velocityVector = sf::Vector2f(std::stof(velocity.substr(0, velocity.find(','))), std::stof(velocity.substr(velocity.find(',') + 1)));
                try {
                    gameInformation.updateEnemyData(std::stoi(id), positionVector, velocityVector, std::stof(rotation));
                } catch(const std::runtime_error& e) {
                    std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
                    requestGameUpdate();
                }
            }
        }
        else {
            std::cout<<"Error: Unknown message type"<<std::endl;
            std::cout<<socketMessage<<std::endl;
        }


        int endOfMessage = socketMessage.find('|');
        if(socketMessage.substr(endOfMessage + 1).find(" -- ") != std::variant_npos) {
            handleUDPMessage(socketMessage.substr(endOfMessage + 10));
        }
    } catch (const std::exception& e) {
        std::cout << "Error: Exception in handleUDPMessage" << std::endl;
    }
}

std::string Client::subString(std::string str, int start, int end) {
    try {
        return str.substr(start, end - start);
    } catch(const std::exception& e) {
        std::cerr << "Error in subString: " << e.what() << std::endl;
        return "";
    }
}

std::string Client::extractParameter(const std::string &message, const std::string &parameter) {
    try {
        size_t start = message.find(parameter);
        if (start == std::string::npos) return "";
        start += parameter.length() + 1;

        size_t end = message.find(';', start);
        if (end == std::string::npos) end = message.length();

        return message.substr(start, end - start);
    } catch(const std::exception& e) {
        std::cerr << "Error in extractParameter: " << e.what() << std::endl;
        return "";
    }
}

void Client::requestGameUpdate() {
    sendMessage(messageTypeToString(MessageType::RequestGameUpdate) + " -- |");
    std::cout << "Requested game update." << std::endl;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* outString) {
    size_t totalSize = size * nmemb;
    outString->append((char*)contents, totalSize);
    return totalSize;
}

std::vector<ServerData> Client::requestServerList() {
    std::vector<ServerData> serverlist;
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/serverApi/getServers"); //185.117.249.22
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Verarbeitung der JSON-Daten
        try {
            auto jsonData = nlohmann::json::parse(readBuffer);
            int count = 0;
            for (const auto& item : jsonData) {
                std::string ip = item.at("ip").get<std::string>();
                int port = item.at("port").get<int>();
                std::string name = item.at("name").get<std::string>();
                int id = item.at("id").get<int>();

                serverlist.push_back(ServerData(id, ip, port, name));

            }
        } catch (const std::exception& e) {
            std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
        }

        curl_easy_cleanup(curl);
        return serverlist;
    }
    return serverlist;
}



std::string Client::calculate_crc32(const std::string& data) {
    uint32_t crc = crc32(0L, Z_NULL, 0);  // Initialisiert CRC32
    crc = crc32(crc, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());

    // Umwandlung in Hex-String
    char crc_str[9];  // 8 Stellen + null Terminator
    snprintf(crc_str, sizeof(crc_str), "%08X", crc);  // Hexadezimale Formatierung

    return std::string(crc_str);
}


