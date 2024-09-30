# server/CMakeLists.txt
add_executable(Server main.cpp Server.cpp
        ClientInfo.cpp
        ClientInfo.h
        MessageHandler.cpp
        MessageHandler.h)
target_link_libraries(Server sfml-system sfml-network ${ENET_LIBRARY})
