cmake_minimum_required(VERSION 3.15)
project(ProjectorServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-network)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-window)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system)
