# client/CMakeLists.txt
add_executable(Client main.cpp Client.cpp
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
)

# libcurl suchen und einbinden
find_package(CURL REQUIRED)
if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "libcurl wurde nicht gefunden!")
endif()

target_link_libraries(Client sfml-graphics sfml-system sfml-network ${ENET_LIBRARY} ${CURL_LIBRARIES} nlohmann_json::nlohmann_json ZLIB::ZLIB)


