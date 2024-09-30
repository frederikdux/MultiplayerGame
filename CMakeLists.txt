cmake_minimum_required(VERSION 3.10)
project(MultiplayerGame VERSION 1.0 LANGUAGES CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc --static")

# Find SFML (require Network module)
set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR E:/CLion/x64/SFML-2.6.1/lib/cmake/SFML)
find_package(SFML COMPONENTS system window graphics network audio REQUIRED)

find_library(ENET_LIBRARY NAMES enet PATHS C:/Users/Frederik/CLionProjects/enet/build)
include_directories(C:/Users/Frederik/CLionProjects/enet/include)

# Add subdirectories for server and client
add_subdirectory(server)
add_subdirectory(client)
