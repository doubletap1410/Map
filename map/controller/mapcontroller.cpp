
#include <common/Runtime.h>

#include "frame/mapframe.h"
#include "interact/maphelper.h"
#include "coord/mapcamera.h"

#include "mapcontroller.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

struct MapControllerPrivate
{
    MapFrame *map = NULL;
    MapHelperUniversal *mhu = NULL;
    QList<MapQMLButton*> actionButtons;
    QMultiHash<HelperState, int> mapStBt;

    QList<MapQMLPlugin*> settings;
    QList<MapQMLPlugin*> panels;
};

// -------------------------------------------------------

MapController::MapController(QObject *parent)
    : QObject(parent), d_ptr(new MapControllerPrivate)
{

}

MapController::~MapController()
{
    qDeleteAll(d_ptr->actionButtons);
    d_ptr->actionButtons.clear();
    delete d_ptr;
}

void MapController::init(MapFrame *map)
{
    Q_D(MapController);
    d->map = map;
    grabMouse();

    autoRegistration();
}

void MapController::autoRegistration()
{
    // Buttons registration

    // Move button
    registerController(
                this,
                &MapController::moveMap,
                "qrc:///Images/Move_map_white.svg",
                true,
                false,
                stMove
                );

    // Rotate button
    registerController(
                this,
                &MapController::rotateMap,
                "qrc:///Images/rotate_white.svg",
                true,
                false,
                stRotate
                );

    // Scale plus button
    registerController(
                this,
                &MapController::zoomIn,
                "qrc:///Images/plus_white.svg",
                false,
                true
                );

    // Scale minus button
    registerController(
                this,
                &MapController::zoomOut,
                "qrc:///Images/minus_white.svg",
                false,
                true
                );

    // Panels registration
    registerPanel("qrc:/qml/MapFade.qml", QString::fromUtf8("Панель инструментов"));
    registerPanel("qrc:/qml/MapFade2.qml", QString::fromUtf8("Прочее..."));

    // Settings registration
    registerSetting("qrc:/qml/MapFade.qml", QString::fromUtf8("Карта"));
    registerSetting("qrc:/qml/MapFade2.qml", QString::fromUtf8("Интрументы"));
}

QQmlListProperty<minigis::MapQMLButton> MapController::controls()
{
    Q_D(MapController);
    return QQmlListProperty<minigis::MapQMLButton>(this, d->actionButtons);
}

QQmlListProperty<MapQMLPlugin> MapController::settings()
{
    Q_D(MapController);
    return QQmlListProperty<minigis::MapQMLPlugin>(this, d->settings);
}

void MapController::registerSetting(const QString &qmlform, const QString &text, const QString &image)
{
    Q_D(MapController);
    d->settings.append(new MapQMLPlugin(qmlform, text, image, this));
}

QQmlListProperty<MapQMLPlugin> MapController::panels()
{
    Q_D(MapController);
    return QQmlListProperty<minigis::MapQMLPlugin>(this, d->panels);
}

void MapController::registerPanel(const QString &qmlform, const QString &text, const QString &image)
{
    Q_D(MapController);
    d->panels.append(new MapQMLPlugin(qmlform, text, image, this));
}

void MapController::moveMap(bool flag)
{
    Q_D(MapController);
    if (grabMouse())
        flag ? d->mhu->move() : d->mhu->moveStop();
}

void MapController::rotateMap(bool flag)
{
    Q_D(MapController);
    if (grabMouse())
        flag ? d->mhu->rotate() : d->mhu->rotateStop();
}

void MapController::zoomIn(bool)
{
    Q_D(MapController);
    if (!d->map)
        return;
    d->map->camera()->setScale(d->map->camera()->scale() * 2, QPointF(), 300);
    d->map->updateScene();
}

void MapController::zoomOut(bool)
{
    Q_D(MapController);
    if (!d->map)
        return;
    d->map->camera()->setScale(d->map->camera()->scale() * 0.5, QPointF(), 300);
    d->map->updateScene();
}

void MapController::stateChanged(States st)
{
    Q_D(MapController);

    QHash<int, bool> hash;
    if (st.testFlag(stMove)) {
        QList<int> res = d->mapStBt.values(stMove);
        foreach (int key, res)
            hash[key] = true;
    }
    if (st.testFlag(stScale)) {
        QList<int> res = d->mapStBt.values(stScale);
        foreach (int key, res)
            hash[key] = true;
    }
    if (st.testFlag(stRotate)) {
        QList<int> res = d->mapStBt.values(stRotate);
        foreach (int key, res)
            hash[key] = true;
    }
    if (st.testFlag(stSelect)) {
        QList<int> res = d->mapStBt.values(stSelect);
        foreach (int key, res)
            hash[key] = true;
    }
    if (st.testFlag(stCreate)) {
        QList<int> res = d->mapStBt.values(stCreate);
        foreach (int key, res)
            hash[key] = true;
    }
    if (st.testFlag(stEdit)) {
        QList<int> res = d->mapStBt.values(stEdit);
        foreach (int key, res)
            hash[key] = true;
    }

    for (auto it = d->mapStBt.begin(); it != d->mapStBt.end(); ++it)
        if (!hash.contains(it.value()))
            hash[it.value()] = false;

    for (auto it = hash.begin(); it != hash.end(); ++it)
        emit buttonStateChanged(it.key(), it.value());
}

bool MapController::grabMouse()
{
    Q_D(MapController);
    if (!d->map)
        return false;

    if (!d->mhu) {
        d->mhu = d->map->actionUniversal(this, SIGNAL(breakHepler()));
        if (d->mhu)
            connect(d->mhu, SIGNAL(stateChanged(States)), SLOT(stateChanged(States)));
    }
    return !!d->mhu;
}

template <typename T>
void MapController::registerController(T *target, void (T::*func)(bool), const QString &image, bool checkable, bool autoRepeat, States st)
{
    Q_D(MapController);
    d->actionButtons.append(new MapQMLButton(
                                makeCallable(target, func),
                                image,
                                checkable,
                                autoRepeat,
                                this
                                )
                            );
    if (st != stNoop) {
        int idx = d->actionButtons.size() - 1;
        if (st.testFlag(stMove))
            d->mapStBt.insert(stMove, idx);
        if (st.testFlag(stRotate))
            d->mapStBt.insert(stRotate, idx);
        if (st.testFlag(stScale))
            d->mapStBt.insert(stScale, idx);
        if (st.testFlag(stSelect))
            d->mapStBt.insert(stSelect, idx);
        if (st.testFlag(stCreate))
            d->mapStBt.insert(stCreate, idx);
        if (st.testFlag(stEdit))
            d->mapStBt.insert(stEdit, idx);
    }
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
