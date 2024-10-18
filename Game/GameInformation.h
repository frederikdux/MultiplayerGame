//
// Created by Frederik on 27.09.2024.
//

#ifndef GAMEINFORMATION_H
#define GAMEINFORMATION_H
#include <algorithm>
#include <iostream>
#include <list>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <nlohmann/json.hpp>

#include "Bullet.h"
#include "EnemyInformation.h"
#include "MapData.h"


class GameInformation {
private:
    std::vector<EnemyInformation> enemies;
    std::mutex enemy_mutex;

    std::vector<Bullet> bullets;
    std::mutex bullet_mutex;

    sf::Vector2f playerPosition;
    sf::Vector2f playerVelocity;
    float playerRotation;
    int playerHealth;
    bool respawnRequested;
    int nextBulletId = 0;
public:
    GameInformation();

    void addEnemy(EnemyInformation enemy) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        std::cout << "Adding enemy with id " << enemy.getId() << std::endl;
        try {
            enemies.push_back(enemy);
            return;
        } catch (std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
        throw std::runtime_error("addEnemy: Error adding enemy with id " + std::to_string(enemy.getId()));
    }

    void removeEnemy(int id) {
        std::lock_guard<std::mutex> lock(enemy_mutex);

        for (auto it = enemies.begin(); it != enemies.end(); ++it) {
            if (it->getId() == id) {
                enemies.erase(it);
                return;
            }
        }
        throw std::runtime_error("removeEnemy: No enemy with id " + std::to_string(id));
    }

    void updateEnemy(EnemyInformation enemy) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        for (auto& e : enemies) {
            if (e.getId() == enemy.getId()) {
                e = enemy;
                return;
            }
        }
        throw std::runtime_error("updateEnemy: No enemy with id " + std::to_string(enemy.getId()));
    }

    void updateEnemyPosition(int id, sf::Vector2f position) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        for (auto& e : enemies) {
            if (e.getId() == id) {
                e.setPosition(position);
                return;
            }
        }
        throw std::runtime_error("updateEnemyPosition: No enemy with id " + std::to_string(id));
    }

    void updateEnemyVelocity(int id, sf::Vector2f velocity) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        for (auto& e : enemies) {
            if (e.getId() == id) {
                e.setVelocity(velocity);
                return;
            }
        }
        throw std::runtime_error("updateEnemyVelocity: No enemy with id " + std::to_string(id));
    }

    //update enemy position and velocity
    void updateEnemyPositionAndVelocity(int id, sf::Vector2f position, sf::Vector2f velocity) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        for (auto& e : enemies) {
            if (e.getId() == id) {
                e.setPosition(position);
                e.setVelocity(velocity);
                //std::cout << "Updated enemy with id " << id << " vel: " << velocity.x << " " << velocity.y << std::endl;
                return;
            }
        }
        throw std::runtime_error("updateEnemyPositionAndVelocity: No enemy with id " + std::to_string(id));
    }

    void updateEnemyData(int id, sf::Vector2f position, sf::Vector2f velocity, float rotation) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        for (auto& e : enemies) {
            if (e.getId() == id) {
                e.setPosition(position);
                e.setVelocity(velocity);
                e.setRotation(rotation);
                //std::cout << "Updated enemy with id " << id << " vel: " << velocity.x << " " << velocity.y << std::endl;
                return;
            }
        }
        throw std::runtime_error("updateEnemyPositionAndVelocity: No enemy with id " + std::to_string(id));
    }


    EnemyInformation getEnemy(int id) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        for (EnemyInformation e : enemies) {
            if (e.getId() == id) {
                return e;
            }
        }
        throw std::runtime_error("getEnemy: No enemy with id " + std::to_string(id));
    }

    std::vector<EnemyInformation> getEnemies() {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        return enemies;
    }

    bool enemyExists(int id) {
        for (auto& e : enemies) {
            if (e.getId() == id) {
                return true;
            }
        }
        return false;
    }

    std::mutex& getEnemyMutex() {
        return enemy_mutex;
    }

    //update the enemy positions with velocitys and parameter deltaTime
    void updateEnemyPositions(float deltaTime) {
        std::lock_guard<std::mutex> lock(enemy_mutex);
        try {
            for (auto& e : enemies) {
                sf::Vector2f position = e.getPosition();
                sf::Vector2f velocity = e.getVelocity();
                position += velocity * deltaTime;
                e.setPosition(position);
            }
        } catch (...) {
            std::cout << "Error updating bullet positions" << std::endl;
        }
    }

    void updateBulletPositions(float deltaTime, MapData mapData) {
        std::lock_guard<std::mutex> lock(bullet_mutex);
        try {
            for (auto it = bullets.begin(); it != bullets.end(); ) {
                sf::Vector2f position = it->getPosition();
                sf::Vector2f velocity = it->getVelocity();
                position += velocity * deltaTime;
                it->setPosition(position);
                for(auto& wall : mapData.getBasicWalls()) {
                    if(wall.getPosition().x < position.x && position.x < wall.getPosition().x + wall.getSize().x &&
                       wall.getPosition().y < position.y && position.y < wall.getPosition().y + wall.getSize().y) {
                        it = bullets.erase(it);
                        return;
                    }
                }
                if(abs(position.x) > 1000 || abs(position.y) > 1000) {
                    it = bullets.erase(it);
                } else {
                    ++it;
                }
            }
        } catch (...) {
            std::cout << "Error updating bullet positions" << std::endl;
        }
    }

    void addBullet(Bullet bullet) {
        std::lock_guard<std::mutex> lock(bullet_mutex);
        bullets.push_back(bullet);
    }

    void addBullets(std::vector<Bullet> new_bullets) {
        std::lock_guard<std::mutex> lock(bullet_mutex);
        bullets.insert(bullets.end(), new_bullets.begin(), new_bullets.end());
    }

    std::vector<Bullet> getBullets() {
        std::lock_guard<std::mutex> lock(bullet_mutex);
        return bullets;
    }

    void removeBulletByAddress(Bullet& bulletAddr) {
        std::lock_guard<std::mutex> lock(bullet_mutex);
        for (auto it = bullets.begin(); it != bullets.end(); ++it) {
            if (bulletAddr.getPosition() == it->getPosition() && bulletAddr.getVelocity() == it->getVelocity()) {  // Vergleich der Speicheradresse
                bullets.erase(it);
                return;
            }
        }
        throw std::runtime_error("removeBulletByAddress: No bullet with address ...");
    }
    void removeBullet(int playerId, int bulletId) {
        std::lock_guard<std::mutex> lock(bullet_mutex);
        for (auto it = bullets.begin(); it != bullets.end(); ++it) {
            if (it->getPlayerId() == playerId && it->getBulletIdOfPlayer() == bulletId) {  // Vergleich der Speicheradresse
                bullets.erase(it);
                return;
            }
        }
        throw std::runtime_error("removeBulletByAddress: No bullet with playerId: " + std::to_string(playerId) + " and bulletId: " + std::to_string(bulletId));
    }

    sf::Vector2f getPlayerPosition() {
        return playerPosition;
    }

    sf::Vector2f getPlayerVelocity() {
        return playerVelocity;
    }

    float getPlayerRotation() {
        return playerRotation;
    }

    void setPlayerPosition(sf::Vector2f position) {
        playerPosition = position;
    }

    void setPlayerVelocity(sf::Vector2f velocity) {
        playerVelocity = velocity;
    }

    void setRotation(float rotation) {
        playerRotation = rotation;
    }

    void setPositionX(float x) {
        playerPosition.x = x;
    }

    void setPositionY(float y) {
        playerPosition.y = y;
    }

    void setVelocityX(float x) {
        playerVelocity.x = x;
    }

    void setVelocityY(float y) {
        playerVelocity.y = y;
    }

    void setPlayerRotation(float rotation) {
        playerRotation = rotation;
    }

    void increasePlayerRotation(float rotation) {
        playerRotation += rotation;
    }

    void setPlayerHealth(int health) {
        playerHealth = health;
    }

    int getPlayerHealth() const {
        return playerHealth;
    }

    bool getRespawnRequested() const {
        return respawnRequested;
    }

    void setRespawnRequested(bool requested) {
        respawnRequested = requested;
    }

    [[nodiscard]] int getNextBulletId() {
        nextBulletId++;
        return nextBulletId-1;
    }
};



#endif //GAMEINFORMATION_H
