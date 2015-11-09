#include <QApplication>
#include <QStyleHints>

#include "frame/mapframe.h"
#include "coord/mapcamera.h"

#include "maphelpertouch.h"

//----------------------------------------------------------

#if defined ( QT_4 )
#define StartDragDistance 30
#endif

#define MAP_FLICK_DEFAULTMAXVELOCITY 2500
#define MAP_FLICK_MINIMUMDECELERATION 500
#define MAP_FLICK_DEFAULTDECELERATION 2500
#define MAP_FLICK_MAXIMUMDECELERATION 10000
#define MAP_FLICK_VELOCITY_SAMPLE_PERIOD 50

static const int FlickThreshold = 20;
const qreal MinimumFlickVelocity = 75.0;

//----------------------------------------------------------

namespace minigis {

//----------------------------------------------------------

MapHelperTouch::MapHelperTouch(MapFrame *map_, QObject *parent)
    : QObject(parent), map(map_)
{
    activeGesture = AllGesture;
    pan.enabled = true;
    pan.maxVelocity  = MAP_FLICK_DEFAULTMAXVELOCITY;
    pan.deceleration = MAP_FLICK_DEFAULTDECELERATION;
    touchPointState = touchPoints0;
    pinchState = pinchInactive;
    panState = panInactive;
    tapState = tapNone;

    pan.animation.reset(new QPropertyAnimation(map->camera(), "pos", this));
    pan.animation->setEasingCurve(QEasingCurve(QEasingCurve::OutQuad));
    connect(pan.animation.data(), SIGNAL(finished()), this, SLOT(endFlick()));
    connect(pan.animation.data(), SIGNAL(valueChanged(QVariant)), map, SLOT(update()));
    //connect(this, SIGNAL(movementStopped()), map, SLOT(cameraStopped()));

    // TODO: вынести в property у mapFrame
    enabled = true;
}

MapHelperTouch::~MapHelperTouch()
{

}

MapHelperTouch::ActiveGestures MapHelperTouch::activeGestures() const
{
    return activeGesture;
}

void MapHelperTouch::setActiveGestures(ActiveGestures curentActiveGestures)
{
    if (activeGesture == curentActiveGestures)
        return;
    activeGesture = curentActiveGestures;
    emit activeGesturesChanged();
}

void MapHelperTouch::endFlick()
{
    emit panFinished();
    if (pan.animation->state() == QPropertyAnimation::Running)
        pan.animation->stop();
    emit flickFinished();
    panState = panInactive;
    emit movementStopped();
}

void MapHelperTouch::updateTouchPosition()
{
    // First state machine is for the number of touch points
    touchPointStateMachine();

    // Parallel state machine for pinch
    if (isPinchActive() || (enabled && pinch.enabled && (activeGesture & (ZoomGesture))))
        pinchStateMachine();

    // Parallel state machine for pan (since you can pan at the same time as pinching)
    // The stopPan function ensures that pan stops immediately when disabled,
    // but the line below allows pan continue its current gesture if you disable
    // the whole gesture (enabled_ flag), this keeps the enabled_ consistent with the pinch
    if (isPanActive() || (enabled && pan.enabled && (activeGesture & (PanGesture | FlickGesture))))
        panStateMachine();
}

void MapHelperTouch::touchPointStateMachine()
{
    // Transitions:
    switch (touchPointState) {
    case touchPoints0:
        if (touchPoints.count() == 1) {
            clearTouchData();
            startOneTouchPoint();
            touchPointState = touchPoints1;
        } else if (touchPoints.count() == 2) {
            clearTouchData();
            startTwoTouchPoints();
            touchPointState = touchPoints2;
        }
        break;
    case touchPoints1:
        if (touchPoints.count() == 0) {
            touchPointState = touchPoints0;
        } else if (touchPoints.count() == 2) {
//            touchCenterCoord = map->camera()->toWorld(MapDoubleVector2D(sceneCenter));
            startTwoTouchPoints();
            startCoord = map->camera()->toWorld(lastPos);
            touchPointState = touchPoints2;
        }
        break;
    case touchPoints2:
        if (touchPoints.count() == 0) {
            touchPointState = touchPoints0;
        } else if (touchPoints.count() == 1) {
//            touchCenterCoord = map->camera()->toWorld(MapDoubleVector2D(sceneCenter));
            startOneTouchPoint();
            startCoord = map->camera()->toWorld(lastPos);
            touchPointState = touchPoints1;
        }
        break;
    };

    // Update
    switch (touchPointState) {
    case touchPoints0:
        break; // do nothing if no touch points down
    case touchPoints1:
        updateOneTouchPoint();
        break;
    case touchPoints2:
        updateTwoTouchPoints();
        break;
    }
}

void MapHelperTouch::startOneTouchPoint()
{
    sceneStartPoint1 = touchPoints.at(0).scenePos();
    lastPos = sceneStartPoint1;
    lastPosTime.start();
}

void MapHelperTouch::updateOneTouchPoint()
{
    sceneCenter = touchPoints.at(0).scenePos();
    updateVelocityList(sceneCenter);
}

void MapHelperTouch::startTwoTouchPoints()
{
    sceneStartPoint1 = touchPoints.at(0).scenePos();
    sceneStartPoint2 = touchPoints.at(1).scenePos();
    QPointF startPos = (sceneStartPoint1 + sceneStartPoint2) * 0.5;
    lastPos = startPos;
    lastPosTime.start();
}

void MapHelperTouch::updateTwoTouchPoints()
{
    QPointF p1 = touchPoints.at(0).scenePos();
    QPointF p2 = touchPoints.at(1).scenePos();
    qreal dx = p1.x() - p2.x();
    qreal dy = p1.y() - p2.y();
    distanceBetweenTouchPoints = sqrt(dx * dx + dy * dy);
    sceneCenter = (p1 + p2) / 2;

    updateVelocityList(sceneCenter);

    twoTouchAngle = QLineF(p1, p2).angle();
    if (twoTouchAngle > 180)
        twoTouchAngle -= 360;
}

void MapHelperTouch::pinchStateMachine()
{
    PinchState lastState = pinchState;
    // Transitions:
    switch (pinchState) {
    case pinchInactive:
        if (canStartPinch()) {
            startPinch();
            setPinchActive(true);
        }
        break;
    case pinchActive:
        if (touchPoints.count() <= 1) {
            endPinch();
            setPinchActive(false);
        }
        break;
    }
    // This line implements an exclusive state machine, where the transitions and updates don't
    // happen on the same frame
    if (pinchState != lastState)
         return;

    // Update
    switch (pinchState) {
    case pinchInactive:
        break; // do nothing
    case pinchActive:
        updatePinch();
        break;
    }
}

bool MapHelperTouch::canStartPinch()
{
    const int startDragDistance = qApp->styleHints()->startDragDistance();

    if (touchPoints.count() >= 2) {
        QPointF p1 = touchPoints.at(0).scenePos();
        QPointF p2 = touchPoints.at(1).scenePos();
        if (qAbs(p1.x()-sceneStartPoint1.x()) > startDragDistance
         || qAbs(p1.y()-sceneStartPoint1.y()) > startDragDistance
         || qAbs(p2.x()-sceneStartPoint2.x()) > startDragDistance
         || qAbs(p2.y()-sceneStartPoint2.y()) > startDragDistance) {
            pinch.event.setCenter(sceneCenter);
            pinch.event.setAngle(twoTouchAngle);
            pinch.event.setPoint1(p1);
            pinch.event.setPoint2(p2);
            pinch.event.setPointCount(touchPoints.count());
            pinch.event.setAccepted(true);
            emit pinchStarted(&pinch.event);
            return pinch.event.accepted();
        }
    }
    return false;
}

void MapHelperTouch::startPinch()
{
    pinch.startDist = distanceBetweenTouchPoints;
    pinch.zoom.previous = 1.0;
    pinch.lastAngle = twoTouchAngle;

    pinch.lastPoint1 = touchPoints.at(0).scenePos();
    pinch.lastPoint2 = touchPoints.at(1).scenePos();

    pinch.zoom.start = map->camera()->scale();
    pinch.zoom.minimum = map->camera()->scaleForZoomLevel(0);
    pinch.zoom.maximum = map->camera()->scaleForZoomLevel(23);

    panState = panInactive;
}

void MapHelperTouch::updatePinch()
{
    // Calculate the new zoom level if we have distance ( >= 2 touchpoints), otherwise stick with old.
    qreal newScale = pinch.zoom.previous;
    if (distanceBetweenTouchPoints)
        newScale = qBound(pinch.zoom.minimum,
                          distanceBetweenTouchPoints / pinch.startDist * pinch.zoom.start,
                          pinch.zoom.maximum);
    qreal da = pinch.lastAngle - twoTouchAngle;
    if (da > 180)
        da -= 360;
    else if (da < -180)
        da += 360;
    pinch.event.setCenter(sceneCenter);
    pinch.event.setAngle(twoTouchAngle);

    pinch.lastPoint1 = touchPoints.at(0).scenePos();
    pinch.lastPoint2 = touchPoints.at(1).scenePos();
    pinch.event.setPoint1(pinch.lastPoint1);
    pinch.event.setPoint2(pinch.lastPoint2);
    pinch.event.setPointCount(touchPoints.count());
    pinch.event.setAccepted(true);

    pinch.lastAngle = twoTouchAngle;
    emit pinchUpdated(&pinch.event);

    if (activeGesture & ZoomGesture) {
        QPointF mid = sceneCenter;
        if (touchPoints.size() >= 2) {
            QTouchEvent::TouchPoint tp0 = touchPoints.at(0);
            QTouchEvent::TouchPoint tp1 = touchPoints.at(1);
            // если оба пальца двигаются, крутим карту через центр
            if (tp0.state() == Qt::TouchPointMoved && tp1.state() == Qt::TouchPointMoved)
                mid = sceneCenter;
            // если один из пальцев стоит на месте, крутим карту вогруг него
            else if (tp0.state() == Qt::TouchPointStationary)
                mid = tp0.scenePos();
            else if (tp1.state() == Qt::TouchPointStationary)
                mid = tp1.scenePos();
        }

#warning Если вызывать rotateBy с параметром mid, то работает НЕ ВЕРНО
#if 0
    Порядок такой
    1. Зажимаем 1 палец
    2. И не двигая первый, начинаем водить второй - карта идет вразнос
    Если начать крутить сразу 2-мя пальцами, то такого эффекта не происходит
#endif

        map->camera()->setScale(newScale, mid);
        map->camera()->rotateBy(da);//, mid);
        map->update();
    }
}

void MapHelperTouch::endPinch()
{
    QPointF pinchCenter = sceneCenter; //declarativeMap->mapFromScene(sceneCenter);
    pinch.event.setCenter(pinchCenter);
    pinch.event.setAngle(pinch.lastAngle);
    pinch.event.setPoint1(pinch.lastPoint1 /*declarativeMap->mapFromScene(pinch.lastPoint1)*/);
    pinch.event.setPoint2(pinch.lastPoint2 /*declarativeMap->mapFromScene(pinch.lastPoint2)*/);
    pinch.event.setAccepted(true);
    pinch.event.setPointCount(0);
    emit pinchFinished(&pinch.event);
    pinch.startDist = 0;
}

void MapHelperTouch::panStateMachine()
{
    PanState lastState = panState;

    // Transitions
    switch (panState) {
    case panInactive:
        if (tapState == tapNone)
            tapState = tapDown;
        if (canStartPan()) {
            // Update startCoord_ to ensure smooth start for panning when going over startDragDistance
            startCoord = map->camera()->toWorld(lastPos);
            panState = panActive;
            tapState = tapClear;
        }
        break;
    case panActive:
        if (touchPoints.count() == 0) {
            panState = panFlick;
            if (!tryStartFlick()) {
                panState = panInactive;
                // mark as inactive for use by camera
                if (pinchState == pinchInactive) {
                    emit panFinished();
                    emit movementStopped();
                }
            }
        }
        break;
    case panFlick:
        if (touchPoints.count() > 0) { // re touched before movement ended
            endFlick();
            startCoord = map->camera()->toWorld(lastPos);
            panState = panActive;
        }
        break;
    }
    // Update
    switch (panState) {
    case panInactive: // do nothing
        break;
    case panActive:
        updatePan();
        // this ensures 'panStarted' occurs after the pan has actually started
        if (lastState != panActive) {
            emit panStarted();
        }
        break;
    case panFlick:
        break;
    }
}

bool MapHelperTouch::canStartPan()
{
    if (touchPoints.count() == 0 || (activeGesture & PanGesture) == 0)
        return false;

    // Check if thresholds for normal panning are met.
    // (normal panning vs flicking: flicking will start from mouse release event).
    const int startDragDistance = qApp->styleHints()->startDragDistance();
    QPointF p = touchPoints.at(0).scenePos();
    return (qAbs(p.y() - sceneStartPoint1.y()) > startDragDistance)
            || (qAbs(p.x() - sceneStartPoint1.x()) > startDragDistance)
            ;
}

void MapHelperTouch::updatePan()
{
    QSize s = map->camera()->screenSize();
    map->camera()->moveTo(map->camera()->toWorld(QPointF(s.width(), s.height())  * 0.5 - lastPos + map->camera()->toScreen(startCoord)));
    map->update();
}

bool MapHelperTouch::tryStartFlick()
{
    if ((activeGesture & FlickGesture) == 0)
        return false;

    // if we drag then pause before release we should not cause a flick.
    qreal velocityXl = 0.0;
    qreal velocityYl = 0.0;
    if (lastPosTime.elapsed() < MAP_FLICK_VELOCITY_SAMPLE_PERIOD) {
        velocityXl = velocityX;
        velocityYl = velocityY;
    }

    int flickTimeX = 0;
    int flickTimeY = 0;
    int flickPixelsX = 0;
    int flickPixelsY = 0;

    if (qAbs(velocityXl) > MinimumFlickVelocity && qAbs(sceneCenter.x() - sceneStartPoint1.x()) > FlickThreshold) {
        // calculate X flick animation values
        qreal acceleration = pan.deceleration;
        if ((velocityXl > 0.0f) == (pan.deceleration > 0.0f))
            acceleration = acceleration * -1.0f;
        flickTimeX = static_cast<int>(-1000 * velocityXl / acceleration);
        flickPixelsX = -(flickTimeX * velocityXl) / (1000.0 * 2);
    }
    if (qAbs(velocityYl) > MinimumFlickVelocity && qAbs(sceneCenter.y() - sceneStartPoint1.y()) > FlickThreshold) {
        // calculate Y flick animation values
        qreal acceleration = pan.deceleration;
        if ((velocityYl > 0.0f) == (pan.deceleration > 0.0f))
            acceleration = acceleration * -1.0f;
        flickTimeY = static_cast<int>(-1000 * velocityYl / acceleration);
        flickPixelsY = -(flickTimeY * velocityYl) / (1000.0 * 2);
    }

    return startFlick(flickPixelsX, flickPixelsY, qMax(flickTimeX, flickTimeY));
}

bool MapHelperTouch::startFlick(int dx, int dy, int timeMs)
{
    if (timeMs < 0)
        return false;

    QPointF aniStart = map->camera()->position();
    if (pan.animation->state() == QPropertyAnimation::Running)
        pan.animation->stop();
    QPointF aniEnd = map->camera()->toWorld(map->camera()->toScreen(aniStart) + QPointF(dx, dy));
    pan.animation->setDuration(timeMs);
    pan.animation->setStartValue(QVariant::fromValue(aniStart));
    pan.animation->setEndValue(QVariant::fromValue(aniEnd));
    pan.animation->start();
    emit flickStarted();
    return true;
}

bool MapHelperTouch::isPinchActive() const
{
    return pinchState == pinchActive;
}

void MapHelperTouch::setPinchActive(bool active)
{
    if ((active && pinchState == pinchActive) || (!active && pinchState != pinchActive))
        return;
    pinchState = active ? pinchActive : pinchInactive;
    emit pinchActiveChanged();
}

bool MapHelperTouch::isPanActive() const
{
   return panState == panActive || panState == panFlick;
}

void MapHelperTouch::stopPan()
{
    velocityX = 0;
    velocityY = 0;
    if (panState == panFlick) {
        endFlick();
    } else if (panState == panActive) {
        emit panFinished();
        emit movementStopped();
    }
    panState = panInactive;
}

void MapHelperTouch::clearTouchData()
{
    velocityX = 0;
    velocityY = 0;
    sceneCenter = QPointF(0, 0);
//    touchCenterCoord.setLongitude(0);
//    touchCenterCoord.setLatitude(0);
    startCoord = QPointF(0, 0);
}

void MapHelperTouch::updateVelocityList(const QPointF &pos)
{
    // Take velocity samples every sufficient period of time, used later to determine the flick
    // duration and speed (when mouse is released).
    qreal elapsed = qreal(lastPosTime.elapsed());

    if (elapsed >= MAP_FLICK_VELOCITY_SAMPLE_PERIOD) {
        elapsed /= 1000.;
        int dyFromLastPos = pos.y() - lastPos.y();
        int dxFromLastPos = pos.x() - lastPos.x();
        lastPos = pos;
        lastPosTime.restart();
        qreal velX = qreal(dxFromLastPos) / elapsed;
        qreal velY = qreal(dyFromLastPos) / elapsed;
        velocityX = qBound<qreal>(-pan.maxVelocity, velX, pan.maxVelocity);
        velocityY = qBound<qreal>(-pan.maxVelocity, velY, pan.maxVelocity);
    }
}

//----------------------------------------------------------

} // namespace minigis

//----------------------------------------------------------
