cmake_minimum_required(VERSION 3.11)
project(MultiplayerGame VERSION 1.0 LANGUAGES CXX)

# Setze den Build-Typ auf Debug und die Compiler-Flags
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g3")

set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -fno-sanitize=undefined-trap-on-error -fno-sanitize=undefined-trap")


# Setze den C++-Standard für alle Unterprojekte
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Compiler Flags für statisches Linking
set(CMAKE_CXX_FLAGS "-g3 -static-libstdc++ -static-libgcc")

# Definiere den benutzerdefinierten Modulpfad für zusätzliche CMake-Skripte
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Suche nach SFML (erfordere system, window, graphics, network)
set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR E:/CLion/x64/SFML-2.6.1/lib/cmake/SFML)
find_package(SFML 2.5 COMPONENTS system window graphics network REQUIRED)

# Suche nach CURL
find_package(CURL REQUIRED)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "libcurl wurde nicht gefunden!")
endif()

# Suche nach ENet
find_library(ENET_LIBRARY NAMES enet PATHS C:/Users/Frederik/CLionProjects/enet/build)
include_directories(C:/Users/Frederik/CLionProjects/enet/include)
if(NOT ENET_LIBRARY)
    message(FATAL_ERROR "ENet library not found!")
endif()

# Suche nach ZLIB
find_package(ZLIB REQUIRED)

# Fetch nlohmann_json
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Füge Unterverzeichnisse für Server, Client und UserInterface hinzu
add_subdirectory(Client)
add_subdirectory(Server)
add_subdirectory(UserInterface)

# (Optional) Setze die C++-Flags und den Standard auch in den Unterprojekten
# Das ist nützlich, wenn die Unterprojekte eigene CMakeLists.txt haben.
# Füge das in jede Unterprojekt-CMakeLists.txt ein:
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED True)
