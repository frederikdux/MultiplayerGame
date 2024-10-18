project(UserInterface VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui NETWORK)

set(CMAKE_AUTOMOC ON)  # Automatisch Qt MOC ausführen
set(CMAKE_AUTOUIC ON)  # Automatisch Qt UIC für .ui Dateien ausführen
set(CMAKE_AUTORCC ON)  # Automatisch Qt RCC für Ressourcen ausführen

add_executable(UserInterface
        main.cpp
        MainWindow.cpp
        ui_MainWindow.h
)

target_link_libraries(UserInterface
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
        Qt5::Network
)
