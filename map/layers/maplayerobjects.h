#ifndef MAPLAYEROBJECTS_H
#define MAPLAYEROBJECTS_H

#include <QScopedPointer>
#include "layers/maprtree.h"

#include "layers/maplayer.h"
#include "layers/maplayer_p.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapLayerObjects;
class MapLayerObjectsPrivate : public MapLayerWithObjectsPrivate
{
    Q_OBJECT

public:
    explicit MapLayerObjectsPrivate(QObject *parent = 0);
    virtual ~MapLayerObjectsPrivate();

public Q_SLOTS:
    virtual void render(QPainter *painter, const QRectF &rgn, const MapCamera *camera, MapOptions options = optNone);

public:
    QScopedPointer<MapRTree> tree;      // дерево объектов
    QList<MapObject*> localObjects;     // объекты с пиксельной привязкой

    int nppMin;
    int nppMax;
    QHash<MapObject *, int> npp;

    void validateMinMax();

public:
    Q_DECLARE_PUBLIC(MapLayerObjects)
    Q_DISABLE_COPY(MapLayerObjectsPrivate)

    MapLayerObjects *q_ptr;
};

// -------------------------------------------------------

class MapLayerObjects : public MapLayerWithObjects
{
    Q_OBJECT

public:    
    explicit MapLayerObjects(QObject *parent = 0);
    virtual ~MapLayerObjects();

    virtual void addObject(MapObject *mo);
    virtual void remObject(MapObject *mo);
    void removeObjects(QList<MapObject *> const &objList);

    virtual MapObject *findObject(QString const &uid) const;
    QList<MapObject*> findObjects(int key, QVariant sem, QList<MapObject*> *continueFind = NULL) const;
    QList<MapObject*> findObjects(QHash<int, QVariant> sem, QList<MapObject*> *continueFind = NULL) const;
    QList<MapObject*> findObjects(QVariantMap sem, QList<MapObject*> *continueFind = NULL) const;
    template <typename T>
    QList<MapObject*> findObjects(T, QList<MapObject*> *continueFind = NULL) const;
    MapObject *findObject(int key, QVariant sem, QList<MapObject*> *continueFind = NULL) const;
    MapObject *findObject(QHash<int, QVariant> sem, QList<MapObject*> *continueFind = NULL) const;
    MapObject *findObject(QVariantMap sem, QList<MapObject*> *continueFind = NULL) const;

    virtual void clearLayer();

    virtual QList<MapObject*> const selectObjects(QRectF const &rgn, MapCamera const *cam = NULL) const;
    virtual QList<MapObject*> const selectObjects(QPointF const &pos, MapCamera const *cam = NULL) const;

    virtual QList<MapObject*> const &objectsFree() const;
    virtual QList<MapObject*> const &objectsChilds() const;
    virtual QList<MapObject*> const objects() const;

public Q_SLOTS:
    void objectChanged(MapObject *obj, QRect rect);
    void updateObj(MapObject *);


    virtual void raiseObject(MapObject *object);
    virtual void lowerObject(MapObject *object);
    virtual void stackUnderObject(MapObject *object, MapObject *parentObject);

    void lockLayer();
    void unlockLayer();

protected:
    explicit MapLayerObjects(MapLayerObjectsPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(MapLayerObjects)
    Q_DISABLE_COPY(MapLayerObjects)
};

// -------------------------------------------------------

template <typename T>
QList<MapObject*> MapLayerObjects::findObjects(T t, QList<MapObject *> *continueFind) const
{
    Q_D(const MapLayerObjects);
    QList<QList<MapObject *> const*> lSouces;
    if (continueFind)
        lSouces.append(continueFind);
    else {
        lSouces.append(&d->lObjects);
        lSouces.append(&d->localObjects);
    }

    QList<MapObject *> result;
    for (QListIterator<QList<MapObject *> const*> llIt(lSouces); llIt.hasNext();) {
        QList<MapObject *> const *ll = llIt.next();
        for (QListIterator<MapObject *> lIt(*ll); lIt.hasNext();) {
            MapObject *obj = lIt.next();
            if (obj && t(obj))
                result.append(obj);
        }
    }
    return result;
}

} // namespace minigis

// -------------------------------------------------------

#endif // MAPLAYEROBJECTS_H
