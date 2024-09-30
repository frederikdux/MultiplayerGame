# client/CMakeLists.txt
add_executable(Client main.cpp Client.cpp
        Game/GameInformation.cpp
        Game/GameInformation.h
        Game/EnemyInformation.cpp
        Game/EnemyInformation.h
        Game/main2.cpp
        Game/main2.h)
target_link_libraries(Client sfml-graphics sfml-system sfml-network ${ENET_LIBRARY})


