// client/main.cpp
#include <cmath>
#include <SFML/Graphics.hpp>
#include "Client.h"
#include <iostream>
#include <json.hpp>
#include <fstream>
#include <thread>

#include "../Game/GameInformation.h"
#include "../Game/MapData.h"
#include "../Game/MessageType.h"

#define PI 3.14159265359f

using namespace sf;

Vector2f moveTowards(const sf::Vector2f& current, const sf::Vector2f& target, float deltaTime, float speed, float minDistanceDelta);
void loadMap(const std::string &map, MapData &mapData);
sf::Vector2f movePlayerUntilCollision(sf::Vector2f& currentPosition, sf::Vector2f& velocity, const Vector2f& nextPosition, const sf::FloatRect& wallBounds);
void resolveCollision(GameInformation& gameInformation, const sf::RectangleShape& wall);
void updatePlayer(GameInformation &gameInformation, float deltaTime, const std::vector<BasicWall>& walls);

void networkFunctions(GameInformation &gameInformation, MapData &mapData, Client &client) {
    Clock clock;
    float timeCounter = 0;
    float deltaTime = 0;
    while(true) {
        try {
            deltaTime = clock.restart().asSeconds();

            timeCounter += deltaTime;
            if(timeCounter >= 0.0125) {
                timeCounter = 0;
                client.sendPlayerUpdate(gameInformation.getPlayerPosition().x, gameInformation.getPlayerPosition().y, gameInformation.getPlayerVelocity().x, gameInformation.getPlayerVelocity().y, gameInformation.getPlayerRotation());
            }

            gameInformation.updateEnemyPositions(deltaTime);
            gameInformation.updateBulletPositions(deltaTime, mapData);
            updatePlayer(gameInformation, deltaTime, mapData.getBasicWalls());
        } catch (const std::exception& e) {
            std::cout << "Error in networkFunctions: " << e.what() << std::endl;
        }
    }
}

int main() {
    GameInformation gameInformation;
    gameInformation.setPlayerHealth(100);
    gameInformation.setPlayerPosition(Vector2f(100, 100));
    gameInformation.setPlayerRotation(0);
    gameInformation.setPlayerVelocity(Vector2f(0, 0));
    MapData mapData = MapData();

    boolean focus = true;
    boolean withVelocity = true;
    boolean velocityPressed = false;

    boolean shotBullet = false;

    Client client(gameInformation);
    loadMap("TestMap.txt", mapData);

    RenderWindow window(VideoMode(800, 800), "SFML", Style::Titlebar | Style::Close);
    window.setFramerateLimit(100);

    RectangleShape shape(Vector2f(20, 20));
    shape.setFillColor(Color::Red);
    shape.setOrigin(shape.getSize().x/2, shape.getSize().y/2);
    shape.setPosition(100, 100);
    Vertex line[] = {
            Vertex(Vector2f(0, 0)),
            Vertex(Vector2f(0, 0))
    };

    CircleShape circle(3);
    circle.setFillColor(Color::Blue);
    circle.setOrigin(3, 3);

    RectangleShape basicWallRec(Vector2f(40, 40));
    basicWallRec.setFillColor(Color::Yellow);

    RectangleShape playerPositionRec(Vector2f(1, 1));

    float maxSpeed = 200;
    float acceleration = 150;

    Font font;
    font.loadFromFile("arial.ttf");

    Text nameText;
    nameText.setFont(font);
    nameText.setCharacterSize(18);
    nameText.setFillColor(Color::White);

    Text respawnText;
    respawnText.setFont(font);
    respawnText.setCharacterSize(25);
    respawnText.setFillColor(Color::Black);
    respawnText.setString("Press R to respawn");
    respawnText.setOrigin(respawnText.getLocalBounds().width / 2, respawnText.getLocalBounds().height / 2);
    respawnText.setPosition(window.getSize().x / 2, 18);

    Clock clock;
    int timeCounter=0;

    boolean positionUpdated = false;

    std::thread networkThread(networkFunctions, std::ref(gameInformation), std::ref(mapData), std::ref(client));
    networkThread.detach();

    try {
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

                float deltaTime = clock.restart().asSeconds();

                if(focus) {
                    if(Keyboard::isKeyPressed(Keyboard::W)) {
                        float angleRad = gameInformation.getPlayerRotation() * PI / 180;
                        Vector2f targetVelocity = Vector2f(cos(angleRad) * maxSpeed, sin(angleRad) * maxSpeed);
                        Vector2f velocity = moveTowards(gameInformation.getPlayerVelocity(), targetVelocity, deltaTime, acceleration, 0.1f);
                        gameInformation.setPlayerVelocity(velocity);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::S)) {
                        float angleRad = gameInformation.getPlayerRotation() * PI / 180;
                        Vector2f targetVelocity = Vector2f(cos(angleRad) * -maxSpeed, sin(angleRad) * -maxSpeed);
                        Vector2f velocity = moveTowards(gameInformation.getPlayerVelocity(), targetVelocity, deltaTime, acceleration, 0.1f);
                        gameInformation.setPlayerVelocity(velocity);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::LShift)) {
                        acceleration = 400;
                        maxSpeed = 350;
                    } else {
                        acceleration = 150;
                        maxSpeed = 200;
                    }
                    if(Keyboard::isKeyPressed(Keyboard::A)) {
                        gameInformation.increasePlayerRotation(-200 * deltaTime);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::D)) {
                        gameInformation.increasePlayerRotation(200 * deltaTime);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::Subtract)) {
                        ShowWindow(GetConsoleWindow(), SW_HIDE);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::Add)) {
                        ShowWindow(GetConsoleWindow(), SW_SHOW);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::V)) {
                        if(!velocityPressed) {
                            std::cout << "Velocity: " << withVelocity << std::endl;
                            withVelocity = !withVelocity;
                            velocityPressed = true;
                        }
                    } else if(!Keyboard::isKeyPressed(Keyboard::V)){
                        velocityPressed = false;
                    }
                    if(Keyboard::isKeyPressed(Keyboard::R) && gameInformation.getPlayerHealth() == 0 && !gameInformation.getRespawnRequested()) {
                        client.sendMessage(messageTypeToString(MessageType::RequestRespawn) + " -- id=" + std::to_string(client.getId()) + ";|");
                        gameInformation.setRespawnRequested(true);
                    }
                    if(Keyboard::isKeyPressed(Keyboard::Space) && !shotBullet) {
                        float angleRad = gameInformation.getPlayerRotation() * PI / 180;
                        client.sendBulletShot(client.getId(), gameInformation.getNextBulletId(), gameInformation.getPlayerPosition().x + cos(angleRad) * 30, gameInformation.getPlayerPosition().y + sin(angleRad) * 30, cos(angleRad) * 600, sin(angleRad) * 600);
                        shotBullet = true;
                    } else if(!Keyboard::isKeyPressed(Keyboard::Space)) {
                        shotBullet = false;
                    }
                }
                if(!Keyboard::isKeyPressed(Keyboard::W) && !Keyboard::isKeyPressed(Keyboard::S) || !focus) {
                    gameInformation.setPlayerVelocity(moveTowards(gameInformation.getPlayerVelocity(), Vector2f(0, 0), deltaTime, 250, 1));
                }

                //playerVelocity = playerPosition - oldPlayerPosition;
                //oldPlayerPosition = playerPosition;
                //playerVelocity = playerVelocity / deltaTime;
                //std::cout << "Player velocity: " << playerVelocity.x << " " << playerVelocity.y << std::endl;
                try {
                    window.clear();

                    //draw all basicwalls in mapdata
                    for(BasicWall basicWall : mapData.getBasicWalls()) {
                        basicWallRec.setPosition(basicWall.getPosition());
                        window.draw(basicWallRec);
                    }

                    shape.setFillColor(Color::Green);
                    shape.setPosition(gameInformation.getPlayerPosition());
                    shape.setRotation(gameInformation.getPlayerRotation());
                    window.draw(shape);

                    line[0].position = Vector2f(gameInformation.getPlayerPosition().x, gameInformation.getPlayerPosition().y);
                    float angleRad = gameInformation.getPlayerRotation() * PI / 180;
                    float lineLength = 30;
                    line[1].position = Vector2f(line[0].position.x + cos(angleRad) * lineLength, line[0].position.y + sin(angleRad) * lineLength);
                    window.draw(line, 2, Lines);

                    window.setTitle(client.getSocket().getRemoteAddress().toString() + " - " + client.getName() + " - health: " + std::to_string(gameInformation.getPlayerHealth()) +" id:" + std::to_string(client.getId()) + " FPS: " + std::to_string((int)(1.0f/deltaTime)));

                    for (EnemyInformation enemy : gameInformation.getEnemies()) {
                        if(true) {
                            shape.setFillColor(Color::Red);
                            shape.setPosition(enemy.getPosition());
                            shape.setRotation(enemy.getRotation());
                            nameText.setString(enemy.getName());
                            nameText.setOrigin(nameText.getLocalBounds().width / 2, nameText.getLocalBounds().height / 2);
                            nameText.setPosition(enemy.getPosition().x, enemy.getPosition().y - 20);
                            window.draw(shape);

                            line[0].position = Vector2f(enemy.getPosition().x, enemy.getPosition().y);
                            float angleRad = enemy.getRotation() * PI / 180;
                            float lineLength = 30;
                            line[1].position = Vector2f(line[0].position.x + cos(angleRad) * lineLength, line[0].position.y + sin(angleRad) * lineLength);
                            window.draw(line, 2, Lines);

                            window.draw(nameText);
                        }
                    }

                    for (Bullet bullet : gameInformation.getBullets()) {
                        circle.setPosition(bullet.getPosition());
                        window.draw(circle);
                    }

                    if(gameInformation.getPlayerHealth() == 0) {
                        window.draw(respawnText);
                    }

                    window.display();
                }catch (const std::exception& e) {
                    std::cout << "Error in main: " << e.what() << std::endl;
                }

            }
        }
    }catch(const std::exception& e) {
        std::cout << "Error in main: " << e.what() << std::endl;
    }
    std::cout << "Window closed" << std::endl;
    return 0;
}

sf::Vector2f moveTowards(const sf::Vector2f& current, const sf::Vector2f& target, float deltaTime, float speed, float minDistanceDelta) {
    // Berechne den Unterschied (delta) zwischen dem Ziel- und dem aktuellen Vektor
    sf::Vector2f delta = target - current;

    // Berechne die Länge des delta-Vektors (Distanz zum Ziel)
    float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    // Wenn der Abstand kleiner ist als minDistanceDelta, setzen wir den Vektor direkt auf das Ziel
    if (distance <= minDistanceDelta || distance == 0.0f) {
        return target;
    }

    // Berechne die mögliche Entfernung basierend auf Geschwindigkeit und deltaTime
    float distanceToMove = speed * deltaTime;

    // Normalisiere den delta-Vektor und multipliziere ihn mit der tatsächlichen Distanz, die wir bewegen wollen
    sf::Vector2f direction = delta / distance;
    return current + direction * distanceToMove;
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
                                mapData.addBasicWall(BasicWall(Vector2f(j * 40, i * 40), Vector2f(40, 40)));
                                break;
                            case '2':
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

sf::Vector2f movePlayerUntilCollision(const sf::Vector2f& currentPosition, const sf::Vector2f& velocity, const Vector2f& nextPosition, const sf::FloatRect& wallBounds) {
    // Berechne die mögliche Zeit bis zur Kollision mit den vier Seiten der Wand
    float tXmin = (wallBounds.left - currentPosition.x) / velocity.x;
    float tXmax = (wallBounds.left + wallBounds.width - currentPosition.x) / velocity.x;
    float tYmin = (wallBounds.top - currentPosition.y) / velocity.y;
    float tYmax = (wallBounds.top + wallBounds.height - currentPosition.y) / velocity.y;

    // Filtere nur positive Zeiten (Zukunft)
    float tMinX = std::min(tXmin, tXmax);
    float tMinY = std::min(tYmin, tYmax);

    // Wir wählen den ersten Zeitpunkt, an dem der Spieler auf eine Wandseite trifft
    float tMin = std::min(tMinX, tMinY);

    if (tMin > 0.0f) {
        // Der Spieler trifft eine Wand, bevor der Frame endet
        return currentPosition + velocity * tMin * 0.99f; // Bewege den Spieler bis kurz vor den Schnittpunkt
    }

    // Falls keine Kollision: Bewege den Spieler normal weiter
    return nextPosition;
}

void resolveCollision(GameInformation& gameInformation, const sf::RectangleShape& wall) {
    sf::RectangleShape player(sf::Vector2f(2, 2)); // Temporäres Rechteck für den Spieler
    player.setPosition(gameInformation.getPlayerPosition());
    sf::FloatRect playerBounds = player.getGlobalBounds();
    sf::FloatRect wallBounds = wall.getGlobalBounds();

    // Überprüfen, von welcher Seite die Kollision kam
    float overlapLeft   = playerBounds.left + playerBounds.width - wallBounds.left; // Überlappung auf der linken Seite
    float overlapRight  = wallBounds.left + wallBounds.width - playerBounds.left; // Überlappung auf der rechten Seite
    float overlapTop    = playerBounds.top + playerBounds.height - wallBounds.top;  // Überlappung auf der oberen Seite
    float overlapBottom = wallBounds.top + wallBounds.height - playerBounds.top;  // Überlappung auf der unteren Seite

    float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
    float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

    // Priorisiere die Kollision basierend auf der kleineren Überlappung
    if (minOverlapX < minOverlapY) {
        // Kollision von links oder rechts
        if (overlapLeft < overlapRight) {
            // Kollision von links
            gameInformation.setPlayerPosition(sf::Vector2f(wallBounds.left - playerBounds.width, gameInformation.getPlayerPosition().y));
            // Setze Y-Geschwindigkeit, um an der Wand zu gleiten
            gameInformation.setPlayerVelocity(sf::Vector2f(0, gameInformation.getPlayerVelocity().y));
        } else {
            // Kollision von rechts
            gameInformation.setPlayerPosition(sf::Vector2f(wallBounds.left + wallBounds.width, gameInformation.getPlayerPosition().y));
            // Setze Y-Geschwindigkeit, um an der Wand zu gleiten
            gameInformation.setPlayerVelocity(sf::Vector2f(0, gameInformation.getPlayerVelocity().y));
        }
    } else {
        // Kollision von oben oder unten
        if (overlapTop < overlapBottom) {
            // Kollision von oben
            gameInformation.setPlayerPosition(sf::Vector2f(gameInformation.getPlayerPosition().x, wallBounds.top - playerBounds.height));
            // Setze X-Geschwindigkeit, um an der Wand zu gleiten
            gameInformation.setPlayerVelocity(sf::Vector2f(gameInformation.getPlayerVelocity().x, 0));
        } else {
            // Kollision von unten
            gameInformation.setPlayerPosition(sf::Vector2f(gameInformation.getPlayerPosition().x, wallBounds.top + wallBounds.height));
            // Setze X-Geschwindigkeit, um an der Wand zu gleiten
            gameInformation.setPlayerVelocity(sf::Vector2f(gameInformation.getPlayerVelocity().x, 0));
        }
    }
}

void updatePlayer(GameInformation &gameInformation, float deltaTime, const std::vector<BasicWall>& walls) {
    // Berechne die geplante neue Position des Spielers
    sf::Vector2f newPosition = gameInformation.getPlayerPosition() + gameInformation.getPlayerVelocity() * deltaTime;

    // Setze die neue Position des Spielers
    gameInformation.setPlayerPosition(newPosition);

    sf::RectangleShape player(sf::Vector2f(2, 2)); // Temporäres Rechteck für den Spieler
    player.setPosition(gameInformation.getPlayerPosition());

    // Kollisionserkennung mit Wänden
    for (const auto& wall : walls) {
        sf::RectangleShape basicWallRec(sf::Vector2f(40, 40)); // Stelle sicher, dass die Größe der Wand stimmt
        basicWallRec.setPosition(wall.getPosition());
        if (player.getGlobalBounds().intersects(basicWallRec.getGlobalBounds())) {
            resolveCollision(gameInformation, basicWallRec);
        }
    }

    // Zusätzliche Überprüfung, um sicherzustellen, dass der Spieler nicht zwischen Wänden gefangen ist
    for (const auto& wall : walls) {
        sf::RectangleShape basicWallRec(sf::Vector2f(40, 40));
        basicWallRec.setPosition(wall.getPosition());
        if (player.getGlobalBounds().intersects(basicWallRec.getGlobalBounds())) {
            // Überprüfe, ob die neue Position des Spielers außerhalb der Wand ist
            sf::Vector2f position = gameInformation.getPlayerPosition();
            if (position.x < basicWallRec.getGlobalBounds().left ||
                position.x > basicWallRec.getGlobalBounds().left + basicWallRec.getGlobalBounds().width ||
                position.y < basicWallRec.getGlobalBounds().top ||
                position.y > basicWallRec.getGlobalBounds().top + basicWallRec.getGlobalBounds().height) {
                // Stelle sicher, dass der Spieler nicht durch die Wand hindurch rutscht
                gameInformation.setPlayerPosition(position);
                }
        }
    }
}