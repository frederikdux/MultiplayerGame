//
// Created by Frederik on 27.09.2024.
//

#ifndef GAMEINFORMATION_H
#define GAMEINFORMATION_H
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "EnemyInformation.h"


class GameInformation {
private:
    std::vector<EnemyInformation> enemies;

public:
    GameInformation();

    void addEnemy(EnemyInformation enemy) {
        std::cout << "Adding enemy with id " << enemy.getId() << std::endl;
        enemies.push_back(enemy);
    }

    void removeEnemy(int id) {
        for (auto it = enemies.begin(); it != enemies.end(); ++it) {
            if (it->getId() == id) {
                enemies.erase(it);
                break;
            }
        }
    }

    void updateEnemy(EnemyInformation enemy) {
        for (auto& e : enemies) {
            if (e.getId() == enemy.getId()) {
                e = enemy;
                break;
            }
        }
    }

    void updateEnemyPosition(int id, sf::Vector2f position) {
        for (auto& e : enemies) {
            if (e.getId() == id) {
                e.setPosition(position);
                return;
            }
        }
        throw std::runtime_error("updateEnemyPosition: No enemy with id " + std::to_string(id));
    }

     EnemyInformation getEnemy(int id) const {
        for (EnemyInformation e : enemies) {
            if (e.getId() == id) {
                return e;
            }
        }
        throw std::runtime_error("getEnemy: No enemy with id " + std::to_string(id));
    }

     std::vector<EnemyInformation> getEnemies() const {
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

};



#endif //GAMEINFORMATION_H
