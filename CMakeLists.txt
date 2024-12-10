cmake_minimum_required(VERSION 3.15)
project(ProjectorServer)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SRC "src/*.cpp")
add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-network)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-window)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system)
if (UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE X11)
endif (UNIX)