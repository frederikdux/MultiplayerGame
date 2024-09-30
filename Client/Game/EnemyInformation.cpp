//
// Created by Frederik on 27.09.2024.
//

#include "EnemyInformation.h"

#include <utility>

EnemyInformation::EnemyInformation(int id, std::string name, sf::Vector2f position, sf::Vector2f velocity) {
    this->id = id;
    this->name = name;
    this->position = position;
    this->velocitiy = velocity;
    this->health = 100;
}

void EnemyInformation::setPosition(sf::Vector2f position) {
    this->position = position;
}

void EnemyInformation::setVelocity(sf::Vector2f velocity) {
    this->velocitiy = velocity;
}

void EnemyInformation::setName(std::string name) {
    this->name = std::move(name);
}

void EnemyInformation::setHealth(int health) {
    this->health = health;
}

sf::Vector2f EnemyInformation::getPosition() {
    return this->position;
}

sf::Vector2f EnemyInformation::getVelocity() {
    return this->velocitiy;
}

std::string EnemyInformation::getName() {
    return this->name;
}

int EnemyInformation::getHealth() {
    return this->health;
}

int EnemyInformation::getId() {
    return this->id;
}