
#include <QPinchGesture>
#include <QHash>

//#include <tms/common/log.h>
#ifdef ANDROID
//#include <tms/android/Vibrator.h>
#endif

#include "frame/mapsettings.h"
#include "core/mapmath.h"
#include "interact/maphelper_p.h"
#include "drawer/mapdrawer.h"
#include "drawer/mapdrawercommon.h"
#include "coord/mapcoords.h"
#include "interact/maphelperevent.h"
#include "interact/maphelpersubhelpers.h"

#include "interact/maphelper.h"

//----------------------------------------------------------
#define D_MAP if (d->map == NULL) return false;
//----------------------------------------------------------

namespace minigis {

//----------------------------------------------------------

MapHelper::MapHelper(QObject *parent)
    :QObject(parent), d_ptr(new MapHelperPrivate(this))
{
    d_ptr->q_ptr = this;
}

MapHelper::MapHelper(MapHelperPrivate &dd, QObject *parent)
    :QObject(parent), d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}

MapHelper::~MapHelper()
{
    if (d_ptr->started)
        finish();
}

void MapHelper::init(minigis::MapFrame *map)
{
    d_ptr->init(map);
}

bool MapHelper::start()
{
    d_ptr->started = true;
    return true;
}

bool MapHelper::isEnabled() const
{
    return d_ptr->enabled;
}

int MapHelper::type() const
{
    return 0;
}

bool MapHelper::isStill() const
{
    return true;
}

bool MapHelper::isStarted() const
{
    return d_ptr->started;
}

bool MapHelper::clear()
{
    if (isStill()) {
        if (isStarted())
            finish();
        return true;
    }
    else
        return !isStarted();
    return false;
}

QObject *MapHelper::owner() const
{
    return d_ptr->recv;
}

QString MapHelper::humanName() const
{
    return QString::fromUtf8("Нет режима");
}

void MapHelper::setEnabled(bool value)
{
    if (d_ptr->enabled == value)
        return;
    d_ptr->enabled = value;
    emit enabledChanged(value);
}

bool MapHelper::keyPress(QKeyEvent */*event*/)
{
    return false;
}

bool MapHelper::keyRelease(QKeyEvent */*event*/)
{
    return false;
}

bool MapHelper::mouseDown(QMouseEvent */*event*/)
{
    return false;
}

bool MapHelper::mouseUp(QMouseEvent */*event*/)
{
    return false;
}

bool MapHelper::mouseMove(QMouseEvent */*event*/)
{
    return false;
}

bool MapHelper::wheelEvent(QWheelEvent * /*event*/)
{
    return false;
}

bool MapHelper::gesture(QGestureEvent */*event*/)
{
    return false;
}

bool MapHelper::touch(QTouchEvent */*event*/)
{
    return false;
}

void MapHelper::resize(QSizeF, QSizeF)
{
}

void MapHelper::finish()
{
    d_ptr->started = false;
}

//----------------------------------------------------------

QTouchEvent::TouchPoint makeTouchPointFromMouseEvent(QMouseEvent *event, Qt::TouchPointState state)
{
    QTouchEvent::TouchPoint newPoint;

#if defined ( QT_4 )
    newPoint.setPos(event->posF());
    newPoint.setScenePos(event->globalPos());
    newPoint.setScreenPos(event->globalPos());
#elif defined ( QT_5 )
    newPoint.setPos(event->localPos());
    newPoint.setScenePos(event->windowPos());
    newPoint.setScreenPos(event->screenPos());
#endif

    newPoint.setState(state);
    newPoint.setId(0);
    return newPoint;
}

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

MapHelperUniversal::MapHelperUniversal(QObject *parent)
    : MapHelper(*new MapHelperUniversalPrivate, parent)
{
    Q_D(MapHelperUniversal);
    connect(d, SIGNAL(stateChanged(States)), this, SIGNAL(stateChanged(States)));
}

MapHelperUniversal::MapHelperUniversal(MapHelperUniversalPrivate &dd, QObject *parent)
    :MapHelper(dd, parent)
{
    Q_D(MapHelperUniversal);
    connect(d, SIGNAL(stateChanged(States)), this, SIGNAL(stateChanged(States)));
}

MapHelperUniversal::~MapHelperUniversal()
{

}

QString MapHelperUniversal::humanName() const
{
    return QString::fromUtf8("Универсальный режим");
}

int MapHelperUniversal::type() const
{
    Q_D(const MapHelperUniversal);
    return d->states;
}

bool MapHelperUniversal::isStill() const
{
    Q_D(const MapHelperUniversal);
    return !(d->states & (stSelect | stCreate | stEdit));
}

bool MapHelperUniversal::start()
{
    Q_D(MapHelperUniversal);
    if (d->recv && d->recv != d->_recv)
        if (!isStill())
            return false;

    if (d->recv)
        finish();

    d->started = false;
    D_MAP;
    if (!d->_recv || !d->_abortSignal)
        return false;
    d->recv = d->_recv;
    bool connFlag = true;

    if (d->_abortSignal)
        connFlag = connFlag && connect(d->recv, d->_abortSignal, this, SLOT(finish()));
    if (d->_handlerSlot)
        connFlag = connFlag && connect(d, SIGNAL(handler(minigis::MHEvent*)), d->recv, d->_handlerSlot);

    if (connFlag) {
        connect(d->recv, SIGNAL(destroyed()), SLOT(finish()));

        d->started = true;
    }
    else
        finish();

    return connFlag;
}

void MapHelperUniversal::setRunData(QObject *recv, const char *abortSignal, const char *handlerSlot)
{
    Q_D(MapHelperUniversal);

    d->_recv        = recv;
    d->_abortSignal = abortSignal;
    d->_handlerSlot = handlerSlot;
}

States MapHelperUniversal::state() const
{
    Q_D(const MapHelperUniversal);
    return d->states;
}

QObject *MapHelperUniversal::receiver() const
{
    Q_D(const MapHelperUniversal);
    return d->recv.data();
}

bool MapHelperUniversal::hasMode(HelperState state) const
{
    Q_D(const MapHelperUniversal);
    return d->states.testFlag(state);
}

void MapHelperUniversal::move(const QRectF &rgn)
{
    Q_D(MapHelperUniversal);
    if (d->states.testFlag(stMove))
        return;

    MapSubHelperMove *sub = new MapSubHelperMove(d->map);
    d->addSubhelper(sub);
    if (!rgn.isEmpty())
        sub->initData(rgn.normalized());
}

void MapHelperUniversal::rotate()
{
    Q_D(MapHelperUniversal);
    if (d->states.testFlag(stRotate))
        return;

    MapSubHelperRotate *sub = new MapSubHelperRotate(d->map);
    d->addSubhelper(sub);
}

void MapHelperUniversal::scale()
{
    Q_D(MapHelperUniversal);
    if (d->states.testFlag(stScale))
        return;

    MapSubHelperScale *sub = new MapSubHelperScale(d->map);
    d->addSubhelper(sub);
}

void MapHelperUniversal::select(MapObjectList *baseSelection)
{
    Q_D(MapHelperUniversal);
    if (d->states.testFlag(stSelect))
        return;

    reject(d->states & (stCreate | stEdit));

    MapSubHelperSelect *sub = new MapSubHelperSelect(d->map);
    d->addSubhelper(sub);
    sub->initData(baseSelection);
}

void MapHelperUniversal::edit(const MapObjectList &objects, HeOptions options)
{
    Q_D(MapHelperUniversal);
    if (d->states.testFlag(stEdit))
        return;

    reject(d->states & (stSelect | stCreate));

    MapSubHelperEdit *sub = new MapSubHelperEdit(d->map);
    d->addSubhelper(sub);
    sub->initData(objects, options);
}

void MapHelperUniversal::create(MapObject *mo, int maxPoints, HeOptions options)
{
    Q_D(MapHelperUniversal);
    if (d->states.testFlag(stCreate))
        return;

    reject(d->states & (stSelect | stEdit));

    MapSubHelperCreate *sub = new MapSubHelperCreate(d->map, options);
    d->addSubhelper(sub);
    sub->initData(mo, maxPoints, options);
}

void MapHelperUniversal::deselectObjects(MapObject *object)
{
    if (object)
        deselectObjects(QList<MapObject *>() << object);
}

void MapHelperUniversal::deselectObjects(const QList<MapObject *> objects)
{
    Q_D(MapHelperUniversal);
    MapSubHelper *sub = d->subhelpers.value(MapSubHelperSelect::Type);
    if (sub->type() == MapSubHelperSelect::Type) {
        MapSubHelperSelect* subsel = dynamic_cast<MapSubHelperSelect*>(sub);
        Q_ASSERT(subsel);
        subsel->deleteObjects(objects);
    }
}

void MapHelperUniversal::selectObjects(MapObject *object)
{
    if (object)
        selectObjects(QList<MapObject *>() << object);
}

void MapHelperUniversal::selectObjects(const QList<MapObject *> objects)
{
    Q_D(MapHelperUniversal);
    MapSubHelper *sub = d->subhelpers.value(MapSubHelperSelect::Type);
    if (sub->type() == MapSubHelperSelect::Type) {
        MapSubHelperSelect* subsel = dynamic_cast<MapSubHelperSelect*>(sub);
        Q_ASSERT(subsel);
        subsel->appendCustomObjects(objects);
    }
}

void MapHelperUniversal::addPoint(const QPointF &pt)
{
    Q_D(MapHelperUniversal);
    MapSubHelper *sub = d->subhelpers.value(MapSubHelperCreate::Type);
    if (sub->type() == MapSubHelperCreate::Type) {
        MapSubHelperCreate* subcr = dynamic_cast<MapSubHelperCreate*>(sub);
        Q_ASSERT(subcr);
        subcr->addPoint(pt);
    }
}

void MapHelperUniversal::addPoint(qreal x, qreal y)
{
    addPoint(QPointF(x, y));
}

bool MapHelperUniversal::keyPress(QKeyEvent *event)
{
    Q_UNUSED(event);
#if 0
    Q_D(MapHelperUniversal);
    switch (event->key()) {
    case Qt::Key_Q: {
        if (Qt::NoModifier == event->modifiers()) {
            move();
            qDebug() << "Move On";
        }
        else if (Qt::ShiftModifier == event->modifiers()) {
            moveStop();
            qDebug() << "Move Off";
        }
        break;
    }
    case Qt::Key_W: {
        if (Qt::NoModifier == event->modifiers()) {
            rotate();
            qDebug() << "Rotate On";
        }
        else if (Qt::ShiftModifier == event->modifiers()) {
            rotateStop();
            qDebug() << "Rotate Off";
        }
        break;
    }
    case Qt::Key_E: {
        if (Qt::NoModifier == event->modifiers()) {
            scale();
            qDebug() << "Scale On";
        }
        else if (Qt::ShiftModifier == event->modifiers()) {
            scaleStop();
            qDebug() << "Scalse Off";
        }
        break;
    }
    case Qt::Key_R: {
        if (Qt::NoModifier == event->modifiers()) {
            select();
            qDebug() << "Select On";
        }
        else if (Qt::ShiftModifier == event->modifiers()) {
            selectStop();
            qDebug() << "Select Off";
        }
        break;
    }
    case Qt::Key_T: {
        if (Qt::NoModifier == event->modifiers()) {
            edit();
            qDebug() << "Edit On";
        }
        else if (Qt::ShiftModifier == event->modifiers()) {
            editStop();
            qDebug() << "Edit Off";
        }
        break;
    }
    case Qt::Key_Y: {
        if (Qt::NoModifier == event->modifiers()) {
            create();
            qDebug() << "Create On";
        }
        else if (Qt::ShiftModifier == event->modifiers()) {
            createStop();
            qDebug() << "Create Off";
        }
        break;
    }
    case Qt::Key_A: {
        qDebug() << "Custom No On";
        d->customAction = 0;
        break;
    }
    case Qt::Key_S: {
        qDebug() << "Custom Move On";
        d->customAction = 1;
        break;
    }
    case Qt::Key_D: {
        qDebug() << "Custom Select On";
        d->customAction = 2;
        break;
    }
    case Qt::Key_F: {
        qDebug() << "Custom Edit On";
        d->customAction = 4;
        break;
    }
    case Qt::Key_G: {
        qDebug() << "Custom Create On";
        d->customAction = 3;
        break;
    }
    default:
        qDebug() << event;
        break;
    }

    QString types = "";
    foreach (MapSubHelper *sub, d->subhelpers)
        types += QString::number(sub->type()) + " ";
    qDebug() << types;
#endif

    return true;
}

bool MapHelperUniversal::mouseDown(QMouseEvent *event)
{
    Q_D(MapHelperUniversal);
    D_MAP;

    d->touch->touchPoints.clear();
    d->touch->touchPoints << makeTouchPointFromMouseEvent(event, Qt::TouchPointPressed);

    if (d->subhelpers.isEmpty())
        return false;
#warning    d->subhelpers.first(); - что имелось ввиду?
    d->currentHelper = d->subhelpers.begin().value();
    return d->currentHelper->start(event->pos());
}

bool MapHelperUniversal::mouseUp(QMouseEvent *event)
{
    Q_D(MapHelperUniversal);
    D_MAP;

    d->touch->touchPoints.clear();

    if (!d->currentHelper)
        return false;

    bool flag = d->currentHelper->finish(event->pos());
    d->currentHelper = NULL;

    return flag;
}

bool MapHelperUniversal::mouseMove(QMouseEvent *event)
{    
    Q_D(MapHelperUniversal);
    D_MAP;

    d->touch->touchPoints.clear();
    d->touch->touchPoints << makeTouchPointFromMouseEvent(event, Qt::TouchPointMoved);

    if (!d->currentHelper)
        return false;
    return d->currentHelper->proccessing(event->pos());
}

bool MapHelperUniversal::wheelEvent(QWheelEvent *event)
{
    Q_D(MapHelperUniversal);
    D_MAP;

    QTransform tr = MapSubHelperScale::scale(d->map->camera(), event->pos(), event->delta() / 120);
    emit d->worldChanged(NULL, tr);

    d->map->updateScene();

    return true;
}

bool MapHelperUniversal::touch(QTouchEvent *event)
{
    Q_D(MapHelperUniversal);
    D_MAP;

    if (!d->touch)
        return false;

    d->touch->touchPoints.clear();
    for (int i = 0; i < event->touchPoints().count(); ++i) {
        if (!(event->touchPoints().at(i).state() & Qt::TouchPointReleased)) {
            d->touch->touchPoints << event->touchPoints().at(i);
        }
    }

    switch (event->type()) {
    case QEvent::TouchBegin:
        d->touch->tapState = MapHelperTouch::tapNone;
    case QEvent::TouchUpdate: {
        d->touch->touchPoints.clear();
        for (int i = 0; i < event->touchPoints().count(); ++i) {
            if (!(event->touchPoints().at(i).state() & Qt::TouchPointReleased)) {
                d->touch->touchPoints << event->touchPoints().at(i);
            }
        }
        d->touch->updateTouchPosition();
        break;
    }
    case QEvent::TouchEnd: {
        if (d->touch->tapState == MapHelperTouch::tapDown) {
            d->touch->tapState = MapHelperTouch::tapUp;

            MapCamera *cam = d->map->camera();
            MapOptions opt = d->map->settings()->mapOptions();
            QRectF rgn;
            if (!d->touch->touchPoints.isEmpty())
                rgn = d->touch->touchPoints.first().sceneRect();
            else if (!event->touchPoints().isEmpty())
                rgn = event->touchPoints().first().sceneRect();

            rgn.moveCenter(rgn.center() - d->map->position());

#warning ERROR
#if 0
            QList<MapObject*> lo(d->map->selectObjects(rgn, cam));
            QList<MapObject*> reslo;
            reslo.reserve(lo.size());
            foreach (MapObject *mo, lo)
                if (mo->drawer()->hit(mo, rgn, cam, opt))
                    reslo.append(mo);

            MHETap mhtap(reslo, rgn.center());
            emit d->handler(&mhtap);
#endif
        }
        d->touch->touchPoints.clear();
        d->touch->updateTouchPosition();
        break;
    }
    default:
        // no-op
        break;
    }

    return true;
}

void MapHelperUniversal::resize(QSizeF oldSize, QSizeF newSize)
{
    Q_D(MapHelperUniversal);
    QPointF oldMid(oldSize.width() * 0.5, oldSize.height() * 0.5);
    QPointF newMid(newSize.width() * 0.5, newSize.height() * 0.5);

    QPointF dt(newMid - oldMid);

    QTransform tr;
    tr.translate(dt.x(), dt.y());
    emit d->worldChanged(NULL, tr);
}

void MapHelperUniversal::reject(States st)
{
    Q_D(MapHelperUniversal);

    States tmpSt = d->states;
    st &= d->states;

    for (int key = 1, max = st; key <= max; key <<= 1)
        if (st & key)
            d->removeSubhelper(key);

    if (tmpSt != d->states) {
        MHEModeRejected mrej(d->states);
        emit d->handler(&mrej);

        emit stateChanged(d->states);
    }
}

void MapHelperUniversal::finish()
{
    Q_D(MapHelperUniversal);
    MapHelper::finish();

    reject(d->states);
    if (d->recv)
        disconnect(d->recv, 0, this, SLOT(finish()));

    MHEFinished fin(d->recv);
    emit d->handler(&fin);

    disconnect(d, SIGNAL(handler(minigis::MHEvent*)), 0, 0);

    d->recv = NULL;

    // TODO: как правильно?
    //    d->states = stNoop;

    MapSubHelper *sub = new MapSubHelperMove(d->map);
    d->addSubhelper(sub);
}

//----------------------------------------------------------

} // minigis

//----------------------------------------------------------

#undef D_MAP
