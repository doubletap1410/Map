set(SRC frame/mapframe.cpp
        frame/mapframe_p.cpp
        frame/mapsettings.cpp
        core/mapmath.cpp
        core/mapmetric.cpp
        interact/mapuserinteraction.cpp
        interact/maphelper.cpp
        interact/maphelper_p.cpp
        interact/maphelpertouch.cpp
        interact/maphelpersubhelpers.cpp
        interact/maphelpercommon.cpp
        coord/mapcamera.cpp
        coord/mapcamera_p.cpp
        coord/mapcoords.cpp
        loaders/maptileloader.cpp
        layers/maplayer.cpp
        layers/maplayer_p.cpp
        layers/maplayertile.cpp
        layers/maplayertile_p.cpp
        layers/maprtree.cpp
        layers/maplayerobjects.cpp
        layers/maplayersystem.cpp
        sql/mapsql.cpp
        controller/mapcontroller.cpp
        object/mapobject.cpp
        drawer/mapdrawercommon.cpp
        drawer/mapdrawer.cpp
        drawer/mapdrawerline.cpp
)

set(HEADERS frame/mapframe.h
            frame/mapframe_p.h
            frame/mapsettings.h
            core/mapdefs.h
            core/mapmath.h
            core/maptemplates.h
            core/mapmetric.h
            interact/mapuserinteraction.h
            interact/maphelper.h
            interact/maphelper_p.h
            interact/maphelpertouch.h
            interact/maphelpersubhelpers.h
            interact/maphelpercommon.h
            coord/mapcamera.h
            coord/mapcamera_p.h
            coord/mapcoords.h
            loaders/maptileloader.h
            layers/maplayer.h
            layers/maplayer_p.h
            layers/maplayertile.h
            layers/maplayertile_p.h
            layers/maprtree.h
            layers/maplayerobjects.h
            layers/maplayersystem.h
            sql/mapsql.h
            controller/mapcontroller.h
            object/mapobject.h
            drawer/mapdrawercommon.h
            drawer/mapdrawer.h
            drawer/mapdrawer_p.h
            drawer/mapdrawerline.h
)

set(DOCS etc/tileserver.xml)

set(LIBS    Qt5::Core
            Qt5::Quick
            Qt5::Qml
            Qt5::Svg
            Qt5::Network
            Qt5::Xml
            Qt5::Concurrent
)

file(COPY ${DOCS} DESTINATION ${DEPLOY_ETC_DIR})
file(COPY res/rsc.svg DESTINATION ${DEPLOY_SHARE_DIR})

set(LIBS ${LIBS} db common)

qt5_add_resources(RESOURCES qml.qrc
                            res/res.qrc
)

add_library(map ${SRC} ${HEADERS} ${RESOURCES} ${DOCS})
target_link_libraries(map ${LIBS})
