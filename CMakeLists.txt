project(OpenMap)
cmake_minimum_required(VERSION 2.8.11)
aux_source_directory(. SRC_LIST)

if (${UNIX})
    message(STATUS "UNIX")
elseif(${APPLE})
    message(STATUS "APPLE")
elseif(${WIN32})
    message(STATUS "WINDOWS")
else()
    message(STATUS "UNDEFINED OPERATION SYSTEM")
endif()

set(Qt5_COMMPONETS Core Quick Qml Svg Network Xml Sql Concurrent)
find_package(Qt5 COMPONENTS ${Qt5_COMMPONETS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(BUILD_SHARED_LIBS true)

set(DEPLOY_ETC_DIR   ${CMAKE_BINARY_DIR}/etc)
set(DEPLOY_SHARE_DIR ${CMAKE_BINARY_DIR}/share)
set(DEPLOY_QML_DIR ${CMAKE_BINARY_DIR}/qml)
file(MAKE_DIRECTORY ${DEPLOY_ETC_DIR})
file(MAKE_DIRECTORY ${DEPLOY_SHARE_DIR})
file(MAKE_DIRECTORY ${DEPLOY_QML_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(common)
add_subdirectory(db)
add_subdirectory(map)

# Packing
#set(CPACK GENERATOR "TGZ")
#set(CPACK PACKAGE VERSION MAJOR 0)
#set(CPACK PACKAGE VERSION MINOR 1)
#set(CPACK PACKAGE VERSION PATCH 0)
#include(CPack)

set(SRC main.cpp)

set(LIBS ${LIBS} map)

add_executable(OpenMap ${SRC})

target_link_libraries(OpenMap ${LIBS})
