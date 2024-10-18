// server/main.cpp
#include <iostream>

#include "Server.h"
#include <iostream>
#include "../Game/GameInformation.h"
#include <fstream>
#include <sstream>

void loadMap(const std::string &map, MapData &mapData);

int main() {
    unsigned short tcpPort = 54000; // Der Port, auf dem der Server lauscht
    unsigned short udpPort = 54001; // Der Port, auf dem der Server UDP-Nachrichten empfängt
    GameInformation gameInformation; // Die Informationen über das Spiel
    MapData mapData;
    std::string version = "v1.0.0"; // Die Version des Servers
    std::cout << "Starting server with version " << version << " on TCP port " << tcpPort << " and UDP port " << udpPort << std::endl;
    loadMap("TestMap.txt", mapData);
    Server server(tcpPort, udpPort, gameInformation, mapData);
    server.run(); // Starte den Server und akzeptiere Clients

    return 0;
}

void loadMap(const std::string &map, MapData &mapData) {
    std::ifstream mapFile(map);
    std::string parameterLine;

    if (mapFile.is_open()) {
        // Lese die erste Zeile aus der Datei
        if (std::getline(mapFile, parameterLine)) {
            std::cout << "Erste Zeile: " << parameterLine << std::endl;
        } else {
            std::cerr << "Fehler beim Lesen der ersten Zeile!" << std::endl;
            return;
        }

        // Lese die nächsten 30 Zeilen mit jeweils 30 Zeichen
        for (int i = 0; i < 20; ++i) {
            std::string zeile;
            if (std::getline(mapFile, zeile)) {
                std::istringstream iss(zeile);
                for (int j = 0; j < 20; ++j) {
                    char c;
                    if (iss >> c) {
                        switch (c) {
                            case '0':
                                break;
                            case '1':
                                mapData.addBasicWall(BasicWall(sf::Vector2f(j * 40, i * 40), sf::Vector2f(40, 40)));
                            break;
                            case '2':
                                mapData.addSpawnpoint(Spawnpoint(sf::Vector2f(j * 40, i * 40)));
                            break;
                            default:
                                std::cerr << "Ungültiges Zeichen in Zeile " << i + 1 << " und Spalte " << j + 1 << std::endl;
                            return;
                        }
                    } else {
                        std::cerr << "Fehler beim Lesen der Zeichen in Zeile " << i + 1 << std::endl;
                        return;
                    }
                }
            } else {
                std::cerr << "Fehler beim Lesen der Zeile " << i + 1 << std::endl;
                return;
            }
        }

        mapFile.close();
    } else {
        std::cerr << "Die Datei konnte nicht geöffnet werden!" << std::endl;
        return;
    }
}