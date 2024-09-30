// server/main.cpp
#include "Server.h"

int main() {
    unsigned short tcpPort = 54000; // Der Port, auf dem der Server lauscht
    unsigned short udpPort = 54001; // Der Port, auf dem der Server UDP-Nachrichten empfängt
    Server server(tcpPort, udpPort);

    server.run(); // Starte den Server und akzeptiere Clients

    return 0;
}
