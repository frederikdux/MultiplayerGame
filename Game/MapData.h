//
// Created by Frederik on 09.10.2024.
//

#ifndef MAPDATA_H
#define MAPDATA_H
#include <vector>

#include "BasicWall.h"


class MapData {
private:
    std::vector<BasicWall> basicWalls;

public:
    MapData(){}

    void addBasicWall(const BasicWall& basicWall) {
        basicWalls.push_back(basicWall);
    }

    [[nodiscard]] std::vector<BasicWall> getBasicWalls() const {
        return basicWalls;
    }
};



#endif //MAPDATA_H
