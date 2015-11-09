#include "frame/mapframe.h"

#include "coord/mapcoords.h"
#include "core/mapmath.h"

#include "mapcamera_p.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

MapCameraPrivate::MapCameraPrivate(QObject *parent)
    : QObject(parent), map(NULL)
{
    midPos = QPointF(0.5, 0.5);
    location = QPointF(0, 0);
    forward = QPointF(1, 0);
    scale = 1;
    zoom = 1;
    beCacheMatrix = false;
    beCacheInvers = false;
    beCacheZoom = false;

    boundRect = QRectF();
}

MapCameraPrivate::~MapCameraPrivate()
{
}

qreal MapCameraPrivate::calcZoom()
{
    // TODO: переделать
    return qCos(degToRad(TileSystem::metersToLatLon(-location).x())) * PixTo1m / scale;
}

void MapCameraPrivate::changeMatrix()
{
    transformMatrix.reset();
    QPointF mid = midScreenPos();
    transformMatrix.translate(mid.x(), mid.y()).scale(scale, scale).rotate(-90 + radToDeg(polarAngle(forward))).translate(location.x(), location.y());

    beCacheInvers = false;
}

QPointF MapCameraPrivate::midScreenPos()
{
    return QPointF(screenSize.width() * midPos.x(), screenSize.height() * midPos.y());
}

void MapCameraPrivate::moveAnimation(QPointF finish, int duration, QEasingCurve curve)
{
    lMove.setDirection(QTimeLine::Forward);
    lMove.setEasingCurve(curve);
    lMove.setDuration(duration);
    lMove.setUpdateInterval(updateAnimationTime);

    startPos = location;
    finishPos = finish;

    disconnect(&lMove, 0, this, 0);
    connect(&lMove, SIGNAL(valueChanged(qreal)), this, SLOT(updatePos(qreal)));
    lMove.start();
}

void MapCameraPrivate::rotateAnimation(qreal finish, int duration, QEasingCurve curve)
{
    lRotate.setDirection(QTimeLine::Forward);
    lRotate.setEasingCurve(curve);
    lRotate.setDuration(duration);
    lRotate.setUpdateInterval(updateAnimationTime);

    startVect = forward;
    startAngle = polarAngle(forward);
    finishAngle = finish;
    disconnect(&lRotate, 0, this, 0);
    connect(&lRotate, SIGNAL(valueChanged(qreal)), this, SLOT(updateAngle(qreal)));
    lRotate.start();
}

void MapCameraPrivate::scaleAnimation(qreal finish, int duration, QEasingCurve curve)
{
    lScale.setEasingCurve(curve);
    lScale.setDuration(duration);
    lScale.setUpdateInterval(updateAnimationTime);

    startScale = scale;
    finishScale = finish;
    disconnect(&lScale, 0, this, 0);
    connect(&lScale, SIGNAL(valueChanged(qreal)), this, SLOT(updateScale(qreal)));
    lScale.start();
}

void MapCameraPrivate::updatePos(qreal dt)
{
    QPointF vect = finishPos - startPos;
    {
        QMutexLocker locker(&mutex);

        transformMatrix.translate(-location.x(), -location.y());
        location = startPos + vect * dt;
        transformMatrix.translate(location.x(), location.y());

        beCacheInvers = false;
        beCacheZoom = false;
    }
    emit posChanged();
    if (map)
        map->update();
}

void MapCameraPrivate::updateAngle(qreal dt)
{
    qreal vect = finishAngle - startAngle;
    if (startAngle > M_PI + finishAngle)
        vect += 2 * M_PI;
    {
        QMutexLocker locker(&mutex);
        forward = rotateVect(startVect, -vect * dt);
        beCacheMatrix = false;
    }
    emit angleChanged();
    if (map)
        map->update();
}

void MapCameraPrivate::updateScale(qreal dt)
{
    qreal sc = finishScale - startScale;
    {
        QMutexLocker locker(&mutex);
        scale = startScale + sc * dt;
        if (scale < MaxScaleParam) {
            scale = MaxScaleParam;
            lScale.stop();
        }
        if (scale > MinScaleParam) {
            scale = MinScaleParam;
            lScale.stop();
        }
        beCacheMatrix = false;
        beCacheZoom = false;
    }
    emit zoomChanged();
    if (map)
        map->update();
}

// -------------------------------------------------------

} //minigis

// -------------------------------------------------------
