#ifndef MAPHELPERTOUCH_H
#define MAPHELPERTOUCH_H

#include <QObject>
#include <QPropertyAnimation>
#include <QElapsedTimer>
#include <QTouchEvent>

//----------------------------------------------------------

namespace minigis {

//----------------------------------------------------------

class MapFrame;

//----------------------------------------------------------

class MapDeclarativeGeoMapPinchEvent : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QPointF center READ center)
    Q_PROPERTY(qreal angle READ angle)
    Q_PROPERTY(QPointF point1 READ point1)
    Q_PROPERTY(QPointF point2 READ point2)
    Q_PROPERTY(int pointCount READ pointCount)
    Q_PROPERTY(bool accepted READ accepted WRITE setAccepted)

public:
    MapDeclarativeGeoMapPinchEvent(
            const QPointF &center, qreal angle,
            const QPointF &point1, const QPointF &point2,
            int pointCount = 0, bool accepted = true)
        : QObject(), center_(center), angle_(angle),
          point1_(point1), point2_(point2),
          pointCount_(pointCount), accepted_(accepted) {}
    MapDeclarativeGeoMapPinchEvent()
        : QObject(),
          angle_(0.0),
          pointCount_(0),
          accepted_(true) {}

    QPointF center() const { return center_; }
    void setCenter(const QPointF &center) { center_ = center; }
    qreal angle() const { return angle_; }
    void setAngle(qreal angle) { angle_ = angle; }
    QPointF point1() const { return point1_; }
    void setPoint1(const QPointF &p) { point1_ = p; }
    QPointF point2() const { return point2_; }
    void setPoint2(const QPointF &p) { point2_ = p; }
    int pointCount() const { return pointCount_; }
    void setPointCount(int count) { pointCount_ = count; }
    bool accepted() const { return accepted_; }
    void setAccepted(bool a) { accepted_ = a; }

private:
    QPointF center_;
    qreal angle_;
    QPointF point1_;
    QPointF point2_;
    int pointCount_;
    bool accepted_;
};

//----------------------------------------------------------

class MapHelperTouch : public QObject
{
    Q_OBJECT
    Q_ENUMS(ActiveGesture)
    Q_FLAGS(ActiveGestures)

    Q_PROPERTY(ActiveGestures activeGestures READ activeGestures WRITE setActiveGestures NOTIFY activeGesturesChanged)

public:
    MapHelperTouch(MapFrame *map_, QObject *parent = 0);
    ~MapHelperTouch();

    enum ActiveGesture {
        NoGesture    = 0x0000,
        ZoomGesture  = 0x0001,
        PanGesture   = 0x0002,
        FlickGesture = 0x0004,
        AllGesture   = ZoomGesture | PanGesture | FlickGesture
    };
    Q_DECLARE_FLAGS(ActiveGestures, ActiveGesture)

    ActiveGestures activeGestures() const;
    void setActiveGestures(ActiveGestures curentActiveGestures);

Q_SIGNALS:
    void pinchActiveChanged();
    void enabledChanged();
    void maximumZoomLevelChangeChanged();
    void activeGesturesChanged();
    void flickDecelerationChanged();

    // backwards compatibility
    void pinchEnabledChanged();
    void panEnabledChanged();

    void pinchStarted(MapDeclarativeGeoMapPinchEvent *pinch);
    void pinchUpdated(MapDeclarativeGeoMapPinchEvent *pinch);
    void pinchFinished(MapDeclarativeGeoMapPinchEvent *pinch);
    void panStarted();
    void panFinished();
    void flickStarted();
    void flickFinished();
    void movementStopped();

private Q_SLOTS:
    void endFlick();

public:
    void updateTouchPosition();

private:
    void touchPointStateMachine();
    void startOneTouchPoint();
    void updateOneTouchPoint();
    void startTwoTouchPoints();
    void updateTwoTouchPoints();

    void pinchStateMachine();
    bool canStartPinch();
    void startPinch();
    void updatePinch();
    void endPinch();

    void panStateMachine();
    bool canStartPan();
    void updatePan();
    bool tryStartFlick();
    bool startFlick(int dx, int dy, int timeMs = 0);

    bool isPinchActive() const;
    void setPinchActive(bool active);
    bool isPanActive() const;

    void stopPan();
    void clearTouchData();
    void updateVelocityList(const QPointF &pos);

public:
    bool enabled;

    struct Pinch
    {
        Pinch() : enabled(true), startDist(0), lastAngle(0.0) {}

        MapDeclarativeGeoMapPinchEvent event;
        bool enabled;
        struct Zoom
        {
            Zoom() : minimum(-1.0), maximum(-1.0), start(0.0), previous(0.0) {}
            qreal minimum;
            qreal maximum;
            qreal start;
            qreal previous;
        } zoom;

        QPointF lastPoint1;
        QPointF lastPoint2;
        qreal startDist;
        qreal lastAngle;
    } pinch;

    ActiveGestures activeGesture;

    struct Pan
    {
        qreal maxVelocity;
        qreal deceleration;
        QScopedPointer<QPropertyAnimation> animation;
        bool enabled;
    } pan;

    // these are calculated regardless of gesture or number of touch points
    qreal velocityX;
    qreal velocityY;
    QElapsedTimer lastPosTime;
    QPointF lastPos;
    QList<QTouchEvent::TouchPoint> touchPoints;
    QPointF sceneStartPoint1;

    enum TapState {
        tapNone,
        tapDown,
        tapClear,
        tapUp
    } tapState;

    // only set when two points in contact
    QPointF sceneStartPoint2;
    QPointF startCoord;
//    MapGeoCoordinate touchCenterCoord;
    qreal twoTouchAngle;
    qreal distanceBetweenTouchPoints;
    QPointF sceneCenter;

    enum TouchPointState
    {
        touchPoints0,
        touchPoints1,
        touchPoints2
    } touchPointState;

    enum PinchState
    {
        pinchInactive,
        pinchActive
    } pinchState;

    enum PanState
    {
        panInactive,
        panActive,
        panFlick
    } panState;

    MapFrame *map;
};

//----------------------------------------------------------

} // namespace minigis

//----------------------------------------------------------

#endif // MAPHELPERTOUCH_H
