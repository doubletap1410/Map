#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>

#include <QTime>

#include <time.h>

#include <map/core/mapmath.h>
#include <map/core/mapdefs.h>

#include <map/frame/mapframe.h>
#include <map/coord/mapcamera.h>
#include <map/frame/mapsettings.h>
#include <map/controller/mapcontroller.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<minigis::MapFrame>      ("com.sapsan.map"          , 1, 0, "MapFrame");
    qmlRegisterType<minigis::MapSettings>   ("com.sapsan.mapsettings"  , 1, 0, "MapSettings");
    qmlRegisterType<minigis::MapCamera>     ("com.sapsan.mapcamera"    , 1, 0, "MapCamera");
    qmlRegisterType<minigis::MapQMLButton>  ("com.sapsan.mapqmlbutton" , 1, 0, "MapQMLButton");
    qmlRegisterType<minigis::Callable>      ("com.sapsan.callable"     , 1, 0, "MapQMLFunc");
    qmlRegisterType<minigis::MapController> ("com.sapsan.mapcontroller", 1, 0, "MapController");
    qmlRegisterType<minigis::MapQMLPlugin>  ("com.sapsan.mapqmlplugin" , 1, 0, "MapQMLPlugin");

    // TESTS
//    srand(time(NULL));
//    qulonglong max = 40000000;
//    for (qulonglong i = 0; i < max; ++i) {

//    }

//    QScreen *screen = qApp->primaryScreen();
//    int dpi = screen->physicalDotsPerInch() * screen->devicePixelRatio();
//    qreal dp = dpi / 160.0f;
//    bool isMobile = false;

    QQmlApplicationEngine engine;
//    engine.setImportPathList(QStringList() << engine.importPathList() <<
//                             QString("/opt/qml/modules"));
//    engine.load(QUrl(QStringLiteral("qrc:/qml/MainForm.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/MapSettings.qml")));

//    minigis::MapFrame *map = qobject_cast<minigis::MapFrame*>(engine.rootObjects()[0]->children()[0]->children()[0]);
//    if (map && map->controller()) {
//        minigis::MapController *control = map->controller();
//        control->registerSetting(mapsettings::uiPath, mapsettings::name, mapsettings::imagePath);
//    }

    return app.exec();
}

