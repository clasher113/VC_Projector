cmake_minimum_required(VERSION 3.11)
project(ProjectorServer)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC})

include(FetchContent)

# link sfml
set(SFML_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK ON CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS ON CACHE BOOL "" FORCE)
set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
set(SFML_BUILD_SYSTEM ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.x
)
FetchContent_MakeAvailable(sfml)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-system sfml-window sfml-network)

set(LIBS_PATH ${PROJECT_SOURCE_DIR}/libs/)

# link giflib
set(GIFLIB_PATH ${LIBS_PATH}/giflib/lib/)
file(GLOB GIFLIB_SOURCES ${GIFLIB_PATH}/*.h ${GIFLIB_PATH}/*.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${GIFLIB_PATH})
add_library(giflib STATIC ${GIFLIB_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE giflib)

# link portable-file-dialogs
set(PORTABLE_FILE_DIALOGS_PATH ${LIBS_PATH}/portable-file-dialogs/)
target_include_directories(${PROJECT_NAME} PRIVATE ${PORTABLE_FILE_DIALOGS_PATH})

if (UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE Xext X11)
endif (UNIX)