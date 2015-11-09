#include "core/mapmath.h"
#include "core/mapdefs.h"
#include "coord/mapcoords.h"

#include "coord/mapcamera_p.h"
#include "coord/mapcamera.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

static QPointF boundPos(QPointF pos, QRectF boundRect)
{
    if (boundRect.isNull())
        return pos;

    return QPointF(
                qBound(boundRect.left(), pos.x(), boundRect.right() ),
                qBound(boundRect.top() , pos.y(), boundRect.bottom())
                );
}

// -------------------------------------------------------

MapCamera::MapCamera(QObject *parent)
    : QObject(parent), d_ptr(new MapCameraPrivate)
{
    Q_D(MapCamera);
    connect(d, SIGNAL(zoomChanged()), SIGNAL(zoomChanged()));
    connect(d, SIGNAL(posChanged()), SIGNAL(posChanged()));
    connect(d, SIGNAL(angleChanged()), SIGNAL(angleChanged()));
}

MapCamera::~MapCamera()
{
    if (d_ptr) {
        MapCameraPrivate *tmp = d_ptr;
        d_ptr = NULL;
        delete tmp;
    }
}

void MapCamera::init(MapFrame *map)
{
    Q_D(MapCamera);
    d->map = map;
}

void MapCamera::moveTo(QPointF pos, int duration, QPointF mid, QEasingCurve curve)
{
    Q_D(MapCamera);
    if (d->lMove.state() != QTimeLine::NotRunning)
        d->lMove.stop();

    pos = boundPos(pos, d->boundRect);
    bool tr = !mid.isNull();
    QPointF dt;
    if (tr)
        dt = pos + d->location + toWorld(QPointF(screenSize().width() - mid.x(), mid.y()));

    if (duration > 0)
        d->moveAnimation(tr ? -dt : -pos, duration, curve);
    else {
        {
            QMutexLocker locker(&d->mutex);
            d->transformMatrix.translate(-d->location.x(), -d->location.y());
            d->location = tr ? -dt : -pos;
            d->transformMatrix.translate(d->location.x(), d->location.y());

            d->beCacheInvers = false;
            d->beCacheZoom = false;
        }
        emit posChanged();
    }

    //qDebug() << "MoveTo:" << sender() << pos.toPoint();
}

void MapCamera::moveBy(QPointF delta, int duration, QEasingCurve curve)
{
    Q_D(MapCamera);
    if (duration > 0)
        moveTo(delta - d->location, duration, QPointF(), curve);
    else {
        {
            QMutexLocker locker(&d->mutex);
            if (d->lMove.state() != QTimeLine::NotRunning)
                d->lMove.stop();

            delta = -boundPos(-(d->location + delta), d->boundRect) - d->location;
            d->location += delta;
            d->transformMatrix.translate(delta.x(), delta.y());

            d->beCacheInvers = false;
            d->beCacheZoom = false;
        }
        emit posChanged();
    }
}

bool MapCamera::ensureVisible(QPointF pos, int duration, QEasingCurve curve)
{
    Q_D(MapCamera);

    QPolygon worldPoly = toWorld().mapToPolygon(QRect(QPoint(), d->screenSize));
    if (worldPoly.containsPoint(pos.toPoint(), Qt::OddEvenFill))
        return true;

    moveTo(pos, duration, QPoint(), curve);
    return false;
}

QPointF MapCamera::position() const
{
    Q_D(const MapCamera);
    return -d->location;
}

void MapCamera::setBounds(QRectF r)
{
    Q_D(MapCamera);
    if (!r.isNull())
        d->boundRect = r;
}

QRectF MapCamera::bounds() const
{
    Q_D(const MapCamera);
    return d->boundRect;
}

QPointF MapCamera::latLng() const
{
    return TileSystem::metersToLatLon(position());
}

void MapCamera::setLatLng(QPointF ll)
{
    moveTo(minigis::TileSystem::latLonToMeters(ll));
}

QString MapCamera::latLngStr() const
{
    QPointF p = TileSystem::metersToLatLon(position());
    return QString::number(p.x()) + " " + QString::number(p.y());
}

void MapCamera::setLatLngStr(QString ll)
{
    QStringList cameraLL = ll.split(" ");
    if (cameraLL.length() < 2)
        return;
    moveTo(minigis::TileSystem::latLonToMeters(QPointF(cameraLL.at(0).toDouble(), cameraLL.at(1).toDouble())));
}

void MapCamera::rotateTo(qreal angle, QPointF mid, int duration, QEasingCurve curve)
{
    Q_D(MapCamera);
    if (d->lRotate.state() != QTimeLine::NotRunning)
        d->lRotate.stop();

    if (!mid.isNull()) {
        QPointF dt = toWorld(mid);
        qreal len = lengthR2(dt + d->location);
        QPointF dVect = rotateVect(QPointF(1, 0), degToRad(angle)) * len;

        QMutexLocker locker(&d->mutex);
        d->location = -dt + dVect;
    }

    if (duration > 0)
        d->rotateAnimation(degToRad(angle), duration, curve);
    else {
        {
            QMutexLocker locker(&d->mutex);
            d->forward = rotateVect(d->forward, degToRad(angle) - polarAngle(d->forward));
            d->beCacheMatrix = false;
        }
        emit angleChanged();
    }
}

void MapCamera::rotateTo(QPointF angle, QPointF mid, int duration, QEasingCurve curve)
{
    if (angle.isNull())
        return;
    rotateTo(polarAngle(angle), mid, duration, curve);
}

void MapCamera::rotateBy(qreal delta, QPointF mid, int duration, QEasingCurve curve)
{
    rotateTo(angle() + delta, mid, duration, curve);
}

qreal MapCamera::angle() const
{
    return radToDeg(polarAngle(d_ptr->forward));
}

QPointF MapCamera::direction() const
{
    return d_ptr->forward;
}

qreal MapCamera::measurementError(QPointF pos) const
{
    QPointF geo = minigis::CoordTranform::worldToGeo(pos);
    qreal measure = qCos(degToRad(geo.x())) / scale();
    return 2 * measure;
}

qreal MapCamera::metricToScale(qreal metric)
{
    return PixTo1cm / metric;
}

qreal MapCamera::scaleToMetric(qreal scale)
{
    return PixTo1cm / scale;
}

void MapCamera::setScale(qreal sc, QPointF mid, int duration, QEasingCurve curve)
{
    Q_D(MapCamera);
    if (d->lScale.state() != QTimeLine::NotRunning)
        d->lScale.stop();

    if (!mid.isNull()) {
        QPointF dt = d->location + toWorld(mid);
        QMutexLocker locker(&d->mutex);
        d->location -= dt * (sc / d->scale - 1);
    }

    if (duration)
        d->scaleAnimation(sc, duration, curve);
    else {
        {
            QMutexLocker locker(&d->mutex);
            d->scale = qBound<double>(MaxScaleParam, sc, MinScaleParam);
            d->beCacheZoom = false;
            d->beCacheMatrix = false;
        }
        emit zoomChanged();
    }
}

void MapCamera::scaleIn(int stepCount, QPointF mid, int duration, QEasingCurve curve)
{
    Q_D(MapCamera);
    if (stepCount < 1)
        return;

    if (d->lScale.state() != QTimeLine::NotRunning)
        d->lScale.stop();

    qreal scaleFactor = pow(9 / 10., stepCount);
    if (!mid.isNull()) {
        QPointF dt = d->location + toWorld(mid);
        QMutexLocker locker(&d->mutex);
        d->location -= dt / scaleFactor * 0.1 ; // dt / (10% от 9/10);
    }

    if (duration > 0)
        d->scaleAnimation(d->scale / scaleFactor, duration, curve);
    else {
        {
            QMutexLocker locker(&d->mutex);
            d->scale = qBound<double>(MaxScaleParam, d->scale / scaleFactor, MinScaleParam);
            d->beCacheZoom = false;
            d->beCacheMatrix = false;
        }
        emit zoomChanged();
    }
}

void MapCamera::scaleOut(int stepCount, QPointF mid, int duration, QEasingCurve curve)
{
    Q_D(MapCamera);
    if (stepCount < 1)
        return;

    if (d->lScale.state() != QTimeLine::NotRunning)
        d->lScale.stop();

    qreal scaleFactor = pow(9 / 10., stepCount);
    if (!mid.isNull()) {
        QPointF dt = d->location + toWorld(mid);
        QMutexLocker locker(&d->mutex);
        d->location += dt * scaleFactor * 0.1; // dt * (10% от 9/10)
    }

    if (duration > 0)
        d->scaleAnimation(d->scale * scaleFactor, duration, curve);
    else {
        {
            QMutexLocker locker(&d->mutex);
            d->scale = qBound<double>(MaxScaleParam, d->scale * scaleFactor, MinScaleParam);
            d->beCacheZoom = false;
            d->beCacheMatrix = false;
        }
        emit zoomChanged();
    }
}

qreal MapCamera::scale() const
{
    return d_ptr->scale;
}

qreal MapCamera::zoom() const
{
    if (!d_ptr->beCacheZoom) {
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->zoom = d_ptr->calcZoom();
        d_ptr->beCacheZoom = true;
    }
    return d_ptr->zoom;
}

qreal MapCamera::measure() const
{
    return scaleToMetric(d_ptr->scale);
}

void MapCamera::setMeasure(qreal m)
{
    setScale(metricToScale(m));
}

int MapCamera::zoomLevel() const
{
    return TileSystem::zoomForPixelSize(1 / d_ptr->scale);
}

void MapCamera::setZoomLevel(int z)
{
    setScale(1 / TileSystem::groundResolution(z));
}

qreal MapCamera::scaleForZoomLevel(int z)
{
    return 1 / TileSystem::groundResolution(z);
}

void MapCamera::setScreenSize(QSize screen)
{
    Q_D(MapCamera);
    QMutexLocker locker(&d->mutex);
    d->screenSize = screen;
    d->changeMatrix();
}

QSize MapCamera::screenSize() const
{
    Q_D(const MapCamera);
    return d->screenSize;
}

QPolygonF MapCamera::worldRect() const
{
    Q_D(const MapCamera);
    return toWorld().mapToPolygon(QRect(QPoint(), d->screenSize));
}

void MapCamera::setMidPosition(qreal width, qreal heigth)
{
    setMidPosition(QPointF(width, heigth));
}

void MapCamera::setMidPosition(QPointF mid)
{
    Q_D(MapCamera);
    d->midPos = mid;
    d->beCacheMatrix = false;
}

QPointF MapCamera::toWorld(QPointF pos) const
{
    QMutexLocker locker(&d_ptr->mutex);
    if (!d_ptr->beCacheMatrix) {
        d_ptr->changeMatrix();
        d_ptr->beCacheMatrix = true;
    }
    if (!d_ptr->beCacheInvers) {
        d_ptr->inversMatrix = d_ptr->transformMatrix.inverted();
        d_ptr->beCacheInvers = true;
    }
    return d_ptr->inversMatrix.map(pos);
}

QPointF MapCamera::toScreen(QPointF pos) const
{
    QMutexLocker locker(&d_ptr->mutex);
    if (!d_ptr->beCacheMatrix) {
        d_ptr->changeMatrix();
        d_ptr->beCacheMatrix = true;
    }
    return d_ptr->transformMatrix.map(pos);
}

const QTransform &MapCamera::toScreen() const
{
    QMutexLocker locker(&d_ptr->mutex);
    if (!d_ptr->beCacheMatrix) {
        d_ptr->changeMatrix();
        d_ptr->beCacheMatrix = true;
    }
    return d_ptr->transformMatrix;
}

const QTransform &MapCamera::toWorld() const
{
    QMutexLocker locker(&d_ptr->mutex);
    if (!d_ptr->beCacheMatrix) {
        d_ptr->changeMatrix();
        d_ptr->beCacheMatrix = true;
    }
    if (!d_ptr->beCacheInvers) {
        d_ptr->inversMatrix = d_ptr->transformMatrix.inverted();
        d_ptr->beCacheInvers = true;
    }
    return d_ptr->inversMatrix;
}

// -------------------------------------------------------

} //minigis

// -------------------------------------------------------
