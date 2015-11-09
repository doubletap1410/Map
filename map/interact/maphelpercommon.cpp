#include <QTimer>
#include <QImage>

#include "maphelpercommon.h"

// --------------------------------------------

namespace minigis {

// --------------------------------------------

MarkerAnimation::MarkerAnimation(MapObject *obj, QObject *parent, int duration, QEasingCurve curve)
    : QObject(parent), marker(obj)
{
    animator = new QTimeLine(duration, this);
    animator->setDirection(QTimeLine::Forward);
    animator->setEasingCurve(curve);
    animator->setUpdateInterval(updateAnimationTime);

    connect(animator, SIGNAL(valueChanged(qreal)), SLOT(updateMarker(qreal)));
    connect(animator, SIGNAL(finished()), SLOT(finishAni()));

    startOpacity = 255;
    endOpacity = 255;
}

MarkerAnimation::~MarkerAnimation()
{
    if (animator) {
        if (animator->state() != QTimeLine::NotRunning)
            animator->stop();
        delete animator;
        animator = NULL;
    }
    marker = NULL;
}

QTimeLine::State MarkerAnimation::state()
{
    return animator->state();
}

void MarkerAnimation::setEndPosition(QPointF pos)
{
    endPos = pos;
}

void MarkerAnimation::setEndOpacity(qreal opacity)
{
    endOpacity = opacity;
}

MapObject *MarkerAnimation::markerObj() const
{
    return marker;
}

void MarkerAnimation::start()
{
    if (animator->state() != QTimeLine::NotRunning)
        animator->stop();

    if (marker->metric().size() != 0)
        startPos = marker->metric().toMercator(0).first();

    QVariant var = marker->attribute(attrOpacity);
    startOpacity = var.isNull() ? 255 : var.toReal();

    animator->start();
}

void MarkerAnimation::stop()
{
    animator->stop();
}

void MarkerAnimation::updateMarker(qreal t)
{
    QPointF r(1, 1);
    QVariant const &tmp = marker->semantic(ImageSemantic);
    if (!tmp.canConvert<QImage>())
        r = 2 * marker->semantic(PointSemantic).toPointF();
    else {
        QSize s = tmp.value<QImage>().size();
        r = QPointF(s.width(), s.height());
    }

    QPointF mid = startPos + (endPos - startPos) * t;
    QRect upd((mid - r * 0.5).toPoint(), QSizeF(r.x(), r.y()).toSize());

    if (marker->metric().size() != 0) {
        QRect tmp((marker->metric().toMercator(0).first() - r * 0.5).toPoint(), QSizeF(r.x(), r.y()).toSize());
        upd = upd.united(tmp);
    }

    marker->setMetric(Metric(mid, Mercator));
    marker->setAttribute(attrOpacity, startOpacity + (endOpacity - startOpacity) * t);
//    emit valueChanged(upd);
    emit valueChanged(QRect());
}

void MarkerAnimation::finishAni()
{
    if (qFuzzyIsNull(endOpacity))
        marker->setMetric(Metric(Mercator));
    emit finished();
}

//----------------------------------------------------------

MarkerAnimationGroup::MarkerAnimationGroup(QObject *parent) :
    QObject(parent)
{
    currentAni = -1;
}

MarkerAnimationGroup::~MarkerAnimationGroup()
{

}

void MarkerAnimationGroup::addAnimation(MarkerAnimation *ani, int pauseAfter)
{
    if (animations.contains(ani))
        return;

    animations.append(ani);
    pauses.append(pauseAfter);
}

void MarkerAnimationGroup::start()
{
    if (isRunning())
        stop();

    if (animations.isEmpty())
        return;

    currentAni = 0;
    startNext();
}

void MarkerAnimationGroup::stop()
{
    foreach (MarkerAnimation *ani, animations)
        ani->stop();
}

bool MarkerAnimationGroup::isRunning()
{
    bool b = false;
    foreach (MarkerAnimation *ani, animations)
        b |= ani->state() != QTimeLine::NotRunning;
    return b;
}

void MarkerAnimationGroup::startNext()
{
    if (currentAni == -1)
        return;

    MarkerAnimation *ani = animations.at(currentAni);
    int pause = pauses.at(currentAni);

    ani->start();
    QTimer::singleShot(pause, this, SLOT(startNext()));

    if (++currentAni == animations.size())
        currentAni = -1;
}

// --------------------------------------------

} // namespace minigis

// --------------------------------------------
