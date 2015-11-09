#ifndef MAPLAYERSYSTEM_H
#define MAPLAYERSYSTEM_H

#include <QObject>

#include "coord/mapcamera.h"
#include "layers/maplayer.h"
#include "layers/maplayer_p.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapLayerSystem;
class MapLayerSystemPrivate : public MapLayerWithObjectsPrivate
{
    Q_OBJECT

public:
    explicit MapLayerSystemPrivate(QObject *parent = 0);
    virtual ~MapLayerSystemPrivate();

public Q_SLOTS:
    virtual void render(QPainter *painter, const QRectF &rgn, const MapCamera *camera, MapOptions options = optNone);
    void drawObj(const MapObject *object, QPainter *painter, const QRectF &rgn);
};

// -------------------------------------------------------

class MapLayerSystem : public MapLayerWithObjects
{
    Q_OBJECT

public:
    explicit MapLayerSystem(QObject *parent = 0);
    virtual ~MapLayerSystem();

protected:
    explicit MapLayerSystem(MapLayerSystemPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(MapLayerSystem)
    Q_DISABLE_COPY(MapLayerSystem)
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
#endif // MAPLAYERSYSTEM_H
