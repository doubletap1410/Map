#include <QCursor>
#include <QTransform>
#include <QUuid>

#include "coord/mapcamera.h"
#include "frame/mapframe.h"
#include "frame/mapsettings.h"
#include "object/mapobject.h"
#include "drawer/mapdrawer.h"
#include "core/mapmath.h"
#include "layers/maplayersystem.h"
#include "layers/maplayerobjects.h"
#include "core/maptemplates.h"
#include "core/mapmetric.h"

#include "interact/maphelperevent.h"
#include "interact/maphelper_p.h"

#include "maphelpersubhelpers.h"

//----------------------------------------------------------
#define D_MAP if (d->map == NULL) return false;
//----------------------------------------------------------

#define MoveDelta 100                               // дельта для нанесения новой точки

#define MarkColorPen      QColor(255, 0, 0)         // цвет удаляемого маркера
#define MarkColorBrush    QColor(64, 224, 208, 200) // цвет меток
#define MarkerDeletePen   QColor(0, 0, 255)         // цвет удаляемого маркера
#define MarkerDeleteBrush QColor(255, 20, 147)      // цвет удаляемого маркера
#define MarkerActivePen   QColor(0, 0, 0)           // цвет удаляемого маркера
#define MarkerActiveBrush QColor(0, 0, 255)         // цвет удаляемого маркера

// цвета маркеров
#define LineMarkPenColor    QColor(255, 140, 0)
#define PointMarkPenColor   QColor(0, 0, 255)
#define PointMarkBrushColor QColor(255, 255, 0)
#define PolyMarkPenColor    QColor(46, 139, 87)
#define PolyMarkBrushColor  QColor(238, 130, 238, 100)

#define StandartHelperMarker 3

// --------------------------------------

template <class T> Q_INLINE_TEMPLATE uint qHash(QPointer<T> const &ptr)
{
    return QT_PREPEND_NAMESPACE(qHash)<T>(ptr.data());
}

// --------------------------------------

//! вспомогательные функции
namespace  {

QPoint deaccelerate(QPoint &speed, int a = 1, int max = 64)
{
    int x = qBound(-max, speed.x(), max);
    int y = qBound(-max, speed.y(), max);
    x = (x == 0) ? 0 : (x > 0) ? qMax(0, x - a) : qMin(0, x + a);
    y = (y == 0) ? 0 : (y > 0) ? qMax(0, y - a) : qMin(0, y + a);
    return QPoint(x, y);
}

minigis::MapLayerObjects * initStandartLayer(minigis::MapFrame *map) {
    minigis::MapLayerObjects *l = qobject_cast<minigis::MapLayerObjects*>(map->layerByName(minigis::StandartLayerName));
    if (!l) {
        l = new minigis::MapLayerObjects;
        l->setName(minigis::StandartLayerName);
        l->setCamera(map->camera());
        map->registerLayer(l);
    }
    return l;
}

minigis::MapLayerSystem * initSystemLayer(minigis::MapFrame *map) {
    minigis::MapLayerSystem *l = qobject_cast<minigis::MapLayerSystem*>(map->layerByName(minigis::SystemLayerName));
    if (!l) {
        l = new minigis::MapLayerSystem;
        l->setName(minigis::SystemLayerName);
        l->setCamera(map->camera());
        map->registerLayer(l);
    }
    return l;
}

minigis::MapLayerSystem * initMarkerLayer(minigis::MapFrame *map) {
    QString markerLayerName = QString::fromUtf8("Слой маркеров");
    minigis::MapLayerSystem *l = qobject_cast<minigis::MapLayerSystem*>(map->layerByName(markerLayerName));
    if (!l) {
        l = new minigis::MapLayerSystem;
        l->setName(markerLayerName);
        l->setCamera(map->camera());
        l->setZOrder(-1);
        map->registerLayer(l);
    }
    return l;
}

QPointF markerBound(QPointF pos, QRectF boundRect)
{
    return QPointF(
                qBound(boundRect.left() + 25, pos.x(), boundRect.right()  - 25),
                qBound(boundRect.top()  + 25, pos.y(), boundRect.bottom() - 25)
                );
}

void clearIgnorGenList(QHash<minigis::MapObject*, bool> &list) {
    for (QHashIterator<minigis::MapObject*, bool> it(list); it.hasNext(); ) {
        it.next();
        minigis::MapObject *mo = it.key();
        mo->setAttribute(minigis::attrGenIgnore, it.value());
    }
    list.clear();
}

// -------------------------------------- Functors

struct TransformPoint
{
    TransformPoint(QPolygonF &polygon, const QTransform &matrix)
        : points(polygon), trans(matrix) { }

    void operator()(const QPointF &p) { points.append(trans.map(p)); }

private:
    QPolygonF &points;
    const QTransform &trans;
};

}

// --------------------------------------

namespace minigis {

// --------------------------------------

MapSubHelper::MapSubHelper(MapFrame *map_, QObject *parent)
    : MapSubHelperParent(parent), isMouseDown(false), map(map_)
{

}

MapSubHelper::~MapSubHelper()
{

}

// --------------------------------------

MapSubHelperMove::MapSubHelperMove(MapFrame *map_, QObject *parent)
    : MapSubHelper(map_, parent), isTap(false), scrollState(NoScroll)
{

}

MapSubHelperMove::~MapSubHelperMove()
{
    scrollState = NoScroll;
}

bool MapSubHelperMove::start(QPointF pos)
{
    MHEMovePress mhep(pos);
    emit handler(&mhep);

    isMouseDown = true;
    lastScreenPos = pos;
    pressPos = pos;

    isTap = true;

    scrollState = ManualScroll;
    dragPos = QCursor::pos();
    if (scrollState == AutoScroll) {
        scrollState = NoScroll;
        speed = QPoint(0, 0);
    }

    if (!ticker.isActive())
        ticker.start(20, this);

    return true;
}

bool MapSubHelperMove::finish(QPointF pos)
{
    if (!isMouseDown)
        return false;

    MHEMoveRelease mher(pos);
    emit handler(&mher);

    if (isTap) {
        MapCamera *cam = map->camera();
        MapOptions opt = map->settings()->mapOptions();
        QRectF rgn(pos, QSizeF());
        int sizeR = 20;
        rgn.adjust(-sizeR, -sizeR, sizeR, sizeR);
        QList<MapObject*> lo(map->selectObjects(rgn, cam));
        QList<MapObject*> reslo;
        reslo.reserve(lo.size());
        foreach (MapObject *mo, lo) {
            if (mo->drawer()->hit(mo, rgn, cam, opt))
                reslo.append(mo);
        }

        MHETap mhtap(reslo, pos);
        emit handler(&mhtap);
    }

    isMouseDown = false;
    QPointF dt(pos - lastScreenPos);
    map->camera()->moveBy(moveValidate(map->camera()->toWorld(pos) - map->camera()->toWorld(lastScreenPos)));

    sendWorldChanged(dt);

    if (scrollState == ManualScroll) {
#if 0
            QLineF line(QPointF(), speed);
            qreal l = line.length();
            if (qAbs(l) < 0.1) {
                qDebug() << "qAbs(l) < 0.1" << l << speed;
//                return true;
            }
            if (l > 128)
                line.setLength(128);
            speedL = line.unitVector().p2();
#endif

        scrollState = AutoScroll;
        map->updateScene();
    }

    return true;
}

bool MapSubHelperMove::proccessing(QPointF pos)
{
    if (!isMouseDown)
        return false;

    if (qFuzzyCompare(pos.x(), lastScreenPos.x()) &&
            qFuzzyCompare(pos.y(), lastScreenPos.y()))
        return false;

    QPointF dt(pos - lastScreenPos);
    map->camera()->moveBy(map->camera()->toWorld(pos) - map->camera()->toWorld(lastScreenPos));
    lastScreenPos = pos;

    if (isTap) {
        QPoint p((pressPos - pos).toPoint());
        isTap = qAbs(p.x()) < 10 && qAbs(p.y()) < 10;
    }

    sendWorldChanged(dt);

    map->updateScene();

    return true;
}

void MapSubHelperMove::initData(const QRectF &rgn)
{
    moveRgn = rgn;
}

void MapSubHelperMove::timerEvent(QTimerEvent *event)
{
    bool used = false;

    if (scrollState == ManualScroll) {
        used = true;
        speed = (QCursor::pos() - dragPos) * 0.5;
        dragPos = QCursor::pos();
    }

    if (scrollState == AutoScroll) {
        used = true;

        speed = deaccelerate(speed, 2, 64);

        QPointF start = map->camera()->toWorld(QPointF());
        QPointF offset = map->camera()->toWorld(speed) - start;
        map->camera()->moveBy(offset);
        if (speed.isNull())
            scrollState = NoScroll;

        map->camera()->moveTo(moveValidate(map->camera()->position()));

        sendWorldChanged(speed);

        map->updateScene();
    }

    if (!used)
        ticker.stop();

    QObject::timerEvent(event);
}

QPointF MapSubHelperMove::moveValidate(const QPointF &p) const
{
    if (moveRgn.isNull())
        return p;
    QPointF mp(map->camera()->position());
    QPointF r(p + mp);
    return QPointF(
                qBound(moveRgn.left(), r.x(), moveRgn.right()),
                qBound(moveRgn.top(),  r.y(), moveRgn.bottom())
                ) - mp;
}

void MapSubHelperMove::sendWorldChanged(const QPointF &dt)
{
    QTransform tr;
    tr.translate(dt.x(), dt.y());
    emit worldChanged(tr);
}

// --------------------------------------

MapSubHelperScale::MapSubHelperScale(MapFrame *map_, QObject *parent)
    : MapSubHelper(map_, parent)
{

}

MapSubHelperScale::~MapSubHelperScale()
{

}

bool MapSubHelperScale::start(QPointF pos)
{
    isMouseDown = true;
    pressPos = pos;
    prevScale = map->camera()->scale();

    return true;
}

bool MapSubHelperScale::finish(QPointF pos)
{
    if (!isMouseDown)
        return false;

    isMouseDown = false;
    calculate(pos);
    map->updateScene();

    return true;
}

bool MapSubHelperScale::proccessing(QPointF pos)
{
    if (!isMouseDown || qFuzzyCompare(pos.y(), pressPos.y()))
        return false;

    calculate(pos);
    map->updateScene();

    return true;
}

QTransform MapSubHelperScale::scale(MapCamera *cam, const QPointF &mid, int delta)
{
    if (!cam || delta == 0)
        return QTransform();

    qreal lastScale = cam->scale();

    cam->moveBy(cam->position() - cam->toWorld(mid));
    delta < 0 ? cam->scaleOut(-delta) : cam->scaleIn(delta);
    cam->moveBy(cam->toWorld(mid) - cam->position());

    qreal scale = cam->scale() / lastScale;

    QTransform tr;
    tr.translate(mid.x(), mid.y()).scale(scale, scale).translate(-mid.x(), -mid.y());

    return tr;
}

void MapSubHelperScale::calculate(const QPointF &screenPos)
{
    MapCamera *cam = map->camera();
    qreal lastScale = cam->scale();

    qreal dt = screenPos.y() - pressPos.y();
    int steps = qAbs(int(dt / (0.046875 * cam->screenSize().height()))); // 0.046875 = 3/8 * 1/8 * h

    QPointF dtt(cam->position() - cam->toWorld(pressPos));
    cam->setScale(prevScale);
    cam->moveBy(dtt);

    dt < 0 ? cam->scaleOut(steps) : cam->scaleIn(steps);

    dtt = cam->toWorld(pressPos) - cam->position();
    cam->moveBy(dtt);

    sendWorldChanged(pressPos, cam->scale() / lastScale);
}

void MapSubHelperScale::sendWorldChanged(const QPointF &mid, qreal scale)
{
    QTransform tr;
    tr.translate(mid.x(), mid.y()).scale(scale, scale).translate(-mid.x(), -mid.y());
    emit worldChanged(tr);
}

// --------------------------------------

MapSubHelperRotate::MapSubHelperRotate(MapFrame *map_, QObject *parent)
    : MapSubHelper(map_, parent)
{

}

MapSubHelperRotate::~MapSubHelperRotate()
{

}

bool MapSubHelperRotate::start(QPointF pos)
{
    isMouseDown = true;
    lastScreenPos  = pos;
    pressPos = pos;

    return true;
}

bool MapSubHelperRotate::finish(QPointF pos)
{
    if (!isMouseDown)
        return false;
    isMouseDown = false;

    QPointF mid = QPointF(map->camera()->screenSize().width() * 0.5,
                          map->camera()->screenSize().height() * 0.5);
    qreal angle = radToDeg(polarAngle(pos - mid) - polarAngle(lastScreenPos - mid));
    map->camera()->rotateBy(angle);

    sendWorldChanged(mid, angle);
    map->updateScene();

    return true;
}

bool MapSubHelperRotate::proccessing(QPointF pos)
{
    if (qFuzzyCompare(pos.x(), lastScreenPos.x()) &&
            qFuzzyCompare(pos.y(), lastScreenPos.y()))
        return false;

    QPointF mid(map->camera()->screenSize().width() * 0.5,
                map->camera()->screenSize().height() * 0.5);

    qreal angle = radToDeg(polarAngle(pos - mid) - polarAngle(lastScreenPos - mid));
    map->camera()->rotateBy(angle);
    lastScreenPos = pos;

    sendWorldChanged(mid, angle);
    map->updateScene();

    return true;
}

void MapSubHelperRotate::sendWorldChanged(const QPointF &mid, qreal angle)
{
    QTransform tr;
    tr.translate(mid.x(), mid.y()).rotate(angle).translate(-mid.x(), -mid.y());
    emit worldChanged(tr);
}

// --------------------------------------

MapSubHelperSelect::MapSubHelperSelect(MapFrame *map_, bool continuedSelection_, QObject *parent)
    : MapSubHelper(map_, parent), continuedSelection(continuedSelection_)
{
    systemLayer = initSystemLayer(map_);

    visualRubber = new MapObject;
    visualRubber->setSemantic(PenSemanticSquare,   QPen(Qt::black, 2, Qt::DashLine));
    visualRubber->setSemantic(BrushSemanticSquare, QBrush(Qt::NoBrush));
    visualRubber->setSemantic(MarkersSemantic, (int)hoSquare);
    visualRubber->setLayer(systemLayer);
}

MapSubHelperSelect::~MapSubHelperSelect()
{
    delete visualRubber;
}

bool MapSubHelperSelect::start(QPointF pos)
{
    isMouseDown = true;
    lastScreenPos = pos;

    rubberBand = QRectF(pos, QSizeF());
    visualRubber->setMetric(Metric(rectToPoly(rubberBand), Mercator));

    return true;
}

bool MapSubHelperSelect::finish(QPointF pos)
{
    Q_UNUSED(pos);
    if (!isMouseDown)
        return false;
    isMouseDown = false;

    // ------------------------------------------------

    QRectF updateRect = rubberBand.toRect();
    visualRubber->setMetric(Metric(rectToPoly(QRectF()), Mercator));

    QList<MapObject*> objList;
    foreach (MapObject *mo, map->selectObjects(rubberBand, map->camera()))
        if (mo->drawer()->hit(mo, rubberBand, map->camera(), map->settings()->mapOptions()))
            objList.append(mo);

    MHEObjectList current(objList);
    emit handler(&current);

    if (canSelect.isEmpty()) {
        // Deselect all objects
        foreach (MapObjectPtr obj, selectedObjectsFull)
            if (obj) {
                obj->setSelected(false);
                updateRect = updateRect.united(obj->drawer()->localBound(obj, map->camera()));
            }
        clearIgnorGenList(ignorGenList);
        selectedObjectsFull.clear();
    }
    else {
        foreach (MapObjectPtr obj, canSelect.keys()) {
            if (obj.isNull())
                continue;

            if (obj->selected()) {
                selectedObjectsFull.insert(obj);

                if (!ignorGenList.contains(obj.data())) {
                    QVariant var = obj->attribute(attrGenIgnore);
                    ignorGenList.insert(obj.data(), var.isNull() ? false : var.toBool());
                    obj->setAttribute(attrGenIgnore, true);
                }
            }
            else {
                selectedObjectsFull.remove(obj);
                obj->setAttribute(attrGenIgnore, ignorGenList.take(obj.data()));
                updateRect = updateRect.united(obj->drawer()->localBound(obj, map->camera()));
            }
        }

        // clear deselected objects
        // TODO: как могу появиться в списке невыделенные объекты
//        QSet<MapObjectPtr> tmp = selectedObjectsFull;
//        selectedObjectsFull.clear();
//        for (auto it = tmp.begin(); it != tmp.end(); ++it) {
//            MapObjectPtr obj = *it;
//            if (obj.isNull() || !obj->selected())
//                continue;

//            selectedObjectsFull.insert(obj);
//        }
    }

    QList<MapObject*> fullList;
    foreach (MapObjectPtr mo, selectedObjectsFull)
        fullList.append(mo.data());

    MHESelectedObjects selected(fullList);
    emit handler(&selected);

    canSelect.clear();

//    static const int dt = 8;
//    map->updateScene(updateRect.toRect().adjusted(-dt, -dt, dt, dt));
    map->updateScene();

    return true;
}

bool MapSubHelperSelect::proccessing(QPointF pos)
{
    if (!isMouseDown)
        return false;

    static const int adjRect = 10;

    QRectF updateRect = rubberBand;

    rubberBand = QRectF(lastScreenPos, pos).normalized();
    visualRubber->setMetric(Metric(rectToPoly(rubberBand), Mercator));

    if (rubberBand.isNull())
        rubberBand.adjust(-adjRect, -adjRect, adjRect, adjRect);

    updateRect = updateRect.united(rubberBand);

    QList<MapObject*> objList;
    foreach (MapObject *mo, map->selectObjects(rubberBand, map->camera()))
        if (mo->drawer()->hit(mo, rubberBand, map->camera(), map->settings()->mapOptions()))
            objList.append(mo);

    for (QMutableHashIterator<MapObject*, CanSelect> it(canSelect); it.hasNext(); ) {
        it.next();
        it.value().flag = false;
    }

    foreach (MapObjectPtr obj, objList) {
        if (!canSelect.contains(obj.data())) { // добавляем объект в кэш
            MHECanSelect query(obj);
            emit handler(&query);

            CanSelect sel;
            sel.beSelected = obj->selected();
            sel.canSelect  = query.result();
            canSelect.insert(obj.data(), sel);
        }
        CanSelect &sel = canSelect[obj.data()];
        sel.flag = true;
        if (sel.canSelect) { // выделяем объект
            obj->setSelected(!sel.beSelected);
            updateRect = updateRect.united(obj->drawer()->localBound(obj, map->camera()));
        }
    }

    for (QMutableHashIterator<MapObject*, CanSelect> it(canSelect); it.hasNext(); ) { // возвращаем выделение объекту
        it.next();
        MapObject *mo = it.key();
        CanSelect &cs = it.value();
        if (!cs.flag && cs.canSelect && mo) {
            if (mo->selected() != cs.beSelected)
                updateRect = updateRect.united(mo->drawer()->localBound(mo, map->camera()));
            mo->setSelected(cs.beSelected);
        }
    }

//    static const int dt = 8;
//    map->updateScene(updateRect.toRect().adjusted(-dt, -dt, dt, dt));
    map->updateScene();

    return true;
}

void MapSubHelperSelect::reject(bool)
{
    QRectF updateRect;
    if (continuedSelection) {
        foreach (MapObject *mo, selectedObjectsFull) {
            QRectF r = mo->drawer()->localBound(mo, map->camera());
            updateRect = updateRect.isEmpty() ? r : updateRect.united(r);
        }
    }
    else {
        foreach (MapObject *mo, selectedObjectsFull) {
            mo->setSelected(false);
            QRectF r = mo->drawer()->localBound(mo, map->camera());
            updateRect = updateRect.isEmpty() ? r : updateRect.united(r);
        }
    }

    canSelect.clear();
    selectedObjectsFull.clear();
    clearIgnorGenList(ignorGenList);

    if (!updateRect.isEmpty())
//        map->updateScene(updateRect.toRect());
        map->updateScene();
}

void MapSubHelperSelect::initData(MapObjectList *baseSelection)
{
    if (!baseSelection)
        return;

    QRectF updateRect;
    foreach (MapObjectPtr obj, *baseSelection) {
        if (!obj->parentObject())
            continue;

        if (!ignorGenList.contains(obj)) {
            QVariant var = obj->attribute(attrGenIgnore);
            ignorGenList.insert(obj.data(), var.isNull() ? false : var.toBool());
            obj->setAttribute(attrGenIgnore, true);
        }

        obj->setSelected(true);
        selectedObjectsFull.insert(obj);

        updateRect = updateRect.united(obj->drawer()->localBound(obj, map->camera()));
    }

    if (!updateRect.isNull()) {
//        static const int dt = 2;
//        map->updateScene(updateRect.toRect().adjusted(-dt, -dt, dt, dt));
        map->updateScene();
    }
}

void MapSubHelperSelect::deleteObject(QObject *obj)
{
    MapObject *mo = qobject_cast<MapObject*>(obj);
    if (!mo || !selectedObjectsFull.contains(mo))
        return;

    selectedObjectsFull.remove(mo);    
    if (ignorGenList.contains(mo))
        mo->setAttribute(attrGenIgnore, ignorGenList.take(mo));
    canSelect.remove(mo);

    QRectF updateRect = mo->drawer()->localBound(mo, map->camera());
    if (!updateRect.isNull()) {
//        static const int dt = 2;
//        map->updateScene(updateRect.toRect().adjusted(-dt, -dt, dt, dt));
        map->updateScene();
    }
}

void MapSubHelperSelect::deleteObjects(const QList<MapObject *> &objects)
{
    foreach (MapObject *obj, objects)
        deleteObject(obj);
}

void MapSubHelperSelect::appendCustomObjects(const QList<MapObject *> &baseSelection)
{
    QRectF updateRect;
    foreach (MapObject *obj, baseSelection) {
        if (!obj)
            continue;

        if (!ignorGenList.contains(obj)) {
            QVariant var = obj->attribute(attrGenIgnore);
            ignorGenList.insert(obj, var.isNull() ? false : var.toBool());
            obj->setAttribute(attrGenIgnore, true);
        }

        obj->setSelected(true);
        selectedObjectsFull.insert(obj);

        updateRect = updateRect.united(obj->drawer()->localBound(obj, map->camera()));
    }

    if (!updateRect.isNull()) {
//        static const int dt = 2;
//        map->updateScene(updateRect.toRect().adjusted(-dt, -dt, dt, dt));
        map->updateScene();
    }
}

QList<MapObjectPtr> MapSubHelperSelect::getSelectedObjects() const
{
    return selectedObjectsFull.toList();
}

// --------------------------------------
// --------------------------------------
// --------------------------------------

MapSubMultiHelper::MapSubMultiHelper(MapFrame *map_, QObject *parent)
    : MapSubHelper(map_, parent),
      subhelper(NULL)
{
    systemLayer   = initSystemLayer(map_);
    markerLayer   = initMarkerLayer(map_);
    standartLayer = initStandartLayer(map_);
}

MapSubMultiHelper::~MapSubMultiHelper()
{
    qDeleteAll(allSubhelpers);
    allSubhelpers.clear();

    subhelper = NULL;
}

bool MapSubMultiHelper::start(QPointF pos)
{
    pressPos = pos;
    lastScreenPos = pos;
    for (QVector<SubHelper*>::iterator it = allSubhelpers.begin(); it != allSubhelpers.end(); ++it) {
        SubHelper *sub = *it;
        bool f = sub->start(pos);
        if (f) {
            isMouseDown = true;
            subhelper = sub;

            return true;
        }
    }
    return false;
}

bool MapSubMultiHelper::finish(QPointF pos)
{
    if (!isMouseDown)
        return false;

    subhelper->finish(pos);

    isMouseDown = false;
    subhelper = NULL;
    return true;
}

bool MapSubMultiHelper::proccessing(QPointF pos)
{
    if (!isMouseDown)
        return false;

    subhelper->proccessing(pos);
    lastScreenPos = pos;

    return true;
}

void MapSubMultiHelper::reject(bool flag)
{
    foreach (SubHelper *sub, allSubhelpers)
        sub->reject(flag);
}

static bool subhelpersCompare(SubHelper *a, SubHelper *b)
{
    Q_ASSERT(a);
    Q_ASSERT(b);
    return a->type() < b->type();
}

MapSubHelperEdit::MapSubHelperEdit(MapFrame *map, QObject *parent)
    : MapSubMultiHelper(map, parent)
{
    dialogFlag.insert("isCachedMode", true);
    dialogFlag.insert("isChangePointMode", true);
    dialogFlag.insert("isRemovePointMode", false);
    dialogFlag.insert("canUseMarkMode", true);
    dialogFlag.insert("isAddPointMode", true);
    dialogFlag.insert("addTypeMode", 2);

    SubHelperMark   *subMark   = new SubHelperMark(this);
    SubHelperEdit   *subEdit   = new SubHelperEdit(this);
    SubHelperCustom *subCustom = new SubHelperCustom(this);
    allSubhelpers.append(subMark);
    allSubhelpers.append(subEdit);
    allSubhelpers.append(subCustom);

    std::sort(allSubhelpers.begin(), allSubhelpers.end(), subhelpersCompare);
}

MapSubHelperEdit::~MapSubHelperEdit()
{
    objectsMark.deleteAll();
}

void MapSubHelperEdit::reject(bool flag)
{
    MapSubMultiHelper::reject(flag);
    for (EditedObjects::iterator it = objectsMark.begin(), itEnd = objectsMark.end(); it != itEnd; ++it) {
        MapObject *key = *it;
        key->setSelected(false);
        disconnect(key, SIGNAL(metricChanged()), this, SLOT(recalcObject()));
        disconnect(key, SIGNAL(aniNotify(MapObject*)), this, SLOT(recalcObject(MapObject*)));
        disconnect(key, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObject(QObject*)));
        disconnect(key, SIGNAL(classCodeChanged(QString)), this, SLOT(changeClassCode(QString)));
        disconnect(key, SIGNAL(attributeChanged(int, QVariant)), this, SLOT(changeAttribute(int,QVariant)));
    }

    for (EditedObjects::iterator it = objectsMark.mBegin(), itEnd = objectsMark.mEnd(); it != itEnd; ++it) {
        MapObject *mark = *it;
        mark->setMetric(Metric(Mercator));
        mark->clearAttributes();
    }
    objectsMark.clear();
    clearIgnorGenList();

    if (flag) {
        MHEEditedObjects ed(objectsMark.edited());
        emit handler(&ed);
    }

    map->updateScene();
}

void MapSubHelperEdit::addObjects(MapObjectPtr mo, bool isLowPriority)
{
    MapObjectList list;
    list.append(mo);
    addObjects(list, isLowPriority);
}

void MapSubHelperEdit::addObjects(const MapObjectList &objects, bool isLowPriority)
{
    QRectF updateRect;

    int size = objectsMark.size();
    foreach (MapObjectPtr mo, objects) {
        if (!mo || mo->parentObject() || !mo->drawer() || objectsMark.contains(mo))
            continue;

        if (!mo->useMetricNotify())
            mo->setUseMetricNotify(true);
        connect(mo.data(), SIGNAL(metricChanged()), this, SLOT(recalcObject()));
        connect(mo.data(), SIGNAL(aniNotify(MapObject*)), this, SLOT(recalcObject(MapObject*)));

        if (!ignorGenList.contains(mo)) {
            QVariant var = mo->attribute(attrGenIgnore);
            ignorGenList.insert(mo, var.isNull() ? false : var.toBool());
            mo->setAttribute(attrGenIgnore, true);
        }

        updateRect = updateRect.united(mo->drawer()->localBound(mo, map->camera()));

        mo->setSelected(true);
        MapObject *obj;
        if (!(obj = objectsMark.freePop())) {
            obj = new MapObject;
            obj->setUid(QUuid::createUuid().toString());

            obj->setSemantic(PointSemantic      , QPointF(10, 10)                              );
            obj->setSemantic(PenSemantic        , QPen(PointMarkPenColor, 2, Qt::SolidLine)    );
            obj->setSemantic(BrushSemantic      , QBrush(PointMarkBrushColor, Qt::SolidPattern));
            obj->setSemantic(PenSemanticLine    , QPen(LineMarkPenColor, 7, Qt::SolidLine)     );
            obj->setSemantic(PenSemanticSquare  , QPen(PolyMarkPenColor, 1, Qt::SolidLine)     );
            obj->setSemantic(BrushSemanticSquare, QBrush(PolyMarkBrushColor, Qt::SolidPattern) );

            obj->setLayer(markerLayer);
        }

        HeOptions visual = opt & (hoPoint | hoLine | hoSquare | hoHelpers);
        obj->setSemantic(MarkersSemantic, (int)visual);
        obj->setMetric(Metric(mo->drawer()->controlPoints(mo, map->camera()), Mercator));
        obj->setClassCode(mo->classCode());
        obj->clearAttributes();
        obj->setAttribute(attrSmoothed, mo->attributeDraw(attrSmoothed));

        if (isLowPriority)
            objectsMark.push_back(mo.data(), obj);
        else
            objectsMark.push_front(mo.data(), obj);

        connect(mo.data(), SIGNAL(destroyed(QObject*)), SLOT(deleteObject(QObject*)), Qt::UniqueConnection);
        connect(mo.data(), SIGNAL(classCodeChanged(QString)), SLOT(changeClassCode(QString)), Qt::UniqueConnection);
        connect(mo.data(), SIGNAL(attributeChanged(int, QVariant)), SLOT(changeAttribute(int,QVariant)), Qt::UniqueConnection);
    }

    if (!updateRect.isNull()) {
//        static const int dt = 10;
//        map->updateScene(updateRect.toRect().adjusted(-dt, -dt, dt, dt));
        map->updateScene();
    }

    if (size != objectsMark.size())
        emit objectsCountChanged();
}

QList<MapObject *> MapSubHelperEdit::objects() const
{
    return objectsMark.objects();
}

MapObject *MapSubHelperEdit::object(MapObject *mark)
{
    return objectsMark.object(mark);
}

QList<MapObject *> MapSubHelperEdit::markers() const
{
    return objectsMark.markers();
}

MapObject *MapSubHelperEdit::marker(MapObject *obj)
{
    return objectsMark.marker(obj);
}

void MapSubHelperEdit::initData(const MapObjectList &objects, HeOptions options)
{
    objectsMark.clear();

    MHECachedMode mhCm;
    emit handler(&mhCm);

    MHEMovePress mp(QPointF(-1, -1));
    emit handler(&mp);

    bool isCachedMode = mhCm.result();
    setFlag("isCachedMode", isCachedMode);

    if (isCachedMode) {
        MHECanRemovePoint mhCrp;
        emit handler(&mhCrp);
        setFlag("isRemovePointMode", mhCrp.result());

        MHECanChangePoint mhCcp;
        emit handler(&mhCcp);
        setFlag("isChangePointMode", mhCcp.result());

        MHECanAddPoint mhCap;
        emit handler(&mhCap);
        setFlag("isAddPointMode", mhCap.result());
        setFlag("addType", mhCap.flag);
    }

    MHEMarkMode mhUmm;
    emit handler(&mhUmm);
    setFlag("canUseMarkMode", mhUmm.result());

    opt = options;

    if (!objects.isEmpty())
        addObjects(objects);

    emit dataInited(options);
}

void MapSubHelperEdit::recalcObject()
{
    MapObject *obj = qobject_cast<MapObject*>(sender());
    recalcObject(obj);
}

void MapSubHelperEdit::recalcObject(MapObject *obj)
{
    if (!obj)
        return;

    MapObject *mark = objectsMark.marker(obj);
    if (mark)
        mark->setMetric(Metric(obj->drawer()->controlPoints(obj, map->camera()), Mercator));
}

void MapSubHelperEdit::deleteObject(QObject *obj)
{
    MapObject *mobj = static_cast<minigis::MapObject *>(obj);

    objectsMark.remove(mobj);
    ignorGenList.remove(mobj);

    // FIXME: error on destruction
//    emit objectsCountChanged();
}

void MapSubHelperEdit::changeClassCode(QString cls)
{
    MapObject *obj = qobject_cast<MapObject *>(sender());
    MapObject *mark = objectsMark.marker(obj);
    if (mark)
        mark->setClassCode(cls);
}

void MapSubHelperEdit::changeAttribute(int key, QVariant attr)
{
    if (key != attrSmoothed)
        return;

    MapObject *obj = qobject_cast<MapObject *>(sender());
    MapObject *mark = objectsMark.marker(obj);
    if (mark)
        mark->setAttribute(key, attr);
}

void MapSubHelperEdit::correctWorldImpl(QTransform tr)
{
    emit correctSubWorld(tr);

//    for (EditedObjects::iterator it = objectsMark.mBegin(), itEnd = objectsMark.mEnd(); it != itEnd; ++it) {
//        MapObject *mark = *it;
//        if (!mark)
//            continue;
//        mark->setMetric(Metric(tr.map(mark->metric().toMercator()), Mercator));
//    }

    for (EditedObjects::hIterator it = objectsMark.hBegin(), itEnd = objectsMark.hEnd(); it != itEnd; ++it) {
        MapObject* key = it.key();
        MapObject* value = it.value();
        if (!key || !value)
            continue;
        value->setMetric(Metric(key->drawer()->controlPoints(key, map->camera()), Mercator));
    }

    map->updateScene();
}

void MapSubHelperEdit::changeObjectsImpl(QTransform tr, QRectF upd, bool flag)
{
    MapCamera *cam = map->camera();
    emit correctSubWorld2(tr);

    for (EditedObjects::const_hIterator it = objectsMark.hBegin(); it != objectsMark.hEnd(); ++it) {
        EditedObjects::ItemType global = it.key();
        EditedObjects::ItemType local  = it.value();

        QRectF r = global->drawer()->localBound(global, cam);

        for (int i = 0; i < global->metric().size(); ++i)
            global->setMetric(i, Metric(tr.map(global->metric().toMercator(i)), Mercator));

        r = r.united(global->drawer()->localBound(global, cam));

        upd = upd.united(r);

        local->setMetric(Metric(global->drawer()->controlPoints(global, cam), Mercator));
    }
//    static const int d = 30;
//    map->updateScene(upd.toRect().adjusted(-d, -d, d, d));
    map->updateScene();

    if (flag)
        objectsMark.editedAll();
}

void MapSubHelperEdit::objectHasChangedImpl(MapObject *obj)
{
    objectsMark.edited(obj);
}

void MapSubHelperEdit::clearIgnorGenList()
{
    ::clearIgnorGenList(ignorGenList);
}

// --------------------------------------------

MapSubHelperCreate::MapSubHelperCreate(MapFrame *map, HeOptions opts, QObject *parent)
    : MapSubMultiHelper(map, parent),
      currentObject(NULL), visualObject(NULL), ignoreGen(false), tmpObj(false)
{
    visualObject = new MapObject;

    visualObject->setLayer(markerLayer);

    dialogFlag.insert("isCachedMode", true);
    dialogFlag.insert("isAddPointMode", true);
    dialogFlag.insert("addTypeMode", 2);
    dialogFlag.insert("isChangePointMode", true);
    dialogFlag.insert("isRemovePointMode", true);
    dialogFlag.insert("canUseMarkMode", false);

    if (!opts.testFlag(hoDisableEdit)) {
        SubHelperEdit *subEdit = new SubHelperEdit(this);
        allSubhelpers.append(subEdit);
    }
    SubHelperAdd *subAdd  = new SubHelperAdd(this);
    allSubhelpers.append(subAdd);    

    std::sort(allSubhelpers.begin(), allSubhelpers.end(), subhelpersCompare);
}

MapSubHelperCreate::~MapSubHelperCreate()
{
    delete visualObject;
}

void MapSubHelperCreate::reject(bool flag)
{
    MapSubMultiHelper::reject(flag);

    visualObject->setMetric(Metric(Mercator));

    if (currentObject) {
        currentObject->setAttribute(attrGenIgnore, ignoreGen);

        disconnect(currentObject, SIGNAL(metricChanged()), this, SLOT(recalcObject()));
        disconnect(currentObject, SIGNAL(aniNotify(MapObject*)), this, SLOT(recalcObject(MapObject*)));
        disconnect(currentObject, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObject(QObject*)));
        disconnect(currentObject, SIGNAL(classCodeChanged(QString)), this, SLOT(changeClassCode(QString)));
        disconnect(currentObject, SIGNAL(attributeChanged(int, QVariant)), this, SLOT(changeAttribute(int,QVariant)));
    }

    if (tmpObj) {
        tmpObj = false;
        delete currentObject;
    }

    currentObject = NULL;
    map->updateScene();
}

void MapSubHelperCreate::addObjects(MapObjectPtr mo, bool isLowPriority)
{
    Q_UNUSED(isLowPriority);
    init(mo.data(), 0, QFlag(hoPoint | hoHelpers), false);
}

void MapSubHelperCreate::addObjects(const MapObjectList &objects, bool isLowPriority)
{
    if (objects.size() == 1)
        addObjects(objects.first(), isLowPriority);
}

QList<MapObject *> MapSubHelperCreate::objects() const
{
    QList<MapObject*> list;
    if (currentObject)
        list << currentObject;
    return list;
}

MapObject *MapSubHelperCreate::object(MapObject *mark)
{    
    return mark == visualObject ? currentObject : NULL;
}

QList<MapObject *> MapSubHelperCreate::markers() const
{
    QList<MapObject*> list;
    if (currentObject)
        list << visualObject;
    return list;
}

MapObject *MapSubHelperCreate::marker(MapObject *obj)
{
    return (obj && obj == currentObject) ? visualObject : NULL;
}

void MapSubHelperCreate::addPoint(QPointF const &p)
{
    static SubHelperAdd* const add = qobject_cast<SubHelperAdd*>(
                !allSubhelpers.isEmpty() ? allSubhelpers.last() : NULL);
    if (add) {
        add->start(p);
        add->finish(p);
    }
}

void MapSubHelperCreate::initData(MapObject *mo, int maxPoints, HeOptions options)
{    
    init(mo, maxPoints, options, true);
}

void MapSubHelperCreate::recalcObject()
{
    MapObject *obj = qobject_cast<MapObject*>(sender());
    recalcObject(obj);
}

void MapSubHelperCreate::recalcObject(MapObject *obj)
{
    if (!obj || obj != currentObject)
        return;

    visualObject->setMetric(Metric(currentObject->drawer()->controlPoints(currentObject, map->camera()), Mercator));
}

void MapSubHelperCreate::deleteObject(QObject *obj)
{
    MapObject *mobj = static_cast<minigis::MapObject *>(obj);
    if (!mobj || mobj != currentObject)
        return;

    reject(false);
    emit autoReject();
}

void MapSubHelperCreate::changeClassCode(QString cls)
{
    MapObject *obj = qobject_cast<MapObject *>(sender());
    if (!obj || obj != currentObject)
        return;

    visualObject->setClassCode(cls);
}

void MapSubHelperCreate::changeAttribute(int key, QVariant attr)
{
    if (key != attrSmoothed)
        return;

    MapObject *obj = qobject_cast<MapObject *>(sender());
    if (!obj || obj != currentObject)
        return;

    visualObject->setAttribute(key, attr);
}

void MapSubHelperCreate::changeObjectsImpl(QTransform tr, QRectF upd, bool)
{
    emit correctSubWorld2(tr);

    if (!currentObject)
        return;

    MapCamera *cam = map->camera();
    static const int d = 10;

    QRectF r = currentObject->drawer()->localBound(currentObject, cam);
    for (int i = 0; i < currentObject->metric().size(); ++i)
        currentObject->setMetric(i, Metric(tr.map(currentObject->metric().toMercator(i)), Mercator));
    r = r.united(currentObject->drawer()->localBound(currentObject, cam));
    upd = upd.united(r.adjusted(-d, -d, d, d));
    visualObject->setMetric(Metric(currentObject->drawer()->controlPoints(currentObject, cam), Mercator));

//    map->updateScene(upd.toRect());
   map->updateScene();
}

void MapSubHelperCreate::objectHasChangedImpl(MapObject *)
{

}

void MapSubHelperCreate::init(MapObject *mo, int maxPoints, HeOptions options, bool askQuestions)
{
    if (mo && !mo->drawer()) {
        qWarning() << "Object without drawer.";
        emit autoReject();
        return;
    }

    reject(false);

    if (askQuestions) {
        MHECachedMode mhCm;
        emit handler(&mhCm);
        bool isCachedMode = mhCm.result();
        setFlag("isCachedMode", isCachedMode);

        if (isCachedMode) {
            MHECanRemovePoint mhCrp;
            emit handler(&mhCrp);
            setFlag("isRemovePointMode", mhCrp.result());

            MHECanChangePoint mhCcp;
            emit handler(&mhCcp);
            setFlag("isChangePointMode", mhCcp.result());

            MHECanAddPoint mhCap;
            emit handler(&mhCap);
            setFlag("isAddPointMode", mhCap.result());
            setFlag("addType", mhCap.flag);
        }
    }

    visualObject->setMetric(Metric(Mercator));
    visualObject->clearAttributes();

    currentObject = mo;
    tmpObj = !currentObject;
    if (tmpObj) {
        currentObject = new MapObject;
        currentObject->setClassCode("{00000000-0000-409d-9621-f4899f6a4c6e}");
        currentObject->setLayer(standartLayer);
    }

    if (currentObject) {
        QVariant var = currentObject->attribute(attrGenIgnore);
        ignoreGen = var.isNull() ? false : var.toBool();
        currentObject->setAttribute(attrGenIgnore, true);

        if (!currentObject->useMetricNotify())
            currentObject->setUseMetricNotify(true);

        connect(currentObject, SIGNAL(metricChanged()), this, SLOT(recalcObject()));
        connect(currentObject, SIGNAL(aniNotify(MapObject*)), this, SLOT(recalcObject(MapObject*)));
        connect(currentObject, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObject(QObject*)), Qt::UniqueConnection);
        connect(currentObject, SIGNAL(classCodeChanged(QString)), SLOT(changeClassCode(QString)), Qt::UniqueConnection);
        connect(currentObject, SIGNAL(attributeChanged(int, QVariant)), SLOT(changeAttribute(int,QVariant)), Qt::UniqueConnection);

        QVector<QPolygonF> tmp = currentObject->drawer()->controlPoints(currentObject, map->camera());
        visualObject->setMetric(Metric(tmp, Mercator));
        visualObject->setClassCode(currentObject->classCode());
        visualObject->setAttribute(attrSmoothed, currentObject->attribute(attrSmoothed));
    }

    HeOptions visual = options & (hoPoint | hoLine | hoSquare | hoHelpers);

    visualObject->setSemantic(MarkersSemantic,     (int)visual);
    visualObject->setSemantic(PointSemantic,       QPointF(10, 10));
    visualObject->setSemantic(PenSemantic,         QPen(PointMarkPenColor, 2, Qt::SolidLine));
    visualObject->setSemantic(BrushSemantic,       QBrush(PointMarkBrushColor, Qt::SolidPattern));
    visualObject->setSemantic(PenSemanticLine,     QPen(LineMarkPenColor, 7, Qt::SolidLine));
    visualObject->setSemantic(PenSemanticSquare,   QPen(PolyMarkPenColor, 1, Qt::SolidLine));
    visualObject->setSemantic(BrushSemanticSquare, QBrush(PolyMarkBrushColor, Qt::SolidPattern));

    foreach (SubHelper *sub, allSubhelpers) {
        if (sub->type() == SubHelperAdd::Type)
            qobject_cast<SubHelperAdd*>(sub)->initData(currentObject, maxPoints, options);
        else if (sub->type() == SubHelperEdit::Type)
            qobject_cast<SubHelperEdit*>(sub)->initData(options);
    }

    if (currentObject) {
//        static const int dt = 2;
//        map->updateScene(visualObject->drawer()->boundRect(visualObject).toRect().adjusted(-dt, -dt, dt, dt));
        map->updateScene();
    }
}

void MapSubHelperCreate::correctWorldImpl(QTransform tr)
{
    emit correctSubWorld(tr);

    if (!currentObject)
        return;

    visualObject->setMetric(Metric(currentObject->drawer()->controlPoints(currentObject, map->camera()), Mercator));

//    static const int dt = 2;
//    map->updateScene(visualObject->drawer()->boundRect(visualObject).toRect().adjusted(-dt, -dt, dt, dt));
    map->updateScene();
}

// --------------------------------------------
// --------------------------------------------
// --------------------------------------------

SubHelperMark::SubHelperMark(MapSubMultiHelper *subhelper, QObject *parent)
    : SubHelper(subhelper, parent)
{
    midMark = new MapObject;
    midMark->setSemantic(PointSemantic, QPointF(8, 8));
    midMark->setSemantic(PenSemantic, QPen(MarkColorPen, 1, Qt::SolidLine));
    midMark->setSemantic(BrushSemantic, QBrush(MarkColorBrush, Qt::SolidPattern));
    midMark->setSemantic(ImageSemantic, QImage(":/minimap/ctrl/point.png"));
    midMark->setSemantic(MarkersSemantic, (int)hoPoint);
    midMark->setLayer(q_ptr->getSystemLayer());

    posMark = new MapObject;
    posMark->setSemantic(PointSemantic, QPointF(25, 25));
    posMark->setSemantic(PenSemantic, QPen(MarkColorPen, 1, Qt::SolidLine));
    posMark->setSemantic(BrushSemantic, QBrush(MarkColorBrush, Qt::SolidPattern));
    posMark->setSemantic(ImageSemantic, QImage(":/minimap/ctrl/move.png"));
    posMark->setSemantic(MarkersSemantic, (int)hoPoint);
    posMark->setLayer(q_ptr->getSystemLayer());

    rotateMark = new MapObject;
    rotateMark->setSemantic(PointSemantic, QPointF(25, 25));
    rotateMark->setSemantic(PenSemantic, QPen(MarkColorPen, 1, Qt::SolidLine));
    rotateMark->setSemantic(BrushSemantic, QBrush(MarkColorBrush, Qt::SolidPattern));
    rotateMark->setSemantic(ImageSemantic, QImage(":/minimap/ctrl/rotate.png"));
    rotateMark->setSemantic(MarkersSemantic, (int)hoPoint);
    rotateMark->setLayer(q_ptr->getSystemLayer());

    scaleMark = new MapObject;
    scaleMark->setSemantic(PointSemantic, QPointF(25, 25));
    scaleMark->setSemantic(PenSemantic, QPen(MarkColorPen, 1, Qt::SolidLine));
    scaleMark->setSemantic(BrushSemantic, QBrush(MarkColorBrush, Qt::SolidPattern));
    scaleMark->setSemantic(ImageSemantic, QImage(":/minimap/ctrl/scale.png"));
    scaleMark->setSemantic(MarkersSemantic, (int)hoPoint);
    scaleMark->setLayer(q_ptr->getSystemLayer());

    midAni    = new MarkerAnimation(midMark);
    posAni    = new MarkerAnimation(posMark);
    rotateAni = new MarkerAnimation(rotateMark);
    scaleAni  = new MarkerAnimation(scaleMark);

    int pause = 50;
    markersGroup.addAnimation(midAni);
    markersGroup.addAnimation(posAni,    pause);
    markersGroup.addAnimation(rotateAni, pause);
    markersGroup.addAnimation(scaleAni,  pause);

    midMark->setAttribute(attrOpacity,    0);
    posMark->setAttribute(attrOpacity,    0);
    rotateMark->setAttribute(attrOpacity, 0);
    scaleMark ->setAttribute(attrOpacity, 0);

    MapFrame *map = q_ptr->getMap();
    connect(midAni,    SIGNAL(valueChanged(QRect)), map, SLOT(updateScene(QRect)));
    connect(posAni,    SIGNAL(valueChanged(QRect)), map, SLOT(updateScene(QRect)));
    connect(rotateAni, SIGNAL(valueChanged(QRect)), map, SLOT(updateScene(QRect)));
    connect(scaleAni,  SIGNAL(valueChanged(QRect)), map, SLOT(updateScene(QRect)));

    connect(this, SIGNAL(changeObjects(QTransform,QRectF,bool)), q_ptr, SLOT(changeObjects(QTransform,QRectF,bool)));
    connect(q_ptr, SIGNAL(correctSubWorld(QTransform)), this, SLOT(correctMarkers(QTransform)));
    connect(q_ptr, SIGNAL(objectsCountChanged()), this, SLOT(checkMarkers()));
    connect(q_ptr, SIGNAL(dataInited(HeOptions)), this, SLOT(upMarkers(HeOptions)));
}

SubHelperMark::~SubHelperMark()
{
    delete midMark;
    delete posMark;
    delete rotateMark;
    delete scaleMark;

    delete midAni;
    delete posAni;
    delete rotateAni;
    delete scaleAni;
}

bool SubHelperMark::start(QPointF pos)
{
    isMarkMode = 0;
    if (midMark->metric().size() == 0 || markersGroup.isRunning())
        return false;

    static const qreal r = 625; // 25 * 25
    if (lengthR2Squared(scaleMark->metric().toMercator(0).first() - pos) < r)
        isMarkMode = 4;
    else if (lengthR2Squared(rotateMark->metric().toMercator(0).first() - pos) < r)
        isMarkMode = 3;
    else if (lengthR2Squared(posMark->metric().toMercator(0).first() - pos) < r)
        isMarkMode = 2;
    else if (lengthR2Squared(midMark->metric().toMercator(0).first() - pos) < r)
        isMarkMode = 1;

    return isMarkMode > 0;
}

bool SubHelperMark::finish(QPointF pos)
{
    changeMarkers(pos, true);
    return true;
}

bool SubHelperMark::proccessing(QPointF pos)
{
    changeMarkers(pos, false);
    return true;
}

void SubHelperMark::reject(bool)
{
    clearMarkers();
}

void SubHelperMark::clearMarkers()
{
    if (midMark->metric().size() == 0)
        return;

    QPointF boundmid = markerBound(midMark->metric().toMercator(0).first(),
                                   QRectF(QPointF(), q_ptr->getCamera()->screenSize()));

    midAni->setEndPosition(boundmid);
    posAni->setEndPosition(boundmid);
    rotateAni->setEndPosition(boundmid);
    scaleAni->setEndPosition(boundmid);

    midAni->setEndOpacity(0);
    posAni->setEndOpacity(0);
    rotateAni->setEndOpacity(0);
    scaleAni->setEndOpacity(0);

    markersGroup.start();
}

void SubHelperMark::checkMarkers()
{
    QList<MapObject*> list = q_ptr->objects();
    int size = list.size();
    if (size && q_ptr->flag("canUseMarkMode").toBool()) {
        bool one = size == 1 && list.first()->drawer()->canUseMarkMode();
        if (one || size > 1)
            initMarkesByObject();
    }
    else
        clearMarkers();
}

void SubHelperMark::correctMarkers(QTransform tr)
{
    if (!midMark || midMark->metric().size() == 0)
        return;

    QRectF boundRect(QPointF(), q_ptr->getCamera()->screenSize());
    midMark->setMetric(Metric(tr.map(midMark->metric().toMercator(0).first()), Mercator));
    posMark->setMetric(Metric(markerBound(tr.map(posMark->metric().toMercator(0).first()), boundRect), Mercator));
    rotateMark->setMetric(Metric(markerBound(tr.map(rotateMark->metric().toMercator(0).first()), boundRect), Mercator));
    scaleMark->setMetric(Metric(markerBound(tr.map(scaleMark->metric().toMercator(0).first()), boundRect), Mercator));
}

void SubHelperMark::upMarkers(HeOptions)
{
    posMark->layer()->remObject(posMark);
    midMark->layer()->remObject(midMark);
    rotateMark->layer()->remObject(rotateMark);
    scaleMark->layer()->remObject(scaleMark);
    q_ptr->getSystemLayer()->addObject(posMark);
    q_ptr->getSystemLayer()->addObject(midMark);
    q_ptr->getSystemLayer()->addObject(rotateMark);
    q_ptr->getSystemLayer()->addObject(scaleMark);
}

void SubHelperMark::updateMarkRect(QRectF &updateRect)
{
    QPolygonF tmp;
    tmp << midMark->metric().toMercator(0).first()
        << posMark->metric().toMercator(0).first()
        << rotateMark->metric().toMercator(0).first()
        << scaleMark->metric().toMercator(0).first();
    updateRect = updateRect.isEmpty() ? tmp.boundingRect() : updateRect.united(tmp.boundingRect());
}

void SubHelperMark::changeMarkers(QPointF pos, bool flag)
{
    QTransform trWorld;  // локальная матрица

    MapCamera *cam = q_ptr->getCamera();

    QRectF boundRect(QPointF(), cam->screenSize());
    QPointF prevPos = markerBound(q_ptr->getLastScreenPos(), boundRect);
    pos = markerBound(pos, boundRect);

    QRectF updateRect;
    static const int delta = 2;

    if (isMarkMode == 1) {
        updateMarkRect(updateRect);
        moveMarkers(pos - prevPos);
        updateMarkRect(updateRect);
    }
    else if (isMarkMode == 2) {
        QPointF deltaWorld = cam->toWorld(pos) - cam->toWorld(prevPos);
        trWorld.translate(deltaWorld.x(), deltaWorld.y());

        updateMarkRect(updateRect);
        moveMarkers(pos - prevPos);
        updateMarkRect(updateRect);
    }
    else if (isMarkMode == 3) {
        QPointF mid = midMark->metric().toMercator(0).first();
        qreal ang = radToDeg(polarAngle(pos - mid) - polarAngle(prevPos - mid));
        QPointF midWorld = cam->toWorld(mid);
        trWorld.translate(midWorld.x(), midWorld.y()).rotate(ang).translate(-midWorld.x(), -midWorld.y());

        updateRect = QRectF(rotateMark->metric().toMercator(0).first(), QSizeF(delta, delta));
        rotateMark->setMetric(Metric(markerBound(rotateMark->metric().toMercator(0).first() + pos - prevPos, boundRect), Mercator));
        updateRect = updateRect.united(QRectF(rotateMark->metric().toMercator(0).first(), QSizeF(delta, delta)));
    }
    else if (isMarkMode == 4) {
        QPointF mid = midMark->metric().toMercator(0).first();
        qreal dist = lengthR2Squared(prevPos - mid);
        if (!qFuzzyIsNull(dist)) {
            qreal scale = lengthR2Squared(pos - mid) / dist;
            QPointF midWorld = cam->toWorld(mid);
            trWorld.translate(midWorld.x(), midWorld.y()).scale(scale, scale).translate(-midWorld.x(), -midWorld.y());

            updateRect = QRectF(scaleMark->metric().toMercator(0).first(), QSizeF(delta, delta));
            scaleMark->setMetric(Metric(markerBound(scaleMark->metric().toMercator(0).first() + pos - prevPos, boundRect), Mercator));
            updateRect = updateRect.united(QRectF(scaleMark->metric().toMercator(0).first(), QSizeF(delta, delta)));
        }
    }
    else
        return;

    static const int dtMark = 30;
    updateRect = updateRect.adjusted(-dtMark, -dtMark, dtMark, dtMark);

    emit changeObjects(trWorld, updateRect, flag);
}

void SubHelperMark::moveMarkers(QPointF delta)
{
    QRectF boundRect(QPointF(), q_ptr->getCamera()->screenSize());

    midMark->setMetric(Metric(midMark->metric().toMercator(0).first() + delta, Mercator));
    posMark->setMetric(Metric(markerBound(posMark->metric().toMercator(0).first() + delta, boundRect), Mercator));
    rotateMark->setMetric(Metric(markerBound(rotateMark->metric().toMercator(0).first() + delta, boundRect), Mercator));
    scaleMark->setMetric(Metric(markerBound(scaleMark->metric().toMercator(0).first() + delta , boundRect), Mercator));
}

void SubHelperMark::initMarkesByObject()
{
    QPointF mid;
    int count = 0;

    QList<MapObject *> list = q_ptr->markers();
    for (QList<MapObject *>::iterator it = list.begin(); it != list.end(); ++it) {
        MapObject *obj = *it;
        if (!obj || obj->metric().size() == 0)
            continue;

        MetricItem mData = obj->metric().toMercator(0);
        for (int j = 0; j < mData.size(); ++j) {
            mid += mData.at(j);
            ++count;
        }
    }

    if (count != 0)
        initMarkersByPoint(mid / count);
    else
        clearMarkers();
}

void SubHelperMark::initMarkersByPoint(QPointF mid)
{
    qreal tmp = 150;
    qreal dcos = qCos(M_PI / 6.);
    qreal dsin = qSin(M_PI / 6.);
    qreal dx = tmp * dcos;
    qreal dy = tmp * dsin;

    QRectF boundRect(QPointF(), q_ptr->getCamera()->screenSize());

    midAni->setEndOpacity(255);
    posAni->setEndOpacity(255);
    rotateAni->setEndOpacity(255);
    scaleAni->setEndOpacity(255);

    QPointF boundmid = markerBound(mid, boundRect);
    if (midMark->metric().size() == 0)
        midMark->setMetric(Metric(boundmid, Mercator));
    if (posMark->metric().size() == 0)
        posMark->setMetric(Metric(boundmid, Mercator));
    if (rotateMark->metric().size() == 0)
        rotateMark->setMetric(Metric(boundmid, Mercator));
    if (scaleMark->metric().size() == 0)
        scaleMark->setMetric(Metric(boundmid, Mercator));

    midAni->setEndPosition(mid);
    posAni->setEndPosition(markerBound(mid + QPointF(0, tmp), boundRect));
    rotateAni->setEndPosition(markerBound(mid + QPointF(-dx, -dy), boundRect));
    scaleAni->setEndPosition(markerBound(mid + QPointF(dx, -dy), boundRect));

    markersGroup.start();
}

// --------------------------------------------

SubHelperEdit::SubHelperEdit(MapSubMultiHelper *subhelper, QObject *parent)
    : SubHelper(subhelper, parent),
      changinIndex(-1), metricNumber(0),
      currentObject(NULL), visualObject(NULL)
{
    markPoint = false;
    activePoint = new MapObject;

    activePoint->setSemantic(PointSemantic, QPointF(10, 10));
    activePoint->setSemantic(PenSemantic, QPen(MarkerActivePen, 1, Qt::SolidLine));
    activePoint->setSemantic(BrushSemantic, QBrush(MarkerActiveBrush, Qt::SolidPattern));
    activePoint->setSemantic(MarkersSemantic, (int)hoPoint);
    activePoint->setLayer(q_ptr->getMarkerLayer());

    deletePoints = new MapObject;

    deletePoints->setSemantic(PointSemantic, QPointF(10, 10));
    deletePoints->setSemantic(PenSemantic, QPen(MarkerDeletePen, 1, Qt::SolidLine));
    deletePoints->setSemantic(BrushSemantic, QBrush(MarkerDeleteBrush, Qt::SolidPattern));
    deletePoints->setSemantic(MarkersSemantic, (int)hoPoint);
    deletePoints->setLayer(q_ptr->getMarkerLayer());

    connect(q_ptr, SIGNAL(dataInited(HeOptions)), this, SLOT(initData(HeOptions)));
    connect(q_ptr, SIGNAL(objectsCountChanged()), this, SLOT(upDeleteMarkers()));
    connect(q_ptr, SIGNAL(correctSubWorld(QTransform)), this, SLOT(correct(QTransform)));
    connect(q_ptr, SIGNAL(correctSubWorld2(QTransform)), this, SLOT(correct2(QTransform)));
    connect(this, SIGNAL(objectHasChanged(MapObject*)), q_ptr, SLOT(objectHasChanged(MapObject*)));
}

SubHelperEdit::~SubHelperEdit()
{
    delete deletePoints;
    delete activePoint;
}

bool SubHelperEdit::start(QPointF pos)
{
    QList<MapObject*> list = q_ptr->markers();
    for (QList<MapObject*>::iterator it = list.begin(); it != list.end(); ++it) {
        MapObject *mark = *it;
        MetricData mData = mark->metric().toMercator();
        for (int i = mData.size() - 1; i >= 0; --i) { // выбираем текущий объект + запоминаем его индекс
            if (nearControlPoint(mData.at(i), pos, changinIndex)) {
                metricNumber = i;
                currentObject = q_ptr->object(mark);

                visualObject = mark;
                if (!q_ptr->flag("isCachedMode", false).toBool()) {
                    MHECanChangePoint mhCcp(2, currentObject);
                    emit q_ptr->handler(&mhCcp);
                    q_ptr->setFlag("isChangePointMode", mhCcp.result());
                }

                if (changeControlPoint(pos))
                    return true;
                else {
                    visualObject = NULL;
                    return false;
                }
            }
        }
    }

    // тянем объект за промежуточную точку
    for (QList<MapObject*>::iterator it = list.begin(); it != list.end(); ++it) {
        MapObject *mark = *it;
        MapObject *obj  = q_ptr->object(mark);

        bool isClosed = obj->drawer() ? obj->drawer()->isClosed(obj->classCode()) : false;
        bool cubic = obj->attributeDraw(attrSmoothed).toBool();

        MetricData mData = mark->metric().toMercator();
        for (int i = 0; i < mData.size(); ++i) { // выбираем текущий объект + запоминаем его индекс
            if (cubic ?
                    nearEdgePointCubic(mData.at(i), pos, changinIndex, isClosed) :
                    nearEdgePoint(mData.at(i), pos, changinIndex, isClosed)
                    )
            {
                metricNumber = i;
                currentObject = obj;

                visualObject = mark;
                if (changeInterPoint(pos))
                    return true;
                else {
                    visualObject = NULL;
                    return false;
                }
            }
        }
    }

    return false;
}

bool SubHelperEdit::finish(QPointF pos)
{
    Q_UNUSED(pos);

    MHEPointDrop drop(currentObject, metricNumber, changinIndex);
    emit q_ptr->handler(&drop);

    MapCamera *cam = q_ptr->getCamera();

    if (changinIndex == -1 ||
            metricNumber >= visualObject->metric().size() ||
            changinIndex >= visualObject->metric().toMercator(metricNumber).size()) {
       deletePoints->setMetric(Metric(Mercator));
       activePoint->setMetric(Metric(Mercator));
       currentObject = NULL;
       visualObject = NULL;
       return false;
    }

    QPolygonF tmp = visualObject->metric().toMercator(metricNumber);

    bool canRemove = q_ptr->flag("isRemovePointMode").toBool();
    if (!q_ptr->flag("isCachedMode").toBool()) {
        MHECanRemovePoint mhCrp(false, currentObject, metricNumber);
        emit q_ptr->handler(&mhCrp);
        q_ptr->setFlag("isRemovePointMode", mhCrp.result());
    }
    canRemove = currentObject->drawer()->canRemovePoint(currentObject, metricNumber) & q_ptr->flag("isRemovePointMode").toBool();

    if (canRemove) {
        QPointF del;
        bool cubic = currentObject ? currentObject->attributeDraw(attrSmoothed).toBool() : false;
        bool closed = currentObject ? (currentObject->drawer() ? currentObject->drawer()->isClosed(currentObject->classCode()) : false) : false;
        if (cubic ? pointCanBeRemovedCubic(tmp, changinIndex, closed) : pointCanBeRemovedLinear(tmp, changinIndex, closed)) {
            del = tmp.at(changinIndex);
            tmp.remove(changinIndex);
            if (markPoint)
                activePoint->setMetric(Metric(Mercator));

            MHEPointRemoved mherp(currentObject, cam->toWorld(del), metricNumber, changinIndex);
            q_ptr->handler(&mherp);
        }
        else {
            if (changinIndex + 1 < tmp.size())
                if (cubic ? pointCanBeRemovedCubic(tmp, changinIndex + 1, closed) : pointCanBeRemovedLinear(tmp, changinIndex + 1, closed)) {
                    del = tmp.at(changinIndex);
                    tmp.remove(changinIndex + 1);

                    MHEPointRemoved mherp(currentObject, cam->toWorld(del), metricNumber, changinIndex + 1);
                    q_ptr->handler(&mherp);
                }
            if (changinIndex - 1 > 0) {
                if (cubic ? pointCanBeRemovedCubic(tmp, changinIndex - 1, closed)
                        : pointCanBeRemovedLinear(tmp, changinIndex - 1, closed)) {
                    del = tmp.at(changinIndex);
                    tmp.remove(changinIndex - 1);

                    MHEPointRemoved mherp(currentObject, cam->toWorld(del), metricNumber, changinIndex + 1);
                    q_ptr->handler(&mherp);
                }
            }
        }
    }

    QRectF updateRect = currentObject->drawer()->localBound(currentObject, q_ptr->getCamera());
    changeHelperMetric(tmp);
    changeCurrentObject();
    updateRect = updateRect.united(currentObject->drawer()->localBound(currentObject, q_ptr->getCamera()));

    emit objectHasChanged(currentObject);

    reject(false);

//    q_ptr->getMap()->updateScene(updateRect.toRect()
//                                 .adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));
    q_ptr->getMap()->updateScene();
    return true;
}

bool SubHelperEdit::proccessing(QPointF pos)
{
    if (changinIndex == -1)
        return false;

    if (metricNumber >= visualObject->metric().size() ||
            changinIndex >= visualObject->metric().toMercator(metricNumber).size()) {
        deletePoints->setMetric(Metric(Mercator));
        activePoint->setMetric(Metric(Mercator));
        return false;
    }

    QPolygonF tmp = visualObject->metric().toMercator(metricNumber);
    QPointF prev = tmp.at(changinIndex);
    if (qFuzzyCompare(prev.x(), pos.x()) && qFuzzyCompare(prev.y(), pos.y()))
        return false;

    tmp.replace(changinIndex, pos);
    if (markPoint)
        activePoint->setMetric(Metric(MetricItem() << pos, Mercator));

    MHEChangedPoint change(currentObject, q_ptr->getCamera()->toWorld(prev),
                           q_ptr->getCamera()->toWorld(pos), metricNumber, changinIndex);
    emit q_ptr->handler(&change);

    bool canRemove = q_ptr->flag("isRemovePointMode").toBool();
    if (!q_ptr->flag("isCachedMode").toBool()) {
        MHECanRemovePoint mhCrp(false, currentObject, metricNumber);
        emit q_ptr->handler(&mhCrp);
        q_ptr->setFlag("isRemovePointMode", mhCrp.result());
    }
    canRemove = currentObject->drawer()->canRemovePoint(currentObject, metricNumber) & q_ptr->flag("isRemovePointMode").toBool();

    QPolygonF dp;
    if (canRemove) {
        bool cubic = currentObject ? currentObject->attributeDraw(attrSmoothed).toBool() : false;
        bool closed = currentObject ? (currentObject->drawer() ? currentObject->drawer()->isClosed(currentObject->classCode()) : false) : false;
        if (cubic ? pointCanBeRemovedCubic(tmp, changinIndex, closed) : pointCanBeRemovedLinear(tmp, changinIndex, closed))
            dp.append(tmp.at(changinIndex));
        else {
            if (changinIndex < tmp.size() - 1) {
                if (cubic ? pointCanBeRemovedCubic(tmp, changinIndex + 1, closed) : pointCanBeRemovedLinear(tmp, changinIndex + 1, closed))
                    dp.append(tmp.at(changinIndex + 1));
            }
            if (changinIndex > 1) {
                if (cubic ? pointCanBeRemovedCubic(tmp, changinIndex - 1, closed) : pointCanBeRemovedLinear(tmp, changinIndex - 1, closed))
                    dp.append(tmp.at(changinIndex - 1));
            }
        }
    }
    deletePoints->setMetric(Metric(dp, Mercator));

//    QRectF updateRect = currentObject->drawer()->localBound(currentObject, q_ptr->getCamera());
    changeHelperMetric(tmp);
    changeCurrentObject();
//    updateRect = updateRect.united(currentObject->drawer()->localBound(currentObject, q_ptr->getCamera()));

//    q_ptr->getMap()->updateScene(updateRect.toRect()
//                                 .adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));
    q_ptr->getMap()->updateScene();
    return true;
}

void SubHelperEdit::reject(bool flag)
{
    deletePoints->setMetric(Metric(Mercator));
    currentObject = NULL;
    visualObject = NULL;
    metricNumber = 0;
    changinIndex = -1;
    if (flag)
        activePoint->setMetric(Metric(Mercator));
}

void SubHelperEdit::initCustomObject(MapObject *mo, int mNumber, int pNumber)
{
    currentObject = mo;
    visualObject = q_ptr->marker(currentObject);
    metricNumber = mNumber;
    changinIndex = pNumber;

    MHEPointDrag drag(currentObject, metricNumber, changinIndex);
    emit q_ptr->handler(&drag);
}

void SubHelperEdit::initData(HeOptions opt)
{
    activePoint->setSemantic(MarkersSemantic, int(opt.testFlag(hoPoint) ? hoPoint : hoNone));
    deletePoints->setSemantic(MarkersSemantic, int(opt.testFlag(hoPoint) ? hoPoint : hoNone));
    markPoint = opt.testFlag(hoMarkPoint);
}

void SubHelperEdit::upDeleteMarkers()
{
    activePoint->layer()->remObject(activePoint);
    q_ptr->getMarkerLayer()->addObject(activePoint);

    deletePoints->layer()->remObject(deletePoints);
    q_ptr->getMarkerLayer()->addObject(deletePoints);
}

void SubHelperEdit::correct(QTransform tr)
{
    if (markPoint) {
        MetricItem it = activePoint->metric().toMercatorE(0);
        activePoint->setMetric(Metric(tr.map(it), Mercator));
    }
}

void SubHelperEdit::correct2(QTransform tr)
{
    if (markPoint) {
        MapCamera *cam = q_ptr->getCamera();
        MetricItem it = cam->toWorld().map(activePoint->metric().toMercatorE(0));
        activePoint->setMetric(Metric(cam->toScreen().map(tr.map(it)), Mercator));
    }
}

bool SubHelperEdit::changeControlPoint(const QPointF &pos)
{
    if (q_ptr->flag("isChangePointMode").toInt() == 0)
        return false;

    QPolygonF tmp = visualObject->metric().toMercator(metricNumber);
    QPointF prev = tmp.at(changinIndex);

    MHEChangedPoint change(currentObject, q_ptr->getCamera()->toWorld(prev),
                           q_ptr->getCamera()->toWorld(pos), metricNumber, changinIndex);
    emit q_ptr->handler(&change);

    tmp.replace(changinIndex, pos);

//    QRectF updateRect = currentObject->drawer()->localBound(currentObject, q_ptr->getCamera());
    changeHelperMetric(tmp);
    changeCurrentObject();
//    updateRect = updateRect.united(currentObject->drawer()->localBound(currentObject, q_ptr->getCamera()));

    MHEPointDrag drag(currentObject, metricNumber, changinIndex);
    emit q_ptr->handler(&drag);

    if (markPoint)
        activePoint->setMetric(Metric(MetricItem() << pos, Mercator));

//    q_ptr->getMap()->updateScene(updateRect.toRect()
//                                 .adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));
    q_ptr->getMap()->updateScene();

    return true;
}

bool SubHelperEdit::changeInterPoint(const QPointF &pos)
{
    if (q_ptr->flag("isChangePointMode").toInt() < 2)
        return false;

    bool can = true;
    can = currentObject->drawer()->canAddControlPoints(currentObject);

    if (!can)
        return false;

    QPolygonF tmp = visualObject->metric().toMercator(metricNumber);
    tmp.insert(changinIndex, pos);

    MHEPointInsert ins(currentObject, pos, metricNumber, changinIndex);
    emit q_ptr->handler(&ins);

    deletePoints->setMetric(Metric(QPolygonF() << pos, Mercator));

//    QRectF updateRect = currentObject->drawer()->localBound(currentObject, q_ptr->getCamera());
    changeHelperMetric(tmp);
    changeCurrentObject();
//    updateRect = updateRect.united(currentObject->drawer()->localBound(currentObject, q_ptr->getCamera()));

    MHEPointDrag drag(currentObject, metricNumber, changinIndex);
    emit q_ptr->handler(&drag);

    if (markPoint)
        activePoint->setMetric(Metric(MetricItem() << pos, Mercator));
//    q_ptr->getMap()->updateScene(updateRect.toRect()
//                                 .adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));
    q_ptr->getMap()->updateScene();

    return true;
}

void SubHelperEdit::changeHelperMetric(const QPolygonF &tmp)
{
    visualObject->setMetric(
                Metric(
                    currentObject->drawer()->correctMetric(visualObject, tmp, metricNumber),
                    Mercator
                    )
                );
}

void SubHelperEdit::changeCurrentObject()
{
    currentObject->blockSignals(true);
    currentObject->setMetric(Metric(Mercator));
    MetricData mData = visualObject->metric().toMercator();
    for (int i = 0; i < mData.size(); ++i)
        currentObject->setMetric(i, Metric(q_ptr->getCamera()->toWorld().map(mData.at(i)), Mercator));
    currentObject->blockSignals(false);
}

// --------------------------------------------

SubHelperAdd::SubHelperAdd(MapSubMultiHelper *subhelper, QObject *parent)
    : SubHelper(subhelper, parent),
    currentObject(NULL), visualObject(NULL),
    maxPoints(0), noCutMax(true), addType(0)
{
    visualTmp = new MapObject;

    visualTmp->setSemantic(MarkersSemantic,     (int)hoLine);
    visualTmp->setSemantic(PointSemantic,       QPointF(10, 10));
    visualTmp->setSemantic(PenSemantic,         QPen(PointMarkPenColor, 2, Qt::SolidLine));
    visualTmp->setSemantic(BrushSemantic,       QBrush(PointMarkBrushColor, Qt::SolidPattern));
    visualTmp->setSemantic(PenSemanticLine,     QPen(LineMarkPenColor, 7, Qt::SolidLine));
    visualTmp->setSemantic(PenSemanticSquare,   QPen(PolyMarkPenColor, 1, Qt::SolidLine));
    visualTmp->setSemantic(BrushSemanticSquare, QBrush(PolyMarkBrushColor, Qt::SolidPattern));
    visualTmp->setLayer(q_ptr->getSystemLayer());

    connect(this, SIGNAL(autoReject()), q_ptr, SIGNAL(autoReject()));
    connect(this, SIGNAL(objectHasChanged(MapObject*)), q_ptr, SLOT(objectHasChanged(MapObject*)));
}

SubHelperAdd::~SubHelperAdd()
{    
    delete visualTmp;
}

bool SubHelperAdd::start(QPointF pos)
{
    if (maxPoints && pointsCount >= maxPoints)
        return false;

    if (q_ptr->flag("isCacheMode").toBool()) {
        MHECanAddPoint mhCap(true, true, currentObject);
        emit q_ptr->handler(&mhCap);

        q_ptr->setFlag("isAddPointMode", mhCap.result());
        q_ptr->setFlag("addType",  mhCap.flag);
    }

    if (currentObject) {
        addType = (noCutMax & moveObject) ||
                (currentObject->drawer()->canAddControlPoints(currentObject) & q_ptr->flag("isAddPointMode").toBool())
                ? (currentObject->drawer()->canAddPen(currentObject) & q_ptr->flag("addType").toInt() ? 2 : 1)
                : 0;
    }
    else
        addType = q_ptr->flag("isAddPointMode").toBool() ? (q_ptr->flag("addType").toInt() ? 2 : 1) : 0;

    if (addType) {
        visualTmp->setMetric(Metric(pos, Mercator));
//        q_ptr->getMap()->updateScene(QRect(pos.toPoint(), QSize())
//                                     .adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));
        q_ptr->getMap()->updateScene();
    }

    return addType;
}

bool SubHelperAdd::finish(QPointF pos)
{
    Q_UNUSED(pos);
    if (!addType)
        return false;

    QPolygonF tmp = visualTmp->metric().toMercator(0);
    if (addType == 1) {
        ++pointsCount;
    }
    else {
        tmp = polygonSmoothing(tmp, 10);
        if (!noCutMax && maxPoints && pointsCount + tmp.size() > maxPoints)
            tmp.resize(maxPoints - pointsCount);

        if (moveObject)
            pointsCount = tmp.size();
        else
            pointsCount += tmp.size();
    }

    if (currentObject) {
        QList<QPointF> poly = tmp.toList();
        if (moveObject) {
            MapObject tmpObj;
            tmpObj.setClassCode(currentObject->classCode());
            while (!poly.isEmpty() && currentObject->drawer()->canAddControlPoints(&tmpObj)) {
                int num = currentObject->drawer()->whereAddPoint(&tmpObj);
                QPolygonF tmp = tmpObj.metric().size() <= num ? QPolygonF() : tmpObj.metric().toMercator(num);
                tmpObj.setMetric(num, (Metric(tmp << q_ptr->getCamera()->toWorld(poly.takeFirst()), Mercator)));
            }
            currentObject->setMetric(tmpObj.metric());
            visualObject->setMetric(Metric(currentObject->drawer()->controlPoints(currentObject, q_ptr->getCamera()), Mercator));
        }
        else {
            while (!poly.isEmpty() && currentObject->drawer()->canAddControlPoints(currentObject)) {
                int num = currentObject->drawer()->whereAddPoint(visualObject);
                QPointF p = poly.takeFirst();

                currentObject->blockSignals(true);
                QPolygonF tmp = currentObject->metric().size() <= num ?
                            QPolygonF() : currentObject->metric().toMercator(num);
                currentObject->setMetric(num, (Metric(tmp << q_ptr->getCamera()->toWorld(p), Mercator)));
                currentObject->blockSignals(false);

                visualObject->setMetric(Metric(currentObject->drawer()->controlPoints(currentObject, q_ptr->getCamera()), Mercator));
            }
        }
    }

//    QRect updateRect = visualTmp->metric().toMercator(0).boundingRect().toRect();
    visualTmp->setMetric(Metric(Mercator));
//    q_ptr->getMap()->updateScene(updateRect.adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));

    q_ptr->getMap()->updateScene();

    QPolygonF points;
    QTransform const &trans = q_ptr->getCamera()->toWorld();
    std::for_each(tmp.begin(), tmp.end(), TransformPoint(points, trans));

    MHEClickedPoints click(points);
    emit q_ptr->handler(&click);

    allClickedPoints += points;

    if (currentObject)
        emit objectHasChanged(currentObject);

    if (maxPoints && pointsCount >= maxPoints)
        emit autoReject();

    return true;
}

bool SubHelperAdd::proccessing(QPointF pos)
{
    if (!addType)
        return false;

    QPolygonF tmp = visualTmp->metric().toMercator(0);
    if (addType == 1)
        tmp.replace(tmp.size() - 1, pos);
    else if (lengthR2Squared(tmp.last() - pos) > MoveDelta)
        tmp.append(pos);

    visualTmp->setMetric(Metric(tmp, Mercator));

//    q_ptr->getMap()->updateScene(tmp.boundingRect().toRect()
//                                 .adjusted(-updateDelta, -updateDelta, updateDelta, updateDelta));
    q_ptr->getMap()->updateScene();
    return true;
}

void SubHelperAdd::reject(bool flag)
{
    if (currentObject && currentObject->drawer()) {
        currentObject->setMetric(Metric(currentObject->drawer()->fillMetric(currentObject, q_ptr->getCamera()), Mercator));
        q_ptr->getMap()->updateScene();
    }

    if (flag) {
        MHEAllClickedPoints click(allClickedPoints);
        emit q_ptr->handler(&click);
    }

    visualTmp->setMetric(Metric(Mercator));
    currentObject = NULL;
    visualObject  = NULL;

    allClickedPoints.clear();
    pointsCount = 0;
}

void SubHelperAdd::initData(MapObject *mo, int maxPoints_, HeOptions opt)
{
    currentObject = mo;
    visualObject  = q_ptr->marker(mo);
    maxPoints     = maxPoints_;
    noCutMax      = opt.testFlag(hoNoCutMax);
    moveObject    = opt.testFlag(hoMoveObject);

    pointsCount = 0;
}

// --------------------------------------------

SubHelperCustom::SubHelperCustom(MapSubMultiHelper *subhelper, QObject *parent)
    : SubHelper(subhelper, parent), customAction(NULL)
{

}

SubHelperCustom::~SubHelperCustom()
{

}

bool SubHelperCustom::start(QPointF pos)
{
    QPointF worldPos = q_ptr->getCamera()->toWorld(pos);

    MHECustomAction custom(worldPos);
    emit q_ptr->handler(&custom);

    HelperAction *action = custom.action();
    if (!action)
        return false;

    subtype = action->type();
    switch (subtype) {
    case HelperActionMove::Type: {
        MapSubHelperMove *move = new MapSubHelperMove(q_ptr->getMap());
        connect(move, SIGNAL(handler(minigis::MHEvent*)), q_ptr, SIGNAL(handler(minigis::MHEvent*)));
        connect(move, SIGNAL(worldChanged(QTransform)), q_ptr, SIGNAL(worldChanged(QTransform)));
        connect(move, SIGNAL(worldChanged(QTransform)), q_ptr, SLOT(correctWorld(QTransform)));
        customAction = move;
        customAction->start(pos);

        break;
    }
    case HelperActionSelect::Type: {
        MapSubHelperSelect *select = new MapSubHelperSelect(q_ptr->getMap(), true);
        select->appendCustomObjects(q_ptr->objects());
        connect(select, SIGNAL(handler(minigis::MHEvent*)), q_ptr, SIGNAL(handler(minigis::MHEvent*)));
        connect(select, SIGNAL(worldChanged(QTransform)), q_ptr, SIGNAL(worldChanged(QTransform)));
        connect(select, SIGNAL(worldChanged(QTransform)), q_ptr, SLOT(correctWorld(QTransform)));
        customAction = select;
        customAction->start(pos);

        break;
    }
    case HelperActionCreate::Type: {
        HelperActionCreate *hac = dynamic_cast<HelperActionCreate*>(action);
        if (!hac || !hac->object())
            return false;

        q_ptr->addObjects(hac->object());
        SubHelperAdd *create = new SubHelperAdd(q_ptr);
        create->initData(hac->object());
        customAction = create;
        customAction->start(pos);

        break;
    }
    case HelperActionEdit::Type: {
        HelperActionEdit *hae = dynamic_cast<HelperActionEdit *>(action);
        if (!hae || !hae->object())
            return false;

        q_ptr->addObjects(hae->object());
        SubHelperEdit *edit = new SubHelperEdit(q_ptr);
        edit->initCustomObject(hae->object(), hae->metricNumber(), hae->pointNumber());
        customAction = edit;

        break;
    }
    default:
        break;
    }

    return bool(customAction);
}

bool SubHelperCustom::finish(QPointF pos)
{
    customAction->finish(pos);
    if (subtype == 2) {
        MapSubHelperSelect *select = qobject_cast<MapSubHelperSelect*>(customAction);
        QList<MapObjectPtr> tmpList = select->getSelectedObjects();
        q_ptr->reject(false);
        q_ptr->addObjects(tmpList);
    }
    else
        customAction->reject(false);

    subtype = 0;
    delete customAction;
    customAction = NULL;

    return true;
}

bool SubHelperCustom::proccessing(QPointF pos)
{
    return customAction->proccessing(pos);
}

void SubHelperCustom::reject(bool flag)
{
    if (customAction)
        customAction->reject(flag);
}

// --------------------------------------

} // namespace minigis

// --------------------------------------
