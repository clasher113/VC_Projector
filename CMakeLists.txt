cmake_minimum_required(VERSION 3.11)
project(ProjectorServer)

set(CMAKE_CXX_STANDARD 14)

file(GLOB SRC "src/*.cpp")
add_executable(${PROJECT_NAME} ${SRC})

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-system sfml-window sfml-network)
if (UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE X11)
endif (UNIX)
