cmake_minimum_required(VERSION 3.11)
project(MultiplayerGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CURL_LIBRARY "E:/msys2/mingw64/lib/libcurl.dll.a")
set(CURL_INCLUDE_DIR "E:/msys2/mingw64/include")
cmake_policy(SET CMP0135 NEW)

link_directories("E:/msys2/mingw64/lib")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc --static")

# Find SFML (require Network module)
set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR E:/CLion/x64/SFML-2.6.1/lib/cmake/SFML)
find_package(SFML COMPONENTS system window graphics network audio REQUIRED)

find_library(ENET_LIBRARY NAMES enet PATHS C:/Users/Frederik/CLionProjects/enet/build)
include_directories(C:/Users/Frederik/CLionProjects/enet/include)



include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(ZLIB REQUIRED)

# Add subdirectories for server and client
add_subdirectory(Server)
add_subdirectory(Client)
