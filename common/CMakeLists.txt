set(SRC Runtime.cpp)

set(HEADERS Runtime.h
            coord.hpp
)

set(LIBS Qt5::Core)

add_library(common ${SRC} ${HEADERS} ${RESOURCES})
target_link_libraries(common ${LIBS})
