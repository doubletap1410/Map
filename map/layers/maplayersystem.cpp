
#include "frame/mapframe.h"

#include "drawer/mapdrawer.h"
#include "drawer/mapdrawercommon.h"
#include "object/mapobject.h"
#include "core/mapmath.h"
#include "core/mapdefs.h"
#include "frame/mapsettings.h"
#include "coord/mapcoords.h"

#include "layers/maplayersystem.h"

// -----------------------------------------------------------

namespace minigis {

// -----------------------------------------------------------

MapLayerSystemPrivate::MapLayerSystemPrivate(QObject *parent)
    : MapLayerWithObjectsPrivate(parent)
{
}

MapLayerSystemPrivate::~MapLayerSystemPrivate()
{
}

// -----------------------------------------------------------

class LineHelper
{
public:
    LineHelper(QPointF st, QPointF en)
        : begin(st), end(en)
    {
        if (st.x() < en.x()) {
            minX = st.x();
            maxX = en.x();
        }
        else {
            minX = en.x();
            maxX = st.x();
        }

        if (st.y() < en.y()) {
            minY = st.y();
            maxY = en.y();
        }
        else {
            minY = en.y();
            maxY = st.y();
        }
    }

    qreal getX(qreal y, bool &isBound)
    {
        qreal tmpX = 0;
        if (qFuzzyIsNull(end.y() - begin.y())) {
            tmpX = begin.x();
            isBound = true;
        }
        else {
            tmpX = (y - begin.y()) * (end.x() - begin.x()) / (end.y() - begin.y()) + begin.x();
            isBound = (minX < tmpX) && (tmpX < maxX);
        }
        return tmpX;
    }

    qreal getY(qreal x, bool &isBound)
    {
        qreal tmpY = 0;
        if (qFuzzyIsNull(end.x() - begin.x())) {
            tmpY = begin.y();
            isBound = true;
        }
        else {
            tmpY = (x - begin.x()) * (end.y() - begin.y()) / (end.x() - begin.x()) + begin.y();
            isBound = (minY < tmpY) && (tmpY < maxY);
        }
        return tmpY;
    }

private:
    QPointF begin;
    QPointF end;
    qreal minX;
    qreal maxX;
    qreal minY;
    qreal maxY;
};

// -----------------------------------------------------------

void MapLayerSystemPrivate::render(QPainter *painter, const QRectF &rgn, const MapCamera *camera, MapOptions options)
{
    Q_Q(MapLayerWithObjects);
    Q_UNUSED(camera);
    Q_UNUSED(options);

    for (QListIterator<MapObject *> it(q->objects()); it.hasNext(); )
        drawObj(it.next(), painter, rgn);

#if 0
    bool ok;
    qreal h = map->hMatrix()->HPoint(camera->position(), &ok);
    if (ok)
        qDebug() << h;
    else
        qDebug() << "No height";
#endif

#if 0
    static int textWidth = 0;

    QPointF geo = minigis::CoordTranform::worldToGeo(camera->position());
    qreal measure = qCos(degToRad(geo.x())) * PixTo1cm / camera->scale();

    if (map->settings()->isGridEnabled() && measure < gridMaxSize) {
        int factor = gridMeasureMid;
        if (measure < gridSize25 - gridSize25 / gridDissolve)
            factor = gridMeasureMin;
        else if (measure > gridSize100 - gridSize100 / gridDissolve)
            factor = gridMeasureMax;

        QRect screenRect(QPoint(), camera->screenSize());
        int zone = minigis::CoordTranform::zoneSK42FromWorld(camera->toWorld(screenRect.center()));

        QRect rect = camera->toWorld().mapRect(screenRect);
        QPointF tl = minigis::CoordTranform::worldToSK42(rect.topLeft(), zone);
        QPointF tr = minigis::CoordTranform::worldToSK42(rect.topRight(), zone);
        QPointF br = minigis::CoordTranform::worldToSK42(rect.bottomRight(), zone);
        QPointF bl = minigis::CoordTranform::worldToSK42(rect.bottomLeft(), zone);

        LineHelper top(tl, tr);
        LineHelper right(tr, br);
        LineHelper bottom(br, bl);
        LineHelper left(bl, tl);

        PainterSaver save(painter);

        QPen pen(Qt::gray, 1, Qt::SolidLine);
        painter->setPen(pen);

        qreal maxX = qMax(tl.x(), qMax(tr.x(), qMax(br.x(), bl.x())));
        qreal minX = qMin(tl.x(), qMin(tr.x(), qMin(br.x(), bl.x())));
        qreal maxY = qMax(tl.y(), qMax(tr.y(), qMax(br.y(), bl.y())));
        qreal minY = qMin(tl.y(), qMin(tr.y(), qMin(br.y(), bl.y())));

        int stepMinX = qCeil(minX / (qreal)factor) * factor;
        int stepMaxX = qCeil(maxX / (qreal)factor) * factor;

        for (int stepX = stepMinX; stepX < stepMaxX; stepX += factor) {
            bool ok = false;
            qreal y = 0;
            QPolygonF tmpLine;

            y = top.getY(stepX, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(stepX, y), zone));
            y = right.getY(stepX, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(stepX, y), zone));
            y = bottom.getY(stepX, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(stepX, y), zone));
            y = left.getY(stepX, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(stepX, y), zone));

            if (tmpLine.size() != 2)
                continue;

            QLineF l = camera->toScreen().map(QLineF(tmpLine.first(), tmpLine.last()));
            painter->drawLine(l);
        }

        int stepMinY = qCeil(minY / (qreal)factor) * factor;
        int stepMaxY = qCeil(maxY / (qreal)factor) * factor;

        for (int stepY = stepMinY; stepY < stepMaxY; stepY += factor) {
            bool ok = false;
            qreal x = 0;
            QPolygonF tmpLine;

            x = top.getX(stepY, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(x, stepY), zone));
            x = right.getX(stepY, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(x, stepY), zone));
            x = bottom.getX(stepY, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(x, stepY), zone));
            x = left.getX(stepY, ok);
            if (ok) tmpLine.append(minigis::CoordTranform::sk42ToWorld(QPointF(x, stepY), zone));

            if (tmpLine.size() != 2)
                continue;

            QLineF l = camera->toScreen().map(QLineF(tmpLine.first(), tmpLine.last()));
            painter->drawLine(l);
        }
    }
#endif
}

void MapLayerSystemPrivate::drawObj(const MapObject *object, QPainter *painter, const QRectF &rgn)
{
    Q_ASSERT(object);
    Q_ASSERT(painter);

    QVector<QPolygonF> metric = object->metric().toMercator();
    if (metric.size() == 0 || metric.first().isEmpty())
        return;    

    HeOptions type(object->semantic(MarkersSemantic).toInt());
    // 1 - точки
    // 2 - линия
    // 4 - площадь

    QPolygonF fullTmp;
    for (int i = 0; i < metric.size(); ++i)
        fullTmp += metric.at(i);

    if (fullTmp.size() == 0)
        return;
    else {
        QRectF r = fullTmp.boundingRect();
        if (r.isEmpty())
            r.setSize(r.size() + QSizeF(1, 1));
        qreal dr = type.testFlag(hoPoint) ? 25 : 2;
        if (r.adjusted(-dr, -dr, dr, dr).intersected(rgn).isNull())
            return;
    }

    // TODO: защита от некоррекно введенной семантики
    painter->save();
    QVariant var = object->attribute(attrOpacity);
    painter->setOpacity((var.isNull() ? 255 : var.toReal()) / 255.);

    if (type.testFlag(hoHelpers)) { // вспомогательные линии
        painter->setPen(QPen(Qt::black, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        if (object->drawer())
            painter->drawLines(object->drawer()->lines(object));
    }
    if (type.testFlag(hoSquare)) { // площадь
        painter->setPen(object->semantic(PenSemanticSquare).value<QPen>());
        painter->setBrush(object->semantic(BrushSemanticSquare).value<QBrush>());
        metric = object->metric().toMercator();
        if (object->attributeDraw(attrSmoothed).toBool()) {
            for (int i = 0; i < metric.size(); ++i) {
                QPolygonF cubic = simpleCubicSpline(metric.at(i), object->drawer() ? object->drawer()->isClosed(object->classCode()) : false);
                if (cubic.size() < 4)
                    continue;
                QPainterPath bezie;
                bezie.moveTo(cubic.first());
                for (int i = 1; i < cubic.size() - 2; i+=3)
                    bezie.cubicTo(cubic.at(i), cubic.at(i+1), cubic.at(i+2));
                bezie.closeSubpath();
                painter->drawPath(bezie);
            }
        }
        else {
            for (int i = 0; i < metric.size(); ++i)
                painter->drawPolygon(metric.at(i));
        }
    }
    if (type.testFlag(hoLine)) { // линии
        painter->setPen(object->semantic(PenSemanticLine).value<QPen>());
        painter->setBrush(Qt::NoBrush);
        metric = object->metric().toMercator();
        if (object->attributeDraw(attrSmoothed).toBool()) {
            for (int i = 0; i < metric.size(); ++i) {
                QPolygonF cubic = simpleCubicSpline(metric.at(i), object->drawer() ? object->drawer()->isClosed(object->classCode()) : false);
                if (cubic.size() < 4)
                    continue;
                QPainterPath bezie;
                bezie.moveTo(cubic.first());
                for (int i = 1; i < cubic.size() - 2; i+=3)
                    bezie.cubicTo(cubic.at(i), cubic.at(i+1), cubic.at(i+2));
                painter->drawPath(bezie);
            }
        }
        else {
            for (int i = 0; i < metric.size(); ++i)
                painter->drawPolyline(metric.at(i));
        }
    }
    if (type.testFlag(hoPoint)) { // точки

        QVariant const &tmp = object->semantic(ImageSemantic);
        if (!tmp.canConvert<QImage>()) {
            painter->setPen(object->semantic(PenSemantic).value<QPen>());
            painter->setBrush(object->semantic(BrushSemantic).value<QBrush>());
            QPointF r(object->semantic(PointSemantic).toPointF());
            metric = object->metric().toMercator();
            for (int i = 0; i < metric.size(); ++i)
                foreach (QPointF p, metric.at(i))
                    painter->drawEllipse(p, r.x(), r.y());
        }
        else {
            QImage img = tmp.value<QImage>();
            QRectF rect = img.rect();
            QPointF dt(rect.width() * 0.5, rect.height() * 0.5);
            metric = object->metric().toMercator();
            for (int i = 0; i < metric.size(); ++i)
                foreach (QPointF p, metric.at(i))
                    painter->drawImage(p - dt, img);
        }
    }
    painter->restore();
}

// -------------------------------------------------------

MapLayerSystem::MapLayerSystem(QObject *parent)
    : MapLayerWithObjects(*new MapLayerSystemPrivate(), parent)
{
    d_ptr->q_ptr = this;
    setObjectName("MapLayerSystem");
    setName("MapLayerSystem");
    setZOrder(1000000);
}

// -------------------------------------------------------

MapLayerSystem::MapLayerSystem(MapLayerSystemPrivate &dd, QObject *parent)
    : MapLayerWithObjects(dd, parent)
{
    d_ptr->q_ptr = this;
    setObjectName("MapLayerSystem");
    setName("MapLayerSystem");
    setZOrder(1000000);
}

// -------------------------------------------------------

MapLayerSystem::~MapLayerSystem()
{
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
