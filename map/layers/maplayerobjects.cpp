#include <QDebug>
#include <QStack>

#include "coord/mapcamera.h"
#include "object/mapobject.h"
#include "drawer/mapdrawer.h"
#include "frame/mapframe.h"
#include "core/maptemplates.h"

#include "layers/maplayerobjects.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

struct CompareObjectsByNpp
{
    QHash<MapObject *, int> const *npp;
    CompareObjectsByNpp(QHash<MapObject *, int> const *_npp) : npp(_npp) { }
    bool operator ()(MapObject *a, MapObject *b) { return npp->value(a) < npp->value(b); }
};

// -------------------------------------------------------

MapLayerObjectsPrivate::MapLayerObjectsPrivate(QObject *parent)
    : MapLayerWithObjectsPrivate(parent), tree(new MapRTree), nppMin(0), nppMax(0)
{
}

// -------------------------------------------------------

MapLayerObjectsPrivate::~MapLayerObjectsPrivate()
{
}

// -------------------------------------------------------

void MapLayerObjectsPrivate::render(QPainter *painter, const QRectF &rgn, const MapCamera *camera, MapOptions options)
{
    Q_Q(MapLayerObjects);
    foreach (MapObject const *object, q->selectObjects(rgn, camera)) {
        QStack<MapObject const *> stack;
        stack.push(object);
        while (!stack.empty()) {
            MapObject const *o = stack.pop();
            if (o->drawer())
                o->drawer()->paint(o, painter, rgn, camera, options);

            foreach (MapObject const *child, o->childrenObjects())
                stack.push(child);
        }
    }
}

// -------------------------------------------------------

void MapLayerObjectsPrivate::validateMinMax()
{
    if (nppMin > -2000000000 && nppMax < 2000000000)
        return;

    nppMin = nppMax = 0;
    std::sort(lObjects.begin(), lObjects.end(), CompareObjectsByNpp(&npp));
    for (QListIterator<MapObject *> it(lObjects); it.hasNext(); )
        npp[it.next()] = ++nppMax;
}

// -------------------------------------------------------

MapLayerObjects::MapLayerObjects(QObject *parent)
    : MapLayerWithObjects(*new MapLayerObjectsPrivate, parent)
{
    d_ptr->q_ptr = this;
    setObjectName("MapLayerObjects");
    setName("MapLayerObjects");
}

// -------------------------------------------------------

MapLayerObjects::MapLayerObjects(MapLayerObjectsPrivate &dd, QObject *parent)
    : MapLayerWithObjects(dd, parent)
{
    d_ptr->q_ptr = this;
    setObjectName("MapLayerObjects");
    setName("MapLayerObjects");
}

// -------------------------------------------------------

MapLayerObjects::~MapLayerObjects()
{
}

// -------------------------------------------------------

void MapLayerObjects::addObject(MapObject *mo)
{
    Q_D(MapLayerObjects);
    if (!mo)
        return;

    mo->setLayer(this);

    if (d->mObjects.contains(mo->uid()))
        return;

    if (!mo->parentObject()) {
        d->lObjects.append(mo);
        d->tree->insert(mo);
        d->npp[mo] = ++d->nppMax;
    }
    else
        d->localObjects.append(mo);

    d->mObjects.insert(mo->uid(), mo);
    connect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));
    connect(mo, SIGNAL(aniNotify(MapObject*)), SLOT(updateObj(MapObject*)));
}

// -------------------------------------------------------

void MapLayerObjects::remObject(MapObject *mo)
{
    Q_D(MapLayerObjects);
    disconnect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));
    disconnect(mo, SIGNAL(aniNotify(MapObject*)), this, SLOT(updateObj(MapObject*)));

    d->tree->deleteObj(mo);
    d->lObjects.removeAll(mo);
    d->mObjects.remove(d->mObjects.key(mo));
    d->localObjects.removeAll(mo);
    d->npp.remove(mo);
}

// -------------------------------------------------------

void MapLayerObjects::removeObjects(const QList<MapObject *> &objList)
{
    Q_D(MapLayerObjects);
    foreach (MapObject *mo, objList) {
        disconnect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));

        d->lObjects.removeAll(mo);
        d->mObjects.remove(d->mObjects.key(mo));
        d->localObjects.removeAll(mo);
        d->npp.remove(mo);
    }

    d->tree->deleteObj(objList);
}

// -------------------------------------------------------

MapObject *MapLayerObjects::findObject(const QString &uid) const
{
    Q_D(const MapLayerObjects);
    return d->mObjects.value(uid);
    return NULL;
}

// -------------------------------------------------------

QList<MapObject *> MapLayerObjects::findObjects(int key, QVariant sem, QList<MapObject *> *continueFind) const
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
        Q_ASSERT(ll);
        for (QListIterator<MapObject *> lIt(*ll); lIt.hasNext();) {
            MapObject *obj = lIt.next();
            if (obj)
                if (obj->semantic(key) == sem)
                    result.append(obj);
        }
    }

    return result;
}

// -------------------------------------------------------

QList<MapObject *> MapLayerObjects::findObjects(QHash<int, QVariant> sem, QList<MapObject *> *continueFind) const
{
    Q_D(const MapLayerObjects);
    QList<QList<MapObject *> const*> lSouces;
    if (continueFind)
        lSouces.append(continueFind);
    else {
        lSouces.append(&d->lObjects);
        lSouces.append(&d->localObjects);
    }

    bool ok;
    QList<MapObject *> result;
    for (QListIterator<QList<MapObject *> const*> llIt(lSouces); llIt.hasNext();) {
        QList<MapObject *> const *ll = llIt.next();
        for (QListIterator<MapObject *> lIt(*ll); lIt.hasNext();) {
            MapObject *obj = lIt.next();
            if (obj) {
                ok = true;
                for (QHashIterator<int, QVariant> semIt(sem); semIt.hasNext(); ) {
                    semIt.next();
                    if (!(ok = obj->semantic(semIt.key()) == semIt.value()))
                        break;
                }
                if (ok)
                    result.append(obj);
            }
        }
    }
    return result;
}

// -------------------------------------------------------

QList<MapObject *> MapLayerObjects::findObjects(QVariantMap sem, QList<MapObject *> *continueFind) const
{
    Q_D(const MapLayerObjects);
    QList<QList<MapObject *> const*> lSouces;
    if (continueFind)
        lSouces.append(continueFind);
    else {
        lSouces.append(&d->lObjects);
        lSouces.append(&d->localObjects);
    }

    bool ok;
    QList<MapObject *> result;
    for (QListIterator<QList<MapObject *> const*> llIt(lSouces); llIt.hasNext();) {
        QList<MapObject *> const *ll = llIt.next();
        for (QListIterator<MapObject *> lIt(*ll); lIt.hasNext();) {
            MapObject *obj = lIt.next();
            if (obj) {
                ok = true;
                for (QMapIterator<QVariantMap::key_type, QVariantMap::mapped_type> semIt(sem); semIt.hasNext(); ) {
                    semIt.next();
                    if (!(ok = obj->semantic(semIt.key()) == semIt.value()))
                        break;
                }
                if (ok)
                    result.append(obj);
            }
        }
    }
    return result;
}

// -------------------------------------------------------

MapObject *MapLayerObjects::findObject(int key, QVariant sem, QList<MapObject *> *continueFind) const
{
    QList<MapObject *> result = findObjects(key, sem, continueFind);
    if (result.isEmpty())
        return NULL;
    return result.first();
}

// -------------------------------------------------------

MapObject *MapLayerObjects::findObject(QHash<int, QVariant> sem, QList<MapObject *> *continueFind) const
{
    QList<MapObject *> result = findObjects(sem, continueFind);
    if (result.isEmpty())
        return NULL;
    return result.first();
}

// -------------------------------------------------------

MapObject *MapLayerObjects::findObject(QVariantMap sem, QList<MapObject *> *continueFind) const
{
    QList<MapObject *> result = findObjects(sem, continueFind);
    if (result.isEmpty())
        return NULL;
    return result.first();
}

// -------------------------------------------------------

void MapLayerObjects::clearLayer()
{
    Q_D(MapLayerObjects);
    d->mObjects.clear();
    d->npp.clear();
    d->tree->clear();

    foreach (MapObject *mo, d->lObjects) {
        disconnect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));
        disconnect(mo, SIGNAL(aniNotify(MapObject*)), this, SLOT(updateObj(MapObject*)));
        mo->setLayer(NULL);
    }
    d->lObjects.clear();
    foreach (MapObject *mo, d->localObjects) {
        disconnect(mo, SIGNAL(uidChanged(QString,QString)), d, SLOT(objectUidChanged(QString,QString)));
        disconnect(mo, SIGNAL(aniNotify(MapObject*)), this, SLOT(updateObj(MapObject*)));
        mo->setLayer(NULL);
    }
    d->localObjects.clear();
}

// -------------------------------------------------------

const QList<MapObject *> MapLayerObjects::selectObjects(QRectF const &rgn, MapCamera const *cam) const
{
    Q_D(const MapLayerObjects);

// TODO: избавиться от FictiveSize
    int FictiveSize = 30;

    QRectF tmp = rgn.adjusted(-FictiveSize, -FictiveSize, +FictiveSize, +FictiveSize);

    if (!cam)
        cam = camera();
    QPolygonF worldPos(cam->toWorld().map(tmp));
    QList<MapObject *> res = d->tree->find(worldPos);
#if 0
    // сортировка порядка элементов в слое закрашена для скорости
    std::sort(res.begin(), res.end(), CompareObjectsByNpp(&d->npp));
#endif

    res.reserve(res.size() + d->localObjects.size());
    std::copy_if(d->localObjects.begin(), d->localObjects.end(), std::back_inserter(res), std::mem_fun(&MapObject::drawer));
    return res;
}

// -------------------------------------------------------

const QList<MapObject *> MapLayerObjects::selectObjects(QPointF const &pos, MapCamera const *cam) const
{
    return selectObjects(QRectF(pos, QSizeF()), cam);
}

// -------------------------------------------------------

const QList<MapObject *> &MapLayerObjects::objectsFree() const
{
    Q_D(const MapLayerObjects);
    return d->lObjects;
}

// -------------------------------------------------------

const QList<MapObject *> &MapLayerObjects::objectsChilds() const
{
    Q_D(const MapLayerObjects);
    return d->localObjects;
}

// -------------------------------------------------------

const QList<MapObject *> MapLayerObjects::objects() const
{
    Q_D(const MapLayerObjects);
    return QList<MapObject*>() << d->lObjects << d->localObjects;
}

// -------------------------------------------------------

void MapLayerObjects::objectChanged(MapObject *obj, QRect rect)
{
    Q_D(MapLayerObjects);
    if (obj)
        d->tree->movedObj(obj);

    emit imageReady(rect);
    //    updateScene(rect);
}

// -------------------------------------------------------

void MapLayerObjects::updateObj(MapObject *mo)
{
    if (!mo || !mo->drawer())
        return;
//    objectChanged(mo, mo->drawer()->localBound(mo, d_ptr->camera).toRect());
    objectChanged(mo, QRect());
}

// -------------------------------------------------------

void MapLayerObjects::raiseObject(MapObject *object)
{
    if (!object)
        return;
    if (object->layer() != this)
        return;
    Q_D(MapLayerObjects);

    if (object->parentObject()) {
        QList<MapObject*> *lobj = &d->localObjects;
        QList<MapObject*>::iterator it = std::find(lobj->begin(), lobj->end(), object);
        if (it == lobj->end())
            return; // навсякий
        lobj->erase(it);
        lobj->push_front(object);
    }
    else {
        d->npp[object] = ++d->nppMax;
        d->validateMinMax();
    }
}

// -------------------------------------------------------

void MapLayerObjects::lowerObject(MapObject *object)
{
    if (!object)
        return;
    if (object->layer() != this)
        return;
    Q_D(MapLayerObjects);

    if (object->parentObject()) {
        QList<MapObject*> *lobj = &d->localObjects;
        QList<MapObject*>::iterator it = std::find(lobj->begin(), lobj->end(), object);
        if (it == lobj->end())
            return; // навсякий
        lobj->erase(it);
        lobj->push_front(object);
    }
    else {
        d->npp[object] = --d->nppMin;
        d->validateMinMax();
    }
}

// -------------------------------------------------------

void MapLayerObjects::stackUnderObject(MapObject *object, MapObject *parentObject)
{
    if (!object || !parentObject)
        return;
    if (object->layer() != this || parentObject->layer() != this || object->parentObject() != parentObject->parentObject())
        return;
    Q_D(MapLayerObjects);
    if (object->parentObject()) {
        QList<MapObject*> *lobj = &d->localObjects;
        QList<MapObject*>::iterator it = std::find(lobj->begin(), lobj->end(), object);
        if (it == lobj->end())
            return; // навсякий
        lobj->erase(it);
        lobj->push_front(object);
    }
    else
        d->npp[object] = d->npp[parentObject] + 1;
}

// -------------------------------------------------------

void MapLayerObjects::lockLayer()
{
    Q_D(MapLayerObjects);
    d->tree->lock();
}

void MapLayerObjects::unlockLayer()
{
    Q_D(MapLayerObjects);
    d->tree->unlock();
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
