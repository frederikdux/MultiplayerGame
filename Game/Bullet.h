//
// Created by Frederik on 08.10.2024.
//

#ifndef BULLET_H
#define BULLET_H
#include <SFML/System/Vector2.hpp>


class Bullet {
private:
    int playerId;
    int bulletIdOfPlayer;
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation;
    int baseDamage;

public:
    Bullet(const int playerId, const int bulletIdOfPlayer, const sf::Vector2f &position, const sf::Vector2f &velocity, float rotation, int base_damage)
        : playerId(playerId),
          bulletIdOfPlayer(bulletIdOfPlayer),
          position(position),
          velocity(velocity),
          rotation(rotation),
          baseDamage(base_damage) {
    }

    [[nodiscard]] sf::Vector2f getPosition() const {
        return position;
    }

    [[nodiscard]] sf::Vector2f getVelocity() const {
        return velocity;
    }

    [[nodiscard]] float getRotation() const {
        return rotation;
    }

    [[nodiscard]] int getBaseDamage() const {
        return baseDamage;
    }

    void setPosition(const sf::Vector2f &position) {
        this->position = position;
    }

    void setVelocity(const sf::Vector2f &velocity) {
        this->velocity = velocity;
    }

    void setRotation(float rotation) {
        this->rotation = rotation;
    }

    void setBaseDamage(int base_damage) {
        baseDamage = base_damage;
    }

    [[nodiscard]] int getPlayerId() const {
        return playerId;
    }

    void setPlayerId(int player_id) {
        playerId = player_id;
    }

    [[nodiscard]] int getBulletIdOfPlayer() const {
        return bulletIdOfPlayer;
    }

    void setBulletIdOfPlayer(int bullet_id_of_player) {
        bulletIdOfPlayer = bullet_id_of_player;
    }
};



#endif //BULLET_H
