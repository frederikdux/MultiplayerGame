// server/main.cpp
#include <iostream>

#include "Server.h"
#include "../Game/GameInformation.h"

int main() {
    unsigned short tcpPort = 54000; // Der Port, auf dem der Server lauscht
    unsigned short udpPort = 54001; // Der Port, auf dem der Server UDP-Nachrichten empfängt
    GameInformation gameInformation; // Die Informationen über das Spiel
    std::string version = "v1.0.0"; // Die Version des Servers
    std::cout << "Starting server with version " << version << " on TCP port " << tcpPort << " and UDP port " << udpPort << std::endl;
    Server server(tcpPort, udpPort, gameInformation);
    server.run(); // Starte den Server und akzeptiere Clients

    return 0;
}