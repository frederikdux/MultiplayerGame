//
// Created by Frederik on 27.09.2024.
//

#ifndef ENEMYINFORMATION_H
#define ENEMYINFORMATION_H
#include <string>
#include <SFML/System/Vector2.hpp>


class EnemyInformation {
private:
    int id;
    sf::Vector2f position;
    sf::Vector2f velocitiy;
    std::string name;
    int health;

public:
    EnemyInformation(int id, std::string name, sf::Vector2f position, sf::Vector2f velocity);

    void setPosition(sf::Vector2f position);
    void setVelocity(sf::Vector2f velocity);
    void setName(std::string name);
    void setHealth(int health);

    sf::Vector2f getPosition();
    sf::Vector2f getVelocity();
    std::string getName();
    int getHealth();
    int getId();
};



#endif //ENEMYINFORMATION_H
