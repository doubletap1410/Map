set(SRC databasecontroller.cpp)

set(HEADERS databasecontroller.h)

set(LIBS Qt5::Core Qt5::Svg Qt5::Network Qt5::Sql)

add_library(db ${SRC} ${HEADERS} ${RESOURCES})
target_link_libraries(db ${LIBS})

