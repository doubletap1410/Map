#include <QDebug>
#include <QTime>
#include <QThread>
#include <QUuid>

#include "object/mapobject.h"

#include "core/mapdefs.h"
#include "frame/mapframe.h"

#include "layers/maplayer_p.h"
#include "layers/maplayer.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

MapLayer::MapLayer(QObject *parent)
    : QObject(parent), d_ptr(new MapLayerPrivate)
{    
    setObjectName("MapLayer");
    d_ptr->q_ptr = this;
    connect(d_ptr, SIGNAL(needRender(QRect)), this, SIGNAL(imageReady(QRect)));
    d_ptr->uid = QUuid::createUuid().toString();
}

// -------------------------------------------------------

MapLayer::MapLayer(MapLayerPrivate &dd, QObject *parent)
    :QObject(parent), d_ptr(&dd)
{
    setObjectName("MapLayer");
    d_ptr->q_ptr = this;
    connect(d_ptr, SIGNAL(needRender(QRect)), this, SIGNAL(imageReady(QRect)));
    d_ptr->uid = QUuid::createUuid().toString();
}

// -------------------------------------------------------

MapLayer::~MapLayer()
{
    delete d_ptr;
}

// -------------------------------------------------------

MapFrame *MapLayer::map() const
{
    Q_D(const MapLayer);
    return d->map;
}

// -------------------------------------------------------

int MapLayer::zOrder() const
{
    Q_D(const MapLayer);
    return d->zOrder;
}

// -------------------------------------------------------

QString MapLayer::name() const
{
    Q_D(const MapLayer);
    return d->name;
}

// -------------------------------------------------------

QString MapLayer::uid() const
{
    Q_D(const MapLayer);
    return d->uid;
}

// -------------------------------------------------------

bool MapLayer::visible() const
{
    Q_D(const MapLayer);
    return d->visible;
}

// -------------------------------------------------------

MapCamera *MapLayer::camera() const
{
    Q_D(const MapLayer);
    return d->camera;
}

// -------------------------------------------------------

void MapLayer::setMap(MapFrame *map)
{
    Q_D(MapLayer);
    if (map == d->map)
        return;
    MapFrame *m = d->map;
    d->map = map;
    if (m)
        m->unregisterLayer(this);
    if (d->map)
        d->map->registerLayer(this);

    emit mapChanged(d->map);
}

// -------------------------------------------------------

void MapLayer::setZOrder(int z)
{
    Q_D(MapLayer);
    if (z == d->zOrder)
        return;
    d->zOrder = z;
    emit zOrderChanged(d->zOrder);
}

// -------------------------------------------------------

void MapLayer::setName(QString name)
{
    Q_D(MapLayer);
    if (name == d->name)
        return;
    d->name = name;
    emit nameChanged(d->name);
}

// -------------------------------------------------------

void MapLayer::setUid(QString uid)
{
    Q_D(MapLayer);
    if (uid == d->uid)
        return;
    d->uid = uid;
    emit uidChanged(d->uid);
}

// -------------------------------------------------------

void MapLayer::setVisible(bool visible)
{
    Q_D(MapLayer);
    if (visible == d->visible)
        return;
    d->visible = visible;
    emit visibleChanged(d->visible);
}

// -------------------------------------------------------

void MapLayer::setCamera(MapCamera *camera)
{
    Q_D(MapLayer);
    d->camera = camera;
}

// -------------------------------------------------------

void MapLayer::show()
{
    setVisible(true);
}

// -------------------------------------------------------

void MapLayer::hide()
{
    setVisible(false);
}

// -------------------------------------------------------

void MapLayer::update(QPainter *painter, const QRect &rgn, MapOptions options)
{
    Q_ASSERT(painter);
    Q_D(MapLayer);
    d->render(painter, rgn, d->camera ? d->camera : d->map->camera(), options);
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

MapLayerWithObjects::MapLayerWithObjects(QObject *parent)
    : MapLayer(*new MapLayerWithObjectsPrivate, parent)
{
    d_ptr->q_ptr = this;
    setObjectName("MapLayerWithObjects");
    setName("MapLayerWithObject");
}

// -------------------------------------------------------

MapLayerWithObjects::MapLayerWithObjects(MapLayerWithObjectsPrivate &dd, QObject *parent)
    : MapLayer(dd, parent)
{
    d_ptr->q_ptr = this;
    setObjectName("MapLayerWithObjects");
    setName("MapLayerWithObject");
}

// -------------------------------------------------------

MapLayerWithObjects::~MapLayerWithObjects()
{

}

// -------------------------------------------------------

void MapLayerWithObjects::addObject(MapObject *mo)
{
    Q_D(MapLayerWithObjects);

    if (!mo || d->mObjects.contains(mo->uid()))
        return;

    d->lObjects.append(mo);
    d->mObjects.insert(mo->uid(), mo);
    connect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));
    mo->setLayer(this);
}

// -------------------------------------------------------

void MapLayerWithObjects::remObject(MapObject *mo)
{
    Q_D(MapLayerWithObjects);
    disconnect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));

    d->lObjects.removeAll(mo);
    d->mObjects.remove(d->mObjects.key(mo));
}

// -------------------------------------------------------

MapObject *MapLayerWithObjects::findObject(const QString &uid) const
{
    Q_D(const MapLayerWithObjects);
    return d->mObjects.value(uid);
}

// -------------------------------------------------------

void MapLayerWithObjects::clearLayer()
{
    Q_D(MapLayerWithObjects);
    d->lObjects.clear();
    d->mObjects.clear();
}

const QList<MapObject *> MapLayerWithObjects::objects() const
{
    Q_D(const MapLayerWithObjects);
    return d->lObjects;
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

