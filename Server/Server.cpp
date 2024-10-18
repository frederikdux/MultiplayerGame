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
#include "../Game/MessageType.h"

Server* Server::serverInstance = nullptr;

Server::Server(unsigned short tcp_port, unsigned short udp_port, GameInformation& gameInformation, MapData &mapData) : gameInformation(gameInformation), mapData(mapData){
    registerSignalHandlers();
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
    sf::Socket::Status status = tcpClientSocket.connect("localhost", 50123); //185.117.249.22
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

    std::string setIdMessage = messageTypeToString(MessageType::Id) + " -- " + std::to_string(clientInfo->getId()) + "|";
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
            std::lock_guard<std::mutex> lock(recentUpdatesMutex);
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
    std::cout << "Sent message to clients: " << message << std::endl;
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
    //Rest der Message wird in neuen String verpackt
    int endOfMessage = socketMessage.find('|');
    std::string type = subString(socketMessage, 0, endOfMessageType);
    MessageType messageType = stringToMessageType(type);
    std::cout << "Type received: " << type << std::endl;

    std::string message = subString(socketMessage, endOfMessageType + 4, endOfMessage);


    if(messageType == MessageType::SendName) {
        clientInfo->setName(message); // Setze den Namen des Clients
        broadcastMessage(messageTypeToString(MessageType::UpdateEnemyName) + " -- id=" + std::to_string(clientInfo->getId()) + ";name=" + clientInfo->getName() + ";|", clientInfo);
        EnemyInformation enemy = gameInformation.getEnemy(clientInfo->getId());
        enemy.setName(clientInfo->getName());
        try {
            gameInformation.updateEnemy(enemy);
        } catch (...) {
            std::cout << "Error in acceptClients" << std::endl;
        }
        handleOutput("Client set name to: " + clientInfo->getName());
    }
    else if(messageType == MessageType::Message) {
        broadcastMessage(messageTypeToString(MessageType::Message) + " -- " + clientInfo->getName() + ": " + message, clientInfo);
        handleOutput(clientInfo->getName() + " send Message: " + message);
    }
    else if(messageType == MessageType::RequestGameUpdate) {
        sendGameInformation(clientInfo);
    }
    else if(messageType == MessageType::NewBullet) {
        broadcastMessage(socketMessage, nullptr);
        std::string position = extractParameter(socketMessage, "position");
        sf::Vector2f pos = sf::Vector2f(std::stof(subString(position, 0, position.find(','))), std::stof(subString(position, position.find(',') + 1, position.length())));
        std::string velocity = extractParameter(socketMessage, "velocity");
        sf::Vector2f vel = sf::Vector2f(std::stof(subString(velocity, 0, velocity.find(','))), std::stof(subString(velocity, velocity.find(',') + 1, velocity.length())));
        int bulletId = stoi(extractParameter(socketMessage, "bulletId"));
        gameInformation.addBullet(Bullet(clientInfo->getId(), bulletId, pos, vel, 0, 0));
    }
    else if(messageType == MessageType::RequestRespawn) {
        int id = stoi(extractParameter(message, "id"));
        sf::Vector2f position = calculateRandomSpawnpoint(clientInfo);
        broadcastMessage(messageTypeToString(MessageType::EnemyRespawned) + " -- id="+ std::to_string(id) + ";position=" + std::to_string(position.x) + "," + std::to_string(position.y) + ";|", nullptr);
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

    enet_server = enet_host_create(&address, 64, 2, 0, 0);
    if (!enet_server) {
        handleError("An error occurred while trying to create an ENet server host.");
        exit(EXIT_FAILURE);
    }

    handleOutput("ENet server is listening on UDP port " + std::to_string(udp_port) + "...");
}

void Server::processENetEvents() {
    while(true) {
        ENetEvent event;
        while (enet_host_service(enet_server, &event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    //std::cout << "Received packet from ENet client." << std::endl;
                    std::string receivedData(reinterpret_cast<char*>(event.packet->data), event.packet->dataLength);

                    try {
                        int playerId = std::stoi(extractParameter(receivedData, "id"));
                        //std::cout << "Received position update from player " << playerId << ": " << receivedData << std::endl;

                        std::lock_guard<std::mutex> lock(recentUpdatesMutex);
                        recentUpdates[playerId] = receivedData;
                    } catch (const std::invalid_argument& e) {
                        // Falls die ID kein gültiger Integer ist
                        std::cerr << "Invalid ID received: " << extractParameter(receivedData, "id") << std::endl;
                    } catch (const std::out_of_range& e) {
                        // Falls die ID außerhalb des gültigen Integer-Bereichs liegt
                        std::cerr << "ID out of range: " << extractParameter(receivedData, "id") << std::endl;
                    }
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    handleOutput("Client disconnected from ENet server.");
                    break;
                }
                case ENET_EVENT_TYPE_CONNECT: {
                    handleOutput("Client connected to ENet server.");
                    break;
                }
                default:
                    std::cout << "Unknown event type: " << event.type << std::endl;
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
            //std::cout << "sending info of " << recentUpdates.size() << " players" << std::endl;
            std::unordered_map<int, std::string> recentUpdatesCopy;
            {
                std::lock_guard<std::mutex> lock(recentUpdatesMutex);
                recentUpdatesCopy = recentUpdates;
            }
            for (const auto& [id, message] : recentUpdatesCopy) {
                batchMessage += message.c_str();
                int endOfMessageType = message.find(" -- ");
                std::string type = subString(message, 0, endOfMessageType);
                MessageType messageType = stringToMessageType(type);

                if(messageType == MessageType::UpdateEnemyPosVelRot) {
                    std::string id = extractParameter(message, "id");
                    std::string position = extractParameter(message, "position");
                    //std::cout << id << ", ";
                    sf::Vector2f positionVector = sf::Vector2f(std::stof(position.substr(0, position.find(','))), std::stof(position.substr(position.find(',') + 1)));
                    try {
                        gameInformation.updateEnemyData(std::stoi(id), positionVector, sf::Vector2f(0,0), 0);
                    } catch(const std::exception& e) {
                        std::cerr <<"Fehler aufgetreten:" << e.what() << std::endl;
                    }
                }
            }
            //std::cout << std::endl;
            if(batchMessage.empty()) {
                continue;
            }

            //std::cout << "Sending batch message to ENet client." << std::endl;
            for (size_t i = 0; i < enet_server->peerCount; ++i) {
                ENetPeer* currentPeer = &enet_server->peers[i];
                if (currentPeer->state == ENET_PEER_STATE_CONNECTED) {
                    ENetPacket* packet = enet_packet_create(batchMessage.c_str(), batchMessage.length() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
                    enet_peer_send(currentPeer, 0, packet);
                }
                else if(currentPeer->state != 0){
                    std::cout << "Peer state: " << currentPeer->state << std::endl;
                }
            }

            lastTick = now;
            batchMessage.clear();
            std::lock_guard<std::mutex> lock(recentUpdatesMutex);
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
            std::string message = messageTypeToString(MessageType::NewEnemy) + " -- id=" + std::to_string(client->getId()) + ";name=" + client->getName() + ";|" ;
            clientInfo->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::broadcastNewPlayerJoined(ClientInfo *newPlayer) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(client->getId() != newPlayer->getId() && !client->getName().empty()) {
            std::string message = messageTypeToString(MessageType::NewEnemy) + " -- id=" + std::to_string(newPlayer->getId()) + ";name= " + newPlayer->getName() + ";|";
            client->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::broadcastPlayerLeft(ClientInfo *player) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(client->getId() != player->getId() && !client->getName().empty()) {
            std::string message = messageTypeToString(MessageType::RemoveEnemy) + " -- id=" + std::to_string(player->getId()) +";|";
            client->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::broadcastPlayerDied(EnemyInformation *enemy) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if(!client->getName().empty()) {
            std::string message = messageTypeToString(MessageType::EnemyDied) + " -- id=" + std::to_string(enemy->getId()) +";|";
            client->getSocket()->send(message.c_str(), message.size());
        }
    }
}

void Server::sendToPlayer(const std::string &message, ClientInfo *player) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    player->getSocket()->send(message.c_str(), message.size());
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
            gameInformation.updateBulletPositions(clock.restart().asSeconds(), mapData);

            std::vector<Bullet> toBeDeleted;

            for (auto & bullet : gameInformation.getBullets()) {
                for (auto & enemy : gameInformation.getEnemies()) {
                    double xDiff = bullet.getPosition().x - enemy.getPosition().x;
                    double yDiff = bullet.getPosition().y - enemy.getPosition().y;
                    double distance = std::sqrt(xDiff * xDiff + yDiff * yDiff);

                    if (distance < 24) {
                        std::cout << "Player " << enemy.getName() <<" got hit!" << std::endl;
                        broadcastPlayerDied(&enemy);
                        std::cout << "Enemy with id " << enemy.getId() << " got hit!" << std::endl;
                        enemy.setPosition(sf::Vector2f(-1000, -1000));
                        std::cout << "Enemy with id " << enemy.getId() << " set to -1000, -1000!" << std::endl;
                        enemy.setHealth(0);
                        std::cout << "Enemy with id " << enemy.getId() << " health set to 0!" << std::endl;
                        std::lock_guard<std::mutex> lock(recentUpdatesMutex);
                        if(recentUpdates.find(enemy.getId()) != recentUpdates.end()) {
                            recentUpdates.erase(enemy.getId());
                            std::cout << "Enemy with id " << enemy.getId() << " erased!" << std::endl;
                        }
                        std::cout << "Enemy with id " << enemy.getId() << " removed from recentUpdates!" << std::endl;
                        gameInformation.updateEnemy(enemy);
                        std::cout << "Enemy with id " << enemy.getId() << " updated!" << std::endl;

                        toBeDeleted.push_back(bullet);
                    }
                }
            }

            for(auto bullet1 : gameInformation.getBullets()) {
                for(auto bullet2 : gameInformation.getBullets()) {
                    if(bullet1.getPlayerId() != bullet2.getPlayerId()) {
                        double xDiff = bullet1.getPosition().x - bullet2.getPosition().x;
                        double yDiff = bullet1.getPosition().y - bullet2.getPosition().y;
                        double distance = std::sqrt(xDiff * xDiff + yDiff * yDiff);

                        if (distance < 15) {
                            toBeDeleted.push_back(bullet1);
                            toBeDeleted.push_back(bullet2);
                        }
                    }
                }
            }

            for(auto bullet : toBeDeleted) {
                gameInformation.removeBullet(bullet.getPlayerId(), bullet.getBulletIdOfPlayer());
                broadcastMessage(messageTypeToString(MessageType::RemoveBullet) + " -- playerId=" + std::to_string(bullet.getPlayerId()) + ";bulletId=" + std::to_string(bullet.getBulletIdOfPlayer()) + ";|", nullptr);
                std::cout << "Bullet removed!" << std::endl;
            }
        } catch (std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}

sf::Vector2f Server::calculateRandomSpawnpoint(ClientInfo *clientInfo) {
    int random = rand() % mapData.getSpawnpoints().size();
    sf::Vector2f spawnLocation = mapData.getSpawnpoints()[random].getPosition();
    return spawnLocation;
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

void Server::signalHandler(int signal) {
    std::cerr << "Error: Signal " << signal << " received." << std::endl;
    // Hier könntest du die Logik zum Beenden hinzufügen
    exit(signal);
}

void Server::
registerSignalHandlers() {
    std::signal(SIGSEGV, signalHandler);  // Segmentation fault
    std::signal(SIGABRT, signalHandler);  // Abort signal from abort()
    // Weitere Signale hinzufügen, falls nötig
}
