cmake_minimum_required(VERSION 3.20)

project(PingToolSuper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(PingToolSuper
    src/main.cpp
    src/PingToolWindow.h
    src/PingToolWindow.cpp
    src/PingCommandBuilder.h
    src/PingCommandBuilder.cpp
    src/PingOutputParser.h
    src/PingOutputParser.cpp
)

target_link_libraries(PingToolSuper PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
