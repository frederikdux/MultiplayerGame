//
// Created by Frederik on 09.10.2024.
//

#ifndef MAPDATA_H
#define MAPDATA_H
#include <vector>

#include "BasicWall.h"
#include "Spawnpoint.h"


class MapData {
private:
    std::vector<BasicWall> basicWalls;
    std::vector<Spawnpoint> spawnpoints;

public:
    MapData(){}

    void addBasicWall(const BasicWall& basicWall) {
        basicWalls.push_back(basicWall);
    }

    [[nodiscard]] std::vector<BasicWall> getBasicWalls() const {
        return basicWalls;
    }

    void addSpawnpoint(const Spawnpoint& spawnpoint) {
        spawnpoints.push_back(spawnpoint);
    }

    [[nodiscard]] std::vector<Spawnpoint> getSpawnpoints() const {
        return spawnpoints;
    }
};



#endif //MAPDATA_H
