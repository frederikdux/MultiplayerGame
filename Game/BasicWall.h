//
// Created by Frederik on 09.10.2024.
//

#ifndef BASICWALL_H
#define BASICWALL_H
#include <SFML/System/Vector2.hpp>


class BasicWall {
private:
    sf::Vector2f position;
    sf::Vector2f size;


public:
    BasicWall(const sf::Vector2f &position, const sf::Vector2f &size)
        : position(position),
          size(size) {
    }

    void setPosition(const sf::Vector2f &position) {
        this->position = position;
    }

    void setSize(const sf::Vector2f &size) {
        this->size = size;
    }

    [[nodiscard]] sf::Vector2f getPosition() const {
        return position;
    }

    [[nodiscard]] sf::Vector2f getSize() const {
        return size;
    }
};



#endif //BASICWALL_H
