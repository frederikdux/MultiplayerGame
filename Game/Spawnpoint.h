//
// Created by Frederik on 11.10.2024.
//

#ifndef SPAWNPOINT_H
#define SPAWNPOINT_H
#include <SFML/System/Vector2.hpp>

class Spawnpoint{
private:
    sf::Vector2f position;

public:
    Spawnpoint(const sf::Vector2f &position)
        : position(position) {
    }

    void setPosition(const sf::Vector2f &position) {
        this->position = position;
    }

    [[nodiscard]] sf::Vector2f getPosition() const {
        return position;
    }
};
#endif //SPAWNPOINT_H
