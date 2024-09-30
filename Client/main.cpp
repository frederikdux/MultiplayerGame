// client/main.cpp
#include <SFML/Graphics.hpp>
#include "Client.h"
#include <iostream>
#include "Game/GameInformation.h"

using namespace sf;

int main() {
    std::string server_ip = "127.0.0.1"; // IP-Adresse des Servers (localhost)
    unsigned short tcpPort = 54000; // Der Port, auf dem der Server lauscht
    unsigned short udpPort = 54001;      // Port des Servers

    GameInformation gameInformation;

    boolean focus = true;

    RenderWindow window(VideoMode(600, 600), "SFML");
    window.setFramerateLimit(60);

    RectangleShape shape(Vector2f(20, 20));
    shape.setFillColor(Color::Red);
    shape.setPosition(100, 100);

    Client client(server_ip, tcpPort, udpPort, gameInformation);

    Vector2f playerPosition = Vector2f(100, 100);

    Font font;
    font.loadFromFile("arial.ttf");

    Text nameText;
    nameText.setFont(font);
    nameText.setCharacterSize(24);
    nameText.setFillColor(Color::White);




    /*
    std::string message;
    while (true) {
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        client.sendMessage("0001 -- " + message);
    }*/

    while (window.isOpen())
    {
        if(client.getId() != -1) {
            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == Event::Closed) {
                    window.close();
                }
                if(event.type == Event::GainedFocus) {
                    focus = true;
                }
                if(event.type == Event::LostFocus) {
                    focus = false;
                }
            }

            if(focus) {
                if(Keyboard::isKeyPressed(Keyboard::W)) {
                    playerPosition.y -= 1;
                }
                if(Keyboard::isKeyPressed(Keyboard::S)) {
                    playerPosition.y += 1;
                }
                if(Keyboard::isKeyPressed(Keyboard::A)) {
                    playerPosition.x -= 1;
                }
                if(Keyboard::isKeyPressed(Keyboard::D)) {
                    playerPosition.x += 1;
                }
                if(Keyboard::isKeyPressed(Keyboard::Subtract)) {
                    ShowWindow(GetConsoleWindow(), SW_HIDE);
                }
                if(Keyboard::isKeyPressed(Keyboard::Add)) {
                    ShowWindow(GetConsoleWindow(), SW_SHOW);
                }
            }

            window.clear();

            shape.setFillColor(Color::Green);
            shape.setPosition(playerPosition);
            window.draw(shape);
            client.sendPositionUpdate(playerPosition.x, playerPosition.y);
            window.setTitle(client.getSocket().getRemoteAddress().toString() + " - " + client.getName() + " - health: " + std::to_string(client.getHealth()) +" id:" + std::to_string(client.getId()));

            for (EnemyInformation enemy : gameInformation.getEnemies()) {
                shape.setFillColor(Color::Red);
                shape.setPosition(enemy.getPosition());
                nameText.setString(enemy.getName());
                nameText.setPosition(enemy.getPosition().x, enemy.getPosition().y - 20);
                window.draw(shape);
                window.draw(nameText);
            }

            window.display();
        }
    }

    return 0;
}

