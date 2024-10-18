project(Server VERSION 1.0 LANGUAGES CXX)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Add executable
add_executable(Server
        main.cpp
        Server.cpp
        ../Game/GameInformation.cpp
        ../Game/GameInformation.h
        ../Game/EnemyInformation.cpp
        ../Game/EnemyInformation.h
        ../Game/Bullet.cpp
        ../Game/Bullet.h
        ClientInfo.cpp
        ClientInfo.h
        MessageHandler.cpp
        MessageHandler.h
        ../Game/MapData.h
        ../Game/BasicWall.h
        ../Game/Spawnpoint.h
        ../Game/MessageType.h
)

# Link Libraries
target_link_libraries(Server
        sfml-system
        sfml-network
        ${ENET_LIBRARY}
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

# Include directories for the target
target_include_directories(Server PRIVATE ${CURL_INCLUDE_DIRS})

set_target_properties(Client PROPERTIES
        COMPILE_FLAGS "-g -O0"  # Optional, um sicherzustellen, dass Debug-Flags verwendet werden
)
