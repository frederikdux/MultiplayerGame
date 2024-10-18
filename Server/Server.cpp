// server/Server.cpp
#include "Server.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <variant>
#include <fcntl.h>
#include <csignal>
#include <unistd.h>
#include "../Game/GameInformation.h"
#include "../Game/EnemyInformation.h"

Server* Server::serverInstance = nullptr;

Server::Server(unsigned short tcp_port, unsigned short udp_port, GameInformation& gameInformation) : gameInformation(gameInformation){
    serverInstance = this;
    port = tcp_port;
    //Output frage nach name des servers und danach input für den namen
    std::cout << "Enter the name of the server: ";
    std::getline(std::cin, serverName);
    std::cout << std::endl;
    if (listener.listen(tcp_port) != sf::Socket::Done) {
        std::cerr << "Error: Cannot listen on port " << tcp_port << std::endl;
        return;
    }
    std::cout << "Server is listening on port " << tcp_port << "..." << std::endl;
    std::set_terminate(exceptionHandler);
    //registerServer();
    runTcpClient();

    setupENetServer(udp_port);
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


void Server::runTcpClient() {
    // Verbindung zum übergeordneten Server herstellen
    sf::Socket::Status status = tcpClientSocket.connect("localhost", 50123);
    if (status != sf::Socket::Done)
    {
        std::cerr << "Error connecting to the DistributionServer" << std::endl;
        return;
    }

    std::thread tcpClientReceiveThread(&Server::receiveMessagesAsTCPClient, this);
    tcpClientReceiveThread.detach();

    std::thread tcpClientSendUpdateThread(&Server::sendUpdatesToDServer, this);
    tcpClientSendUpdateThread.detach();

    sendMessageAsTCPClient("type=registerNewServer;name=" + serverName + ";port=" + std::to_string(port));
}

void Server::receiveMessagesAsTCPClient() {
    try {
        char buffer[1024];
        std::size_t received;
        while (true) {
            sf::Socket::Status status = tcpClientSocket.receive(buffer, sizeof(buffer), received);
            if (status == sf::Socket::Done) {
                std::string message(buffer, received);
                std::cout << "Received message from server: " << message << std::endl;
            } else if (status == sf::Socket::Disconnected) {
                std::cout << "Disconnected from server." << std::endl;
                break;
            }
        }
    }catch(...) {
        std::cout << "Error: Exception in receiveMessages" << std::endl;
    }
}

void Server::sendMessageAsTCPClient(const std::string& message) {
    std::string messageWithNewline = message + "\n";  // Zeilenumbruch hinzufügen
    if (tcpClientSocket.send(messageWithNewline.c_str(), messageWithNewline.size()) != sf::Socket::Done) {
        std::cerr << "Error: Unable to send message." << std::endl;
    }
}

void Server::sendUpdatesToDServer() {
    while(true) {
        sendMessageAsTCPClient("type=sendPlayerCount;playerCount=" + std::to_string(clients.size()));
        sf::sleep(sf::seconds(5));
    }
}


Server::~Server() {
    if (enet_server) {
        enet_host_destroy(enet_server);
        enet_deinitialize();
    }
}

void Server::run() {
    try {
        std::thread accept_thread(&Server::acceptClients, this);
        accept_thread.detach(); // Startet den Thread für die Akzeptanz von Clients

        std::thread enet_thread(&Server::processENetEvents, this);
        enet_thread.detach();

        std::thread sendUpdates_thread(&Server::sendBatchedPositionUpdates, this);
        sendUpdates_thread.detach();

        std::thread input_thread(&Server::handleInput, this);
        input_thread.detach();

        std::thread checkHits_thread(&Server::checkForHits, this);
        checkHits_thread.detach();

        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

        // Den aktuellen Konsolenmodus speichern
        DWORD prev_mode;
        GetConsoleMode(hInput, &prev_mode);

        // Den Konsolenmodus so ändern, dass keine Eingaben zugelassen werden
        SetConsoleMode(hInput, 0);

        // Hauptthread bleibt aktiv, um Nachrichten zu verarbeiten
        while (true) {
            if (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000 && !consoleButtonPressed) {
                consoleButtonPressed = true;
                if(outputAllowed) {
                    std::cout << "Console open!" << std::endl;
                    FlushConsoleInputBuffer(hInput);
                    SetConsoleMode(hInput, prev_mode);
                    std::cin.clear();
                } else {
                    SetConsoleMode(hInput, 0);
                    FlushConsoleInputBuffer(hInput);
                    std::cin.setstate(std::ios::failbit);
                    std::cout << "Console closed!" << std::endl;

                }
                outputAllowed = !outputAllowed;
            }
            else if(!(GetAsyncKeyState(VK_OEM_PLUS) & 0x8000)){
                //std::cout << GetAsyncKeyState(0x41) << " " << 0x8000 << " " << (GetAsyncKeyState(0x41) & 0x8000) << " " << !consoleButtonPressed << std::endl;
                consoleButtonPressed = false;
            }
            sf::sleep(sf::milliseconds(100)); // Vermeidet Überlastung der CPU
        }
    } catch (...) {
        std::cerr << "Exception caught in run(). Logging out server..." << std::endl;
        //logoutServer();  // Hier wird logoutServer aufgerufen, wenn eine Exception auftritt.
    }
}

void Server::acceptClients() {
    while (true) {
        auto socket = std::make_unique<sf::TcpSocket>();
        if (listener.accept(*socket) == sf::Socket::Done) {
            handleOutput("New client connected: " + socket->getRemoteAddress().toString() + " id=" + std::to_string(nextId));
            ++nextId;
            try {
                gameInformation.addEnemy(EnemyInformation(nextId - 1, "Enemy " + std::to_string(nextId - 1), sf::Vector2f(0, 0), sf::Vector2f(0, 0), 0));
            } catch (...) {
                std::cout << "Error in acceptClients" << std::endl;
            }

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
            //handleOutput("Received message from client: " + message);
            handleMessage(message, clientInfo);
        } else if (status == sf::Socket::Disconnected) {
            handleOutput("Client disconnected: id=" + std::to_string(clientInfo->getId()) + " " + clientInfo->getName());
            gameInformation.removeEnemy(clientInfo->getId());
            recentUpdates.erase(clientInfo->getId());
            broadcastPlayerLeft(clientInfo);
            removeClient(clientInfo);
            break;
        }
    }
}

void Server::broadcastMessage(const std::string& message, ClientInfo* sender) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if ((sender == nullptr || client.get() != sender) && !client->getName().empty()) {
            if (client->getSocket()->send(message.c_str(), message.size()) != sf::Socket::Done) {
                handleError("Error: Could not send message to a client.");
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
        handleOutput("Error: Invalid message format");
        return;
    }
    int endOfMessageType = socketMessage.find(" -- ");
    std::string type = socketMessage.substr(0, endOfMessageType);
    int endOfMessage = socketMessage.find("|");
    std::string message = subString(socketMessage, endOfMessageType + 4, endOfMessage);

    if(type == "0000") {
        clientInfo->setName(message); // Setze den Namen des Clients
        broadcastMessage("0006 -- id=" + std::to_string(clientInfo->getId()) + ";name=" + clientInfo->getName() + ";|", clientInfo);
        EnemyInformation enemy = gameInformation.getEnemy(clientInfo->getId());
        enemy.setName(clientInfo->getName());
        try {
            gameInformation.updateEnemy(enemy);
        } catch (...) {
            std::cout << "Error in acceptClients" << std::endl;
        }
        handleOutput("Client set name to: " + clientInfo->getName());
    }
    else if(type == "0001") {
        broadcastMessage("0001 -- " + clientInfo->getName() + ": " + message, clientInfo);
        handleOutput(clientInfo->getName() + " send Message: " + message);
    }
    else if(type == "1000") {
        sendGameInformation(clientInfo);
    }
    else if(type == "0201") {
        broadcastMessage(socketMessage, nullptr);
        std::string position = extractParameter(socketMessage, "position");
        sf::Vector2f pos = sf::Vector2f(std::stof(subString(position, 0, position.find(','))), std::stof(subString(position, position.find(',') + 1, position.length())));
        std::string velocity = extractParameter(socketMessage, "velocity");
        sf::Vector2f vel = sf::Vector2f(std::stof(subString(velocity, 0, velocity.find(','))), std::stof(subString(velocity, velocity.find(',') + 1, velocity.length())));

        gameInformation.addBullet(Bullet(pos, vel, 0, 0));
    }
    else {
        handleError("Error: Unknown message type");
        handleOutput(socketMessage);
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
        handleError("An error occurred while trying to create an ENet server host.");
        exit(EXIT_FAILURE);
    }

    handleOutput("ENet server is listening on UDP port " + std::to_string(udp_port) + "...");
}

void Server::processENetEvents() {
    while(true) {
        ENetEvent event;
        while (enet_host_service(enet_server, &event, 10) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    std::string receivedData(reinterpret_cast<char*>(event.packet->data), event.packet->dataLength);

                    // Extract position and player ID from the message
                    int playerId = std::stoi(extractParameter(receivedData, "id"));
                    //std::cout << "Received position update from player " << playerId << ": "<< receivedData << std::endl;

                    // Store the player's position
                    recentUpdates[playerId] = receivedData;

                    enet_packet_destroy(event.packet);
                    break;
                }

                default:
                    break;
            }
        }
    }
}

void Server::sendBatchedPositionUpdates() {
    auto tickDuration = std::chrono::milliseconds((1000/tickrate)); // 60 ticks per second = 1000ms / 60 = 16.67ms
    auto lastTick = std::chrono::high_resolution_clock::now();
    std::string batchMessage;
    while(true) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick);
        tickDuration = std::chrono::milliseconds((1000/tickrate));
        if (elapsed >= tickDuration) {
            for (const auto& [id, message] : recentUpdates) {
                batchMessage += message.c_str();
                int endOfMessageType = message.find(" -- ");
                std::string type = message.substr(0, endOfMessageType);

                if(type == "0106") {
                    std::string id = extractParameter(message, "id");
                    std::string position = extractParameter(message, "position");
                    sf::Vector2f positionVector = sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1)));
                    try {
                        try {
                            gameInformation.updateEnemyData(std::stoi(id), positionVector, sf::Vector2f(0,0), 0);
                        } catch (...) {
                            std::cout << "Error in acceptClients" << std::endl;
                        }
                    } catch(const std::runtime_error& e) {
                        std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
                    }
                }
            }
            if(batchMessage.empty()) {
                continue;
            }

            for (size_t i = 0; i < enet_server->peerCount; ++i) {
                ENetPeer* currentPeer = &enet_server->peers[i];
                if (currentPeer->state == ENET_PEER_STATE_CONNECTED) {
                    ENetPacket* packet = enet_packet_create(batchMessage.c_str(), batchMessage.length() + 1, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(currentPeer, 0, packet);
                }
            }

            lastTick = now;
            batchMessage.clear();
            recentUpdates.clear();
        }

        // Optionally sleep for a short time to reduce CPU usage if the server is idle
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
            std::string message = "0005 -- id=" + std::to_string(player->getId()) +";|";
            client->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::broadcastPlayerDied(EnemyInformation *enemy) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(!client->getName().empty()) {
            std::string message = "0109 -- id=" + std::to_string(enemy->getId()) +";|";
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

void Server::exceptionHandler() {
    std::cerr << "An error occurred." << std::endl;
    //getServerInstance()->logoutServer();
    abort();
}

void Server::logoutServer() {
    // Initialisiere libCurl
    CURL *curl;
    CURLcode res;

    // URL zu der du die POST-Anfrage sendest
    std::string url = dServerAddr + "/serverApi/removeServer?id=" + std::to_string(serverId);

    // Initialisiere Curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Setze die URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Setze die POST-Daten
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

        // Optional: Setze den User-Agent oder andere Header (falls benötigt)
        // curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // Führe den POST-Request aus
        res = curl_easy_perform(curl);

        // Überprüfe auf Fehler
        if (res != CURLE_OK) {
            handleError("POST Request failed: " + std::string(curl_easy_strerror(res)));
        } else {
            handleOutput("POST Request erfolgreich!");
        }

        // Bereinige den curl Handler
        curl_easy_cleanup(curl);
    }

    // Beende libCurl
    curl_global_cleanup();
}

void Server::handleOutput(const std::string &message) {
    if(outputAllowed) {
        std::cout << message << std::endl;
    }
}

void Server::handleError(const std::string &message) {
    if(outputAllowed) {
        std::cerr << message << std::endl;
    }
}

void Server::handleInput() {
    while(true) {
        if(!outputAllowed) {
            std::string input;
            if(std::getline(std::cin, input)) {
                std::cout << "Received input: " << input << std::endl;
                if(input == "/exit") {
                    std::cout << "Logging out server..." << std::endl;
                    //logoutServer();
                    exit(0);
                }

                if(input.find("/tickrate") != std::string::npos) {
                    tickrate = std::stoi(input.substr(input.find("/tickrate") + 10));
                    std::cout << "Tickrate set to " << tickrate << std::endl;
                }
            }
        }
        Sleep(100);
    }
}


void Server::checkForHits() {
    sf::Clock clock;
    while(true) {
        try {
            gameInformation.updateBulletPositions(clock.restart().asSeconds(), MapData());

            std::vector<Bullet> bullets = gameInformation.getBullets();
            std::vector<EnemyInformation> enemies = gameInformation.getEnemies();
            for (Bullet bullet : bullets) {
                for (EnemyInformation enemy : enemies) {
                    double xDiff = bullet.getPosition().x - enemy.getPosition().x;
                    double yDiff = bullet.getPosition().y - enemy.getPosition().y;
                    double distance = std::sqrt(xDiff * xDiff + yDiff * yDiff);

                    if (distance < 30) {
                        std::cout << "Player " << enemy.getName() <<" got hit!" << std::endl;
                        broadcastPlayerDied(&enemy);
                        enemy.setPosition(sf::Vector2f(-1000, -1000));
                        enemy.setHealth(0);
                        recentUpdates[enemy.getId()].erase();
                        gameInformation.updateEnemy(enemy);
                        gameInformation.removeBulletByAddress(&bullet);
                    }
                }
            }
        } catch (std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}


//Linux
/*
void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

bool kbhit() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }
    return false;
}
*/