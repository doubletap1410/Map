#ifndef MAPLAYER_P_H
#define MAPLAYER_P_H

#include <QObject>
#include <QPainter>

#include "core/mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapLayer;
class MapLayerWithObjects;
class MapFrame;
class MapCamera;
class MapObject;

// -------------------------------------------------------

class MapLayerPrivate : public QObject
{
    Q_OBJECT

public:
    MapLayerPrivate(QObject *parent = NULL);
    virtual ~MapLayerPrivate() { }

public:
    MapFrame *map;     // карта
    QString uid;       // ид слоя
    QString name;      // имя слоя
    int zOrder;        // номер по порядку
    bool visible;      // видимость слоя

    MapCamera *camera; // указатель на камеру

public Q_SLOTS:
    virtual void render(QPainter *, QRectF const &, MapCamera const *, MapOptions = optNone) { }

Q_SIGNALS:
    void needRender(QRect = QRect());

public:
    Q_DECLARE_PUBLIC(MapLayer)
    Q_DISABLE_COPY(MapLayerPrivate)

    MapLayer *q_ptr;
};

// -------------------------------------------------------

class MapLayerWithObjectsPrivate : public MapLayerPrivate
{
    Q_OBJECT

public:
    MapLayerWithObjectsPrivate(QObject *parent = NULL);
    virtual ~MapLayerWithObjectsPrivate() { }

public:
    QList<MapObject*> lObjects;         // объекты слоя
    QMap<QString, MapObject*> mObjects; // быстрый доступ к объекту по ид

public slots:
    virtual void objectUidChanged(QString,QString);

public:
    Q_DECLARE_PUBLIC(MapLayerWithObjects)
    Q_DISABLE_COPY(MapLayerWithObjectsPrivate)

    MapLayerWithObjects *q_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPLAYER_P_H
