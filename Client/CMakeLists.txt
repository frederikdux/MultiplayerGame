project(Client VERSION 1.0 LANGUAGES CXX)

# Add executable
add_executable(Client
        main.cpp
        Client.cpp
        ../Game/GameInformation.cpp
        ../Game/GameInformation.h
        ../Game/EnemyInformation.cpp
        ../Game/EnemyInformation.h
        ../Game/Bullet.cpp
        ../Game/Bullet.h
        ServerData.cpp
        ServerData.h
        ../Game/MapData.h
        ../Game/BasicWall.h
        ../Game/Spawnpoint.h
        ../Game/MessageType.h
)

# Link Libraries
target_link_libraries(Client
        sfml-graphics
        sfml-system
        sfml-network
        ${ENET_LIBRARY}
        CURL::libcurl
        ZLIB::ZLIB
)

# Include directories for the target
target_include_directories(Client PRIVATE ${CURL_INCLUDE_DIRS})
