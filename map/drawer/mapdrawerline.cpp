#include <QDebug>
#include <QObject>
#include <QStringList>

#include <QSvgRenderer>
#include <QPainter>

// -------------------------------------------------------

#include "object/mapobject.h"
#include "coord/mapcamera.h"
#include "core/mapmath.h"
#include "drawer/mapdrawercommon.h"

// -----------------------------------------------
#include "drawer/mapdrawer_p.h"
#include "drawer/mapdrawerline.h"

// -------------------------------------------------------

namespace minigis {

// =======================================================

struct LineDrawerSettings
{
    enum LineButtonType { TypeDeafult,
                          TypeDestructed,
                          TypeBackline,
                          TypePath,
                          TypeActionDirection,
                          TypeColumnPos,

                          TypeFullEdge,
                          TypePartEdge,

                          TypeTrench,
                          TypeZoneFlood,
                          TypeZoneBarries,
                          TypeZonePoison,
                          TypeZoneBio,
                          TypeZoneUncleaned,

                          TypeDividingLine,

                          TypeAreaFirePos,
                          TypeLocationEdge,
                          TypeRoute,

                          TypeFrontier
                        };

    QString cls;               //!< классификатор

    qreal lineWidth;           //!< толщина линии
    QColor lineColor;          //!< цвет линии
    Qt::PenStyle lineStyle;    //!< стиль линии

    QColor brushColor;         //!< цвет заливки
    Qt::BrushStyle brushStyle; //!< стиль заливки

    Qt::FillRule fillRule;     //!< способ залвки (работает только для замкнутых объектов)

    bool isClosed;             //!< замкнутый
    bool isSmoothed;           //!< сглаженный

    qreal generalizMin;        //!< нижняя граница генерализации
    qreal generalizMax;        //!< верхняя граница генерализации

    /* ArrowTypes in file: arrows_metric.soe (28.05.2015)
    "Arrow"                qHash(QString::fromUtf8("Arrow"))
    "Square"               qHash(QString::fromUtf8("Square"))
    "Small Arrow"          qHash(QString::fromUtf8("Small Arrow"))
    "Dimension Lines"      qHash(QString::fromUtf8("Dimension Lines"))
    "Double Arrow"         qHash(QString::fromUtf8("Double Arrow"))
    "Rounded short Arrow"  qHash(QString::fromUtf8("Rounded short Arrow"))
    "Symmetric Arrow"      qHash(QString::fromUtf8("Symmetric Arrow"))
    "Line Arrow"           qHash(QString::fromUtf8("Line Arrow"))
    "Rounded large Arrow"  qHash(QString::fromUtf8("Rounded large Arrow"))
    "Circle"               qHash(QString::fromUtf8("Circle"))
    "Square 45"            qHash(QString::fromUtf8("Square 45"))
    "Arrow concave"        qHash(QString::fromUtf8("Arrow concave"))
    "Triangle unfilled"    qHash(QString::fromUtf8("Triangle unfilled"))
    "Diamond unfilled"     qHash(QString::fromUtf8("Diamond unfilled"))
    "Circle unfilled"      qHash(QString::fromUtf8("Circle unfilled"))
    "Square 45 unfilled"   qHash(QString::fromUtf8("Square 45 unfilled"))
    "Square unfilled"      qHash(QString::fromUtf8("Square unfilled"))
    "Half Circle unfilled" qHash(QString::fromUtf8("Half Circle unfilled"))
    "Slash_1_Arrow"        qHash(QString::fromUtf8("Slash_1_Arrow"))
    "Slash_2_Arrow"        qHash(QString::fromUtf8("Slash_2_Arrow"))
    "Slash_3_Arrow"        qHash(QString::fromUtf8("Slash_3_Arrow"))
*/

    uint startArrow;           //!< начало
    uint endArrow;             //!< конец
    bool arrowWidth;           //!< изменять толщину стрелки в соответствии с толщиной линии

    LineButtonType buttonType; //!< тип на кнопке

    QString description;       //!< описание объекта    
};

// =======================================================

class MapDrawerLinePrivate : public MapDrawerPrivate
{
public:
    MapDrawerLinePrivate();
    virtual ~MapDrawerLinePrivate() { }

    QMap<QString, LineDrawerSettings*> settings;
    virtual void init();
};

// =======================================================

MapDrawerLine::MapDrawerLine(QObject *parent)
    : MapDrawer(*new MapDrawerLinePrivate, parent)
{
    setObjectName("MapDrawerLine");
}

// -------------------------------------------------------

MapDrawerLine::~MapDrawerLine()
{

}

// -------------------------------------------------------

MapDrawerLine::MapDrawerLine(MapDrawerPrivate &dd, QObject *parent)
    : MapDrawer(dd, parent)
{
    setObjectName("MapDrawerLine");
}

// -------------------------------------------------------

QStringList MapDrawerLine::exportedClassCodes() const
{
    Q_D(const MapDrawerLine);
    return d->settings.keys();
}

// -------------------------------------------------------

bool MapDrawerLine::init()
{
    return true;
}

// -------------------------------------------------------

bool MapDrawerLine::finish()
{
    return true;
}

// -------------------------------------------------------
void MapDrawerLine::paint(MapObject const *object, QPainter *painter, QRectF const &/*rgn*/, MapCamera const *camera, MapOptions options)
{
    Q_ASSERT(object);
    Q_ASSERT(painter);

    Q_D(MapDrawerLine);

    LineDrawerSettings *set = d->settings.value(object->classCode());
    if (!set)
        return;

    qreal gmin = 0;
    qreal gmax = 0;
    bool chekGen = !object->attribute(attrGenIgnore, false).toBool();
    qreal zoom = camera->zoom();
    CHECKGENERALIZATIONPAINT(object, zoom, gmin, gmax, chekGen);

    bool pix = object->parentObject();
    QTransform tr;
    if (pix) {
        QPointF parent(0, 0);
        qreal ang(0);
        getParentPos(object, camera, parent, ang);
        tr.translate(parent.x(), parent.y()).rotate(ang);
    }

    QVariant var = object->attribute(attrSmoothed);
    bool smoothed = var.isNull() ? set->isSmoothed : var.toBool();

    painter->save();

    QVariant opacity;
    opacity = object->attribute(attrOpacity);

    CHECKSELECTCOLOR(selectColor);
    PENDRAWER(selectColor);
    BRUSHDRAWER(selectColor);
    PAINTERGENERALIZATION(painter, zoom, chekGen, gmin, gmax);

    QVariant varArrow = object->attribute(attrStartArrow);
    uint startArrow = varArrow.isNull() ? set->startArrow : varArrow.toUInt();
    varArrow = object->attribute(attrEndArrow);
    uint endArrow = varArrow.isNull() ? set->endArrow : varArrow.toUInt();
    QByteArray startMetric;
    QByteArray endMetric;
    if (!set->isClosed) {
        startMetric = ArrowType::metric(startArrow);
        endMetric =  ArrowType::metric(endArrow);
    }

    QPainterPath oddPath;
    oddPath.setFillRule(set->fillRule);

    QVector<QPolygonF> metric = object->metric().toMercator();
    for (int l = 0; l < metric.size(); ++l) {
        if (metric.at(l).size() < 2)
            continue;
        QPolygonF path = pix ? tr.map(metric.at(l)) : camera->toScreen().map(metric.at(l));

        QSvgRenderer arrowLeft;
        QSvgRenderer arrowRight;

        QRectF rStart;
        QRectF rEnd;
        QPointF midStart;
        QPointF midEnd;
        QString cls = "object";
        // --------------------- расчет линии в зависимости от стрелок
        if (!startMetric.isEmpty()) {
            QByteArray copy = ArrowType::templateSVG();
            copy.replace("%color%", painter->pen().color().name().toUtf8());
            copy.replace("%metric%", startMetric);
            copy.replace("%width%", QString::number(set->arrowWidth ? set->lineWidth : 0).toUtf8());

            arrowLeft.load(copy);
            QRectF rtmp = ArrowType::bound(startArrow);
            rStart = arrowLeft.boundsOnElement(cls);

            qreal wid = painter->pen().widthF();
//            qreal s = qMax(wid * 2, wid + 20);
            qreal s = (wid + 3) * 2;
            rStart.setSize(rStart.size() * s / rStart.width());
            rtmp.setSize(rtmp.size() * s / rtmp.width());

            midStart = path.first();

            QLineF l(path.first(), path.at(1));
            qreal len = lengthR2(l.p2() - l.p1());
            path.replace(0, len < rtmp.height() ? l.p2() + (l.p1() - l.p2()) / len : l.pointAt((rtmp.height() - wid) / len));
        }
        if (!endMetric.isEmpty()) {
            QByteArray copy = ArrowType::templateSVG();
            copy.replace("%color%", painter->pen().color().name().toUtf8());
            copy.replace("%metric%", endMetric);
            copy.replace("%width%", QString::number(set->arrowWidth ? set->lineWidth : 0).toUtf8());

            arrowRight.load(copy);
            QRectF rtmp = ArrowType::bound(endArrow);
            rEnd = arrowRight.boundsOnElement(cls);

            qreal wid = painter->pen().widthF();
//            qreal s = qMax(wid * 2, wid + 20);
            qreal s = (wid + 3) * 2;
            rEnd.setSize(rEnd.size() * s / rEnd.width());
            rtmp.setSize(rtmp.size() * s / rtmp.width());

            midEnd = path.last();

            int n = path.size();
            QLineF l(path.last(), path.at(n - 2));
            qreal len = lengthR2(l.p2() - l.p1());
            path.replace(n - 1, len < rtmp.height() ? l.p2() + (l.p1() - l.p2()) / len : l.pointAt((rtmp.height() - wid) / len));
        }

        // --------------------- отрисовка линии
        qreal angleStart = 0;
        qreal angleEnd = 0;
        if (!smoothed) {
            set->isClosed ? oddPath.addPolygon(path) : painter->drawPolyline(path);

            angleStart = radToDeg(polarAngle(path.at(1) - path.first()));
            angleEnd   = radToDeg(polarAngle(path.at(path.size() - 2) - path.last()));
        }
        else {
            QPolygonF cubic = simpleCubicSpline(path, set->isClosed);
            if (cubic.size() < 4)
                continue;

            QPainterPath bezie;
            bezie.moveTo(cubic.first());
            for (int i = 1; i < cubic.size() - 2; i+=3)
                bezie.cubicTo(cubic.at(i), cubic.at(i+1), cubic.at(i+2));
            if (set->isClosed) {
                bezie.closeSubpath();
                oddPath.addPath(bezie);
            }
            else
                painter->drawPath(bezie);

            angleStart  = radToDeg(polarAngle(derivativeCubic(cubic.at(0), cubic.at(1), cubic.at(2), cubic.at(3), qreal(0))));
            int n = cubic.size();
            angleEnd = radToDeg(polarAngle(derivativeCubic(cubic.at(n - 1), cubic.at(n - 2), cubic.at(n - 3), cubic.at(n - 4), qreal(0))));
        }

        // --------------------- отрисовка стрелок
        if (!startMetric.isEmpty()) {
            rStart.moveTopLeft(midStart - QPointF(rStart.center().x(), rStart.top()) + rStart.topLeft());

            painter->save();

            painter->translate(midStart);
            painter->rotate(angleStart - 90);
            painter->translate(-midStart);

            arrowLeft.render(painter, cls, rStart);
            painter->restore();
        }
        if (!endMetric.isEmpty()) {
            rEnd.moveTopLeft(midEnd - QPointF(rEnd.center().x(), rEnd.top()) + rEnd.topLeft());

            painter->save();

            painter->translate(midEnd);
            painter->rotate(angleEnd - 90);
            painter->translate(-midEnd);

            arrowRight.render(painter, cls, rEnd);
            painter->restore();
        }
    }
    if (set->isClosed) {
        oddPath.closeSubpath();
        painter->drawPath(oddPath);
    }

    painter->restore();
}

// -------------------------------------------------------

bool MapDrawerLine::hit(MapObject const *object, QRectF const &rgn, const MapCamera *camera, MapOptions options)
{
    Q_D(MapDrawerLine);

    if (isEmpty(object))
        return false;

    LineDrawerSettings *set = d->settings.value(object->classCode());
    if (!set)
        return false;

    qreal zoom = camera->zoom();
    CHECKGENERALIZATIONHIT(object, zoom);

    QTransform tr;
    bool pix = object->parentObject();
    if (pix) {
        QPointF parent(0, 0);
        qreal ang(0);
        getParentPos(object, camera, parent, ang);
        tr.translate(parent.x(), parent.y()).rotate(ang);
    }

    QVariant var = object->attribute(attrSmoothed);
    bool smoothed = var.isNull() ? set->isSmoothed : var.toBool();

    var = object->attribute(attrBrushStyle);
    bool isHollow = set->isClosed && (var.isNull() ? set->brushStyle == Qt::NoBrush : Qt::BrushStyle(var.toInt()) == Qt::NoBrush);

    QPainterPath oddPath;
    oddPath.setFillRule(set->fillRule);

    QVector<QPolygonF> metric = object->metric().toMercator();
    for (int l = 0; l < metric.size(); ++l) {
        if (metric.at(l).size() < 2)
            continue;

        QPolygonF path = pix ? tr.map(metric.at(l))
                             : camera->toScreen().map(metric.at(l));

        if (!smoothed) {
            if (isHollow) {
                if (rectIntersectPolyHollow(rgn, path, set->isClosed))
                    return true;
            }
            else {
                oddPath.addPolygon(path);
//                if (rectIntersectPolyFull(rgn, path))
//                    return true;
            }
        }
        else {
            QPolygonF cubic = simpleCubicSpline(path, set->isClosed);
            if (cubic.size() < 4)
                continue;

            if (!isHollow) {
                QPainterPath cubicPath;
                cubicPath.moveTo(cubic.first());
                for (int i = 1; i < cubic.size() - 2; i += 3)
                    cubicPath.cubicTo(cubic.at(i), cubic.at(i + 1), cubic.at(i + 2));

                cubicPath.closeSubpath();
                oddPath.addPath(cubicPath);
                //                if (cubicPath.intersects(rgn))
                //                    return true;
            }
            else if (rectIntersectCubic(rgn, cubic))
                return true;
        }
    }

    if (!isHollow)
        return oddPath.intersects(rgn);

    return false;
}

QRectF MapDrawerLine::boundRect(MapObject const *object)
{
    if (object->parentObject() || isEmpty(object))
        return QRectF();

    // TODO: multiRect
    QVector<QPolygonF> metric = object->metric().toMercator();

    QRectF r = metric.first().boundingRect().normalized();
    for (int i = 1; i < metric.size(); ++i)
        r = r.united(metric.at(i).boundingRect().normalized());
    return r;
}

QRectF MapDrawerLine::localBound(const MapObject *object, const MapCamera *camera)
{
    Q_D(MapDrawerLine);

    LineDrawerSettings *set = d->settings.value(object->classCode());

    if (!set || isEmpty(object))
        return QRectF();

    QPolygonF path;
    QPointF parent(0, 0);
    QTransform tr;
    bool pix = object->parentObject();
    if (pix) {
        qreal ang(0);
        getParentPos(object, camera, parent, ang);
        tr.translate(parent.x(), parent.y()).rotate(ang);

        path = tr.map(object->metric().toMercator(0));
    }
    else
        path = camera->toScreen().map(object->metric().toMercator(0));

    QVariant var = object->attribute(attrPenWidth);
    qreal dt = var.isNull() ? set->lineWidth : var.toInt();

    QRectF rect = path.boundingRect().normalized();
    return rect.adjusted(-dt, -dt, dt, dt);
}

bool MapDrawerLine::isValid(MapObject const *object)
{
    Q_D(MapDrawerLine);
    return d->settings.value(object->classCode());
}

bool MapDrawerLine::isEmpty(MapObject const *object)
{
    Q_D(MapDrawerLine);
    QVector<QPolygonF> metric = object->metric().toMercator();
    if (metric.size() == 0)
        return true;
    LineDrawerSettings *set = d->settings.value(object->classCode());
    Q_ASSERT(set);
    return metric.first().size() < (set->isClosed ? 3 : 2);
}

Attributes MapDrawerLine::attributes(QString const &cls) const
{
    static Attributes attr_line = Attributes()
            << attrDescriptionObject
            << attrGenMinObject
            << attrGenMaxObject

            << attrPenWidth
            << attrPenColor
            << attrPenStyle
            << attrPenCapStyle
            << attrPenJoinStyle

            << attrOpacity
            << attrSmoothed

            << attrStartArrow
            << attrEndArrow
               ;
    static Attributes attr_area = Attributes()
            << attrDescriptionObject
            << attrGenMinObject
            << attrGenMaxObject

            << attrPenWidth
            << attrPenColor
            << attrPenStyle
            << attrPenCapStyle
            << attrPenJoinStyle

            << attrBrushColor
            << attrBrushStyle

            << attrOpacity
            << attrSmoothed
               ;

    Q_D(const MapDrawerLine);
    LineDrawerSettings *set = d->settings.value(cls);
    if (!set)
        return attr_line;
    return set->isClosed ? attr_area : attr_line;
}

QVariant MapDrawerLine::attribute(const QString &uid, int attr) const
{
    Q_D(const MapDrawerLine);
    LineDrawerSettings *set = d->settings.value(uid);
    if (!set)
        return QVariant();

    switch (attr) {
    case attrDescriptionObject: return set->description;
    case attrGenMinObject:      return set->generalizMin;
    case attrGenMaxObject:      return set->generalizMax;

    case attrPenWidth:          return set->lineWidth;
    case attrPenColor:          return set->lineColor;
    case attrPenStyle:          return static_cast<int>(set->lineStyle);
    case attrPenCapStyle:       return Qt::FlatCap;
    case attrPenJoinStyle:      return Qt::BevelJoin;

    case attrBrushColor:        return set->brushColor;
    case attrBrushStyle:        return static_cast<int>(set->brushStyle);

    case attrOpacity:           return 255;
    case attrSmoothed:          return set->isSmoothed;

    case attrStartArrow:        return set->startArrow;
    case attrEndArrow:          return set->endArrow;

    default: return QVariant();
    }
    return QVariant();
}

QHash<int, QVariant> MapDrawerLine::attributes(const QString &uid, const Attributes &attr) const
{
    Q_D(const MapDrawerLine);
    LineDrawerSettings *set = d->settings.value(uid);
    QHash<int, QVariant> res;
    if (!set)
        return res;
    foreach (int a, attr)
        switch (a) {
        case attrDescriptionObject: res[a] = set->description; break;
        case attrGenMinObject:      res[a] = set->generalizMin; break;
        case attrGenMaxObject:      res[a] = set->generalizMax; break;

        case attrPenWidth:          res[a] = set->lineWidth; break;
        case attrPenColor:          res[a] = set->lineColor; break;
        case attrPenStyle:          res[a] = static_cast<int>(set->lineStyle); break;
        case attrPenCapStyle:       res[a] = Qt::FlatCap; break;
        case attrPenJoinStyle:      res[a] = Qt::BevelJoin; break;

        case attrBrushColor:        res[a] = set->brushColor; break;
        case attrBrushStyle:        res[a] = static_cast<int>(set->brushStyle); break;

        case attrOpacity:           res[a] = 255; break;
        case attrSmoothed:          res[a] = set->isSmoothed; break;

        case attrStartArrow:        res[a] = set->startArrow; break;
        case attrEndArrow:          res[a] = set->endArrow; break;
        default: break;
        }
    return res;
}

QPointF MapDrawerLine::pos(const MapObject *object)
{
    if (isEmpty(object))
        return QPointF();
    return object->metric().toMercator(0).first();
}

qreal MapDrawerLine::angle(const MapObject *object, const MapCamera *camera)
{
    Q_UNUSED(object);
    Q_UNUSED(camera);
    return 0;
}

QVector<QPolygonF> MapDrawerLine::controlPoints(const MapObject *object, const MapCamera *camera)
{
    if (object->metric().size() == 0)
        return QVector<QPolygonF>();
    return QVector<QPolygonF>() << camera->toScreen().map(object->metric().toMercator(0));
}

bool MapDrawerLine::canAddControlPoints(const MapObject *object)
{
    Q_UNUSED(object);
    return true;
}

bool MapDrawerLine::canAddPen(const MapObject *object)
{
    Q_UNUSED(object);
    return true;
}

QVector<QPolygonF> MapDrawerLine::correctMetric(const MapObject *object, QPolygonF poly, int number)
{
    // TODO: chek function
    QVector<QPolygonF> metric = object->metric().toMercator();
    if (metric.size() == 0)
        return QVector<QPolygonF>();
    if (number != 0)
        return QVector<QPolygonF>() << metric.first();
    return QVector<QPolygonF>() << poly;
}

QVector<QPolygonF> MapDrawerLine::fillMetric(const MapObject *object, const MapCamera *camera)
{
    Q_UNUSED(camera);
    return object->metric().toMercator();
}

int MapDrawerLine::whereAddPoint(const MapObject *object)
{
    Q_UNUSED(object);
    return 0;
}

bool MapDrawerLine::canRemovePoint(const MapObject *object, int metricNumber)
{
    QVector<QPolygonF> metric = object->metric().toMercator();
    if (metricNumber != 0)
        return false;
    if (metric.size() == 0)
        return false;
    if (metric.first().size() > 2)
        return true;
    return false;
}

bool MapDrawerLine::canUseMarkMode()
{
    return true;
}

int MapDrawerLine::markerType()
{
    return 3;
}

bool MapDrawerLine::isClosed(const QString &cls)
{
    Q_D(MapDrawerLine);
    LineDrawerSettings *set = d->settings.value(cls);
    if (!set)
        return false;
    return set->isClosed;
}

void MapDrawerLine::renderImg(QPainter &painter, const QString &cls, QRectF rgn, int type, bool flag, QPolygonF metric)
{
    Q_UNUSED(flag);
    Q_UNUSED(metric);
    Q_D(MapDrawerLine);
    if (rgn.isEmpty())
        return;
    LineDrawerSettings *set = d->settings.value(cls);
    if (!set)
        return;

    MapCamera camera;
    camera.moveBy(QPointF(-rgn.bottom(), rgn.left()));

    MapObject mo;
    mo.setClassCode(cls);
    if (type == 1)
        mo.setSelected(true);
    else if (type == 2)
        mo.setHighlighted(MapObject::HighligtedHigh);
    else if (type == 3)
        mo.setHighlighted(MapObject::HighligtedLow);

#ifndef DRAWERFIXEDWIDTH
    int lineWidth = set->lineWidth;
#else
    int lineWidth = 2;
#endif
    mo.setAttribute(attrPenWidth, lineWidth);

    qreal dt = lineWidth;
    QPointF mid(rgn.height(), rgn.width());
    QPolygonF poly;
    poly << QPointF(mid.x() * 0.1, mid.y() * 0.1 + dt) << QPointF(mid.x() * 0.9 - dt, mid.y() * 0.1 + dt) << QPointF(mid.x() * 0.1, mid.y() * 0.9 - dt);
    mo.setMetric(Metric(poly, Mercator));
    paint(&mo, &painter, QRectF(), &camera);
}

// -------------------------------------------------------
// -------------------------------------------------------

MapDrawerLinePrivate::MapDrawerLinePrivate()
    :MapDrawerPrivate()
{
    init();
}

void MapDrawerLinePrivate::init()
{
    static const int MAXGEN = 10000000;

    static LineDrawerSettings _line[] =
    {
        {"{00000000-0000-409d-9621-f4899f6a4c6e}", 2, Qt::black,         Qt::NoPen, Qt::black,                  Qt::NoBrush,      Qt::WindingFill, false, true,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Вспомогательная линия для хелпера")},

        {"{00000000-0001-409d-9621-f4899f6a4c6e}", 2, Qt::black,     Qt::SolidLine, Qt::black,                  Qt::NoBrush,      Qt::WindingFill, false, true,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Перо")},
        {"{00000000-0002-409d-9621-f4899f6a4c6e}", 2, Qt::black,     Qt::SolidLine, QColor(128, 128, 128, 200), Qt::SolidPattern, Qt::WindingFill, true,  true,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Область")},

        {"{00000000-6cb3-47f9-98a2-d6ae1d3a4540}", 3, Qt::red,       Qt::SolidLine, Qt::black,                  Qt::NoBrush,      Qt::WindingFill, false, false, 0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Линейка")},
        {"{00000000-930a-4ed8-af31-e7fe69bf1067}", 2, Qt::red,       Qt::SolidLine, Qt::red,                    Qt::BDiagPattern, Qt::WindingFill, true,  false, 0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Площадь")},
        {"{00000000-cdb8-5cff-5318-b419e9f48fc0}", 1, Qt::red,       Qt::SolidLine, QColor(255, 255, 0, 50),    Qt::SolidPattern, Qt::WindingFill, true,  false, 0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Область видимости")},
        {"{00000000-e9c0-f479-ac28-803495efce77}", 5, Qt::gray,      Qt::SolidLine, Qt::transparent,            Qt::NoBrush,      Qt::WindingFill, false, false, 0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Линия маршрута")},

        {"{00000000-34c4-3677-ed3f-8105597d3620}", 1, QColor( 10,  11,  93, 220), Qt::SolidLine, QColor(123, 237, 205, 120), Qt::SolidPattern, Qt::OddEvenFill, true, false,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Федеральный заповедник")},
        {"{00000000-5827-cf3a-f880-cacc75c4051d}", 1, QColor(100,   0, 100, 220), Qt::SolidLine, QColor(172, 223, 113, 120), Qt::SolidPattern, Qt::OddEvenFill, true, false,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Национальный парк")},
        {"{00000000-e9c0-8de8-970e-d3c17ec9ce6f}", 1, QColor(  0,  70,   0, 220), Qt::SolidLine, QColor(172, 223, 113, 120), Qt::SolidPattern, Qt::OddEvenFill, true, false,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Заповедник")},

    #ifdef TACTICSIGNS
        // Уничтоженные объекты
        {"{a9e45039-c16e-44d8-8b91-7614ebbc52bc}", 3,  Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeDestructed, QString::fromUtf8("Поврежденный объект площадной (свои, факт)")},
        {"{acfb1bdd-9d1e-41e4-aa3e-a7ef99346f21}", 3,  Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeDestructed, QString::fromUtf8("Уничтоженный объект площадной (свои, факт)")},
        {"{3284e751-bdf5-44a8-a5b7-1aef1f2f4d52}", 3, Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeDestructed, QString::fromUtf8("Поврежденный объект площадной (пр-к, факт)")},
        {"{bd8ee3ec-4874-4c1b-a2f5-54a671765faf}", 3, Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeDestructed, QString::fromUtf8("Уничтоженный объект площадной (пр-к, факт)")},

        // Тыльные линии
        {"{016f0ae8-08ab-4c5a-b8bb-1992ce35664e}", 2,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,   50000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП взвода (свои, факт)")},
        {"{a84cb067-3aa1-45ca-8df5-6d656120b5c8}", 2, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,   50000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП взвода рода войск (свои, факт)")},
        {"{f5a36782-4907-4ad6-b8a7-02144c030321}", 3,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП роты (свои, факт)")},
        {"{935e241c-1e8d-4cf2-9f66-35a365e072ab}", 3, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП роты рода войск (свои, факт)")},
        {"{718327a5-2d60-46a0-a34b-43dde7ca66be}", 4,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  500000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП б-на (свои, факт)")},
        {"{0208cd71-d5d8-421c-b116-3b227c7b1f5c}", 5,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП полка (свои, факт)")},
        {"{9ff16281-0ffb-45fa-8931-a550954a9675}", 5,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП бригады (свои, факт)")},
        {"{e014207d-eb00-45bf-bc20-ed4234db32f4}", 6,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП дивизии (свои, факт)")},
        {"{29ed2803-5af4-48aa-afd4-4caec6ef60a2}", 3,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП взвода (пр-к, факт)")},
        {"{2f5194c9-fa54-483b-a287-e9defdbde14b}", 3,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП роты (пр-к, факт)")},
        {"{2e958b58-cd94-4938-abdb-f5a507d37613}", 4,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  500000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП б-на (пр-к, факт)")},
        {"{f481f660-4df5-4672-a957-815f226b2d22}", 5,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП полка (пр-к, факт)")},
        {"{fb382b7d-a532-46ae-95e8-c490c95814c1}", 5,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БП бригады (пр-к, факт)")},
        {"{cae97954-9601-4f64-ace6-9832b13732ad}", 6,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия БПдивизии (пр-к, факт)")},
        {"{24dd68d8-2c94-4ff7-8a32-710b98eb7540}", 3,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта ВФ (пр-к, план)")},
        {"{acfe857e-a075-4be0-bcf5-0e2c2416f7fb}", 2,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,   50000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта ВФ уровня взвод (пр-к, план)")},
        {"{c5920761-3865-47ba-8658-f3bf0023b789}", 3,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта ВФ уровня рота (пр-к, план)")},
        {"{1bf9e8c5-24c5-44c9-9da6-837515cdacaa}", 4,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  500000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ уровня батальон (пр-к, план)")},
        {"{6cb55b79-172d-4657-920e-b08969d7957a}", 5,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ уровня полк (пр-к, план)")},
        {"{21545036-9a53-4417-8bb7-2d87ba86ed46}", 6,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ уровня бригада (пр-к, план)")},
        {"{e5e8cd9e-72ed-4cb0-823c-3eee1df8fa3d}", 7,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ уровня дивизия (пр-к, план)")},
        {"{a5f08cf2-6098-4e60-b600-cbd317c29596}", 3,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта общевойскового ВФ (свои, план)")},
        {"{003e2a06-cc64-4a40-a6ba-f16e0c415910}", 2,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,   50000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта общевойскового ВФ уровня взвод (свои, план)")},
        {"{72b80d47-3966-4099-a314-c928823612cf}", 3,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта общевойскового ВФ уровня рота (свои, план)")},
        {"{8bf646f7-03b4-4481-8353-152727308f70}", 4,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  500000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны общевойскового ВФ уровня батальон (свои, план)")},
        {"{64c4833b-0921-48d6-892e-0a80aa69920a}", 5,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны общевойскового ВФ уровня полк (свои, план)")},
        {"{52092871-1d80-4797-946f-35e2b9a5162a}", 6,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны общевойскового ВФ уровня бригада (свои, план)")},
        {"{0996da96-bff4-43b2-9316-49aeaaba15a9}", 7,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны общевойскового ВФ уровня дивизия (свои, план)")},
        {"{ed7b3f16-cf76-4318-bab9-627befc64159}", 3, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта ВФ рода войск (свои, план)")},
        {"{c997eaca-9d58-47d3-9988-4a654d2f7ec1}", 2, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,   50000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта ВФ рода войск уровня взвод (свои, план)")},
        {"{29265fc8-8450-443d-9adb-473db8544316}", 3, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  200000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия опорного пункта ВФ рода войск уровня рота (свои, план)")},
        {"{926c8a6a-af40-4cc0-80ac-840753779058}", 4, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  500000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ рода войск уровня батальон (свои, план)")},
        {"{15e2c12a-2277-4a3d-9007-240e0289ff1f}", 5, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ рода войск уровня полк (свои, план)")},
        {"{771ca75d-5d57-4965-abe2-228faf330c9d}", 6, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ рода войск уровня бригада (свои, план)")},
        {"{3b89bdd9-3266-44cf-b213-13c1a9a42a92}", 7, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeBackline, QString::fromUtf8("Тыльная линия района обороны ВФ рода войск уровня дивизия (свои, план)")},

        // Пути
        {"{24ac7730-b556-4769-94a0-19cb9a6b1e4b}", 3,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра общевойскового СВБ (свои, план)")},
        {"{bc0b621d-8d31-4bef-91c0-d912726f66d0}", 2,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра общевойскового ВФ уровня отделение (свои, план)")},
        {"{e864d8cd-99b8-4af6-b1e7-e5a27f987933}", 3,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра общевойскового ВФ уровня взвод (свои, план)")},
        {"{eb455f36-eb2e-4941-9947-ef73867df8c6}", 4,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра общевойскового ВФ уровня рота (свои, план)")},
        {"{220b18ff-6055-40fc-b163-1ac0c67b903c}", 5,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 500000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра общевойскового ВФ уровня батальон (свои, план)")},
        {"{c12415a6-0e52-4b88-a0b7-cb4421de1975}", 3, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра СВБ рода войск (свои, план)")},
        {"{fcecb979-2928-44d8-aff4-6fc474e91364}", 2, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ рода войск уровня отделение (свои, план)")},
        {"{911f3b86-1adc-49a1-bdea-46e495339faa}", 3, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ рода войск уровня взвод (свои, план)")},
        {"{b586cf70-a2a9-4ec8-afb1-80386d180233}", 5, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ рода войск уровня рота (свои, план)")},
        {"{efa22e12-5bdf-47a4-ab60-84187380b0aa}", 6, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 500000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ рода войск уровня батальон (свои, план)")},
        {"{53cdb8be-c55d-4af9-b8fb-79b7ad601f76}", 3,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра  СВБ (пр-к, план)")},
        {"{126a16cc-43a2-4206-9a66-0899d915a18c}", 2,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ уровня отделение (пр-к, план)")},
        {"{7043f573-d5c6-4623-807a-c81822c50c05}", 3,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0,  50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ уровня взвод (пр-к, план)")},
        {"{1a21f66f-dcef-46de-a5a9-cdd010f476f6}", 5,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ уровня рота (пр-к, план)")},
        {"{cf72a188-aa21-4379-90d6-4c6b159254c3}", 6,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 500000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypePath, QString::fromUtf8("Путь маневра ВФ уровня батальон (пр-к, план)")},

        {"{df290b1b-0627-432c-b481-7bd1276c8278}", 3, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 50000, 0, 0, false, LineDrawerSettings::TypePath, QString::fromUtf8("Колонный путь (свои, факт)")},
        {"{08a3349f-be7d-455b-960d-0882e862cedb}", 3, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 50000, 0, 0, false, LineDrawerSettings::TypePath, QString::fromUtf8("Колонный путь (свои, план)")},
        {"{407ef4b7-1b3a-46f4-8ca9-7d2f462ebab5}", 3,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 50000, 0, 0, false, LineDrawerSettings::TypePath, QString::fromUtf8("Колонный путь (пр-к, факт)")},
        {"{ea0c00f8-a12b-4759-bbad-f1b804beb666}", 3,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, 50000, 0, 0, false, LineDrawerSettings::TypePath, QString::fromUtf8("Колонный путь (пр-к, план)")},

        {"{7d7599f8-a7ef-4bb9-8918-2f0356efad37}", 7, QColor(150, 75, 0), Qt::SolidLine, Qt::black, Qt::NoBrush, Qt::WindingFill, false, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypePath, QString::fromUtf8("Маршрут движения")},
        {"{b68070a9-6922-492c-a1e0-ce8702b506ba}", 3,           Qt::blue,  Qt::DashLine,   Qt::red, Qt::NoBrush, Qt::WindingFill, false, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypePath, QString::fromUtf8("Маршрут движения наземных сил и средств при совершении марша (пр-к, план)")},

        // Направления действий
        {"{396995f2-c01f-4c72-a639-d2f59761ee81}",  4, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Основное направление стрельбы подразделения РВ и А")},
        {"{b0b0cb1c-9cdc-4cc1-88f7-7f90c6fc07af}",  2,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового СВБ (свои, план)")},
        {"{13e6ef72-4d8b-4a88-866c-9bb8137c312d}",  2,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня отделение (свои, план)")},
        {"{f179be5b-d93a-4d68-934f-63595ea41eea}",  3,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня взвод (свои, план)")},
        {"{81043562-f0c7-48ad-8967-b4c105ed4c9e}",  4,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня рота (свои, план)")},
        {"{0de42aad-3b9e-4d79-bced-794d3b87aa5f}",  5,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня батальон (свои, план)")},
        {"{50907d97-7202-42e4-95be-0393a1dd6597}",  6,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня полк (свои, план)")},
        {"{1782ce00-1fb2-42b8-81a1-6cf31acc4963}",  7,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня бригада (свои, план)")},
        {"{98950b99-5a9c-4443-9551-487ef531dbe7}",  8,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня дивизия (свои, план)")},
        {"{0a34fd88-0ca3-4040-8a52-1f4acc1fedb2}",  9,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 3000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового ВФ уровня армия (АК) (свои, план)")},
        {"{c404ad51-1f2d-4b5d-bf97-81d149b2e1ee}", 10,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 5000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий общевойскового оперативно-стратегического (стратегического) ВФ (свои, план)")},
        {"{3b7a1a75-af57-4ba3-9b56-04d29a6157b8}",  2, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий СВБ рода войск (свои, план)")},
        {"{5eb97121-0c37-4427-87fd-8ac658fcdd99}",  2, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня отделение (свои, план)")},
        {"{a63c5ea9-8493-4dda-9012-91a1ab684607}",  3, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня взвод (свои, план)")},
        {"{6fb9e608-65d6-4ab3-b64e-6c1c3f799ece}",  4, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня рота (свои, план)")},
        {"{d61c2ee1-bb65-41bf-a9b2-fba936475d26}",  5, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня батальон (свои, план)")},
        {"{d75094a6-fe49-4089-95ae-7e35459468da}",  6, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня полк (свои, план)")},
        {"{2d521783-0b87-4ed5-8940-813190a0d9d2}",  7, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня бригада (свои, план)")},
        {"{1f7fa16d-3f93-4d05-b4b4-e5e1a8544be6}",  8, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня дивизия (свои, план)")},
        {"{2156b0d7-b4f7-4fce-8e2b-be7315ceb936}",  9, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 3000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ рода войск уровня армия (АК) (свои, план)")},
        {"{b9627e5d-99a3-43db-b00b-0ed1887899f5}",  4,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Основное направление стрельбы подразделения РВ и А (пр-к, план)")},
        {"{3adfe70d-b5bf-4b7c-8e41-19d9f91fabf1}",  2,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий СВБ (пр-к, план)")},
        {"{b4b791fa-c8f5-4797-9e65-28b32bd09861}",  2,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   25000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня отделение (пр-к, план)")},
        {"{120bf6f5-4b2d-46f7-a6a6-328029963c2b}",  3,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,   50000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня взвод (пр-к, план)")},
        {"{89ee38a5-e6ed-461c-8c26-3bb0810e113d}",  4,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  200000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня рота (пр-к, план)")},
        {"{5434ebd9-c2ee-4e02-b655-520e68172749}",  5,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня батальон (пр-к, план)")},
        {"{7cd94d4e-bba2-4866-a99c-c157ffe2acc8}",  6,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня полк (пр-к, план)")},
        {"{65c601bd-ec58-40a7-93eb-a60a28b66d6a}",  7,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня бригада (пр-к, план)")},
        {"{ddda9e03-b2c2-4c7d-b8bd-6227085f8636}",  8,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня дивизия (пр-к, план)")},
        {"{73ed1216-058c-431c-834a-9924793439de}",  9,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 3000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий ВФ уровня армия (АК) (пр-к, план)")},
        {"{71cd99cb-7d5a-49ca-903c-d0e356310a2d}", 10,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 5000000, 0, qHash(QString::fromUtf8("Arrow")), false, LineDrawerSettings::TypeActionDirection, QString::fromUtf8("Направление действий оперативно-стратегического ВФ (пр-к, план)")},

        // Зоны ответственности
        {"{00bc05bb-0c64-41db-bc1f-1e6355a187a2}",  2,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   25000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового СВБ (свои, план)")},
        {"{cb22e548-9e90-48ff-8df0-62e99c8a1814}",  2,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   25000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня отделение (свои, план)")},
        {"{2a10af2e-4dcc-4fdb-bb77-c82e37794852}",  3,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   50000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня взвод (свои, план)")},
        {"{c80b3502-d416-4025-853b-11a49a2a6195}",  4,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,  200000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня рота (свои, план)")},
        {"{d03facff-812f-47cc-9756-9f70d978c240}",  5,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня батальон (свои, план)")},
        {"{fefebc44-1584-4838-8ed1-37678ca98f57}",  6,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня полк (свои, план)")},
        {"{88f884b6-a549-4493-ba4b-38ef6450b66c}",  7,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня бригада (свои, план)")},
        {"{72abe48f-936c-4fdb-8491-343025333bf7}",  8,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня дивизия (свои, план)")},
        {"{623c3f3d-893f-4662-98d6-a3d866d5ac50}",  9,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 3000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового ВФ уровня армия (АК) (свои, план)")},
        {"{e24eee3b-e417-44f7-9a01-1be78f23a955}", 10,   Qt::red, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 5000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности общевойскового оперативно-стратегического (стратегического) ВФ (свои, план)")},
        {"{4a04c34c-555f-4b77-a3af-4b931c3de9be}",  2, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   25000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности СВБ рода войск (свои, план)")},
        {"{a39f5ef9-7848-40e8-a822-a55a55113082}",  2, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   25000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня отделение (свои, план)")},
        {"{13231a3b-0b6e-4f83-82fe-635531b0129d}",  3, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   50000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня взвод (свои, план)")},
        {"{da767c07-a82f-4887-b197-34ba8b566a83}",  4, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,  200000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня рота (свои, план)")},
        {"{31c370e7-4511-4578-baae-32bca8b15ef8}",  5, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня батальон (свои, план)")},
        {"{57e5bb82-7752-46e6-8466-686ceebf61c7}",  6, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня полковник (свои, план)")},
        {"{e7e921da-85b6-42b8-b2c5-4ea56f64f88a}",  7, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня бригада (свои, план)")},
        {"{512b1cb5-a11d-46a9-9ef2-9efaad07f530}",  8, Qt::black, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ рода войск уровня дивизия (свои, план)")},
        {"{d5cfd6a8-9187-466a-b34f-3303c4f4bee2}",  2,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   25000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности СВБ (пр-к, план)")},
        {"{5daaefee-03b3-4215-bf06-4527d6a107a4}",  2,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   25000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня отделение (пр-к, план)")},
        {"{634c9544-47ad-4a21-8a48-bf39d4882928}",  3,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,   50000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня взвод (пр-к, план)")},
        {"{ce2189a4-fdd5-4eef-a0f8-3d5ca844c590}",  4,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,  200000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня рота (пр-к, план)")},
        {"{1a89bd63-84cc-49e1-a3b3-c0f3e1f15c40}",  5,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня батальон (пр-к, план)")},
        {"{001e9032-eaa1-4fe8-a4e9-1dcdf4e3d3ff}",  6,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня полковник (пр-к, план)")},
        {"{42974303-2303-4641-9a41-cab9d56c8eb9}",  7,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня бригада (пр-к, план)")},
        {"{9d719b1a-9387-4502-8975-89ce4b26c6a7}",  8,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня дивизия (пр-к, план)")},
        {"{3cefbca5-971c-4ad2-afb6-b945c709aa24}",  9,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 3000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности ВФ уровня армия (АК) (пр-к, план)")},
        {"{3bdef26c-3c2f-4aa7-b7ba-4b9d050a2710}", 10,  Qt::blue, Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, true, false, 0, 5000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Зона ответственности оперативно-стратегического (стратегического) ВФ (пр-к, план)")},
        // Разграничительные линии
        {"{196afbd1-7345-4031-adc9-2f1c905aedb6}", 5,  Qt::red,    Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Разграничительная линия между батальонами (свои, план)")},
        {"{eb7c3c82-fd83-4cc8-9638-40b63db073bb}", 6,  Qt::red, Qt::DashDotLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Разграничительная линия между бригадами (свои, план)")},
        {"{132d30f2-3c26-4f24-be58-7334a90aa479}", 7,  Qt::red,   Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Разграничительная линия между дивизиями (свои, план)")},
        {"{fcb1241d-55ad-4c45-941d-604ecf48a665}", 5, Qt::blue,    Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Разграничительная линия между батальонами (пр-к, план)")},
        {"{167fe394-131c-4995-8b7d-580eaa153a07}", 6, Qt::blue, Qt::DashDotLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0,  500000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Разграничительная линия между бригадами (пр-к, план)")},
        {"{5359b24b-f8a3-49ca-9779-79995b1e2f07}", 7, Qt::blue,   Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 1000000, 0, 0, false, LineDrawerSettings::TypeDividingLine, QString::fromUtf8("Разграничительная линия между дивизиями (пр-к, план)")},

        // Местоположения колонн
        {"{57b2b800-43b4-4379-9489-05c87d7ac809}", 2,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) отд-я (свои факт)")},
        {"{01c7c160-6b79-478b-8f17-cd48a2078536}", 3,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, qHash(QString::fromUtf8("Slash_1_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) взвода (свои факт)")},
        {"{6936f538-42e6-41ff-87a1-771c4dd766bd}", 4,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, qHash(QString::fromUtf8("Slash_2_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) роты (свои факт)")},
        {"{79aa8449-bb84-485c-86cc-74459a40fda5}", 5,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, qHash(QString::fromUtf8("Slash_3_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) б-на (свои факт)")},
        {"{d341aa46-4c99-404f-9bba-e298b20a6ae1}", 6,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) полка (свои факт)")},
        {"{16fac351-fbb8-4e44-9d78-54f51e3b1982}", 7,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) бригады (свои факт)")},
        {"{dd86e796-33df-409d-9980-4447d79fbf5f}", 8,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) дивизии (свои факт)")},
        {"{43d8c8a8-632c-4057-a122-0a5da4e7a59f}", 2, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) отд-я рода войск (свои факт)")},
        {"{bb3e3c06-95b3-4c5e-93ae-461477d735dd}", 3, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, qHash(QString::fromUtf8("Slash_1_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) взвода рода войск (свои факт)")},
        {"{345b7f8a-4a89-4911-aa0c-d6264d55c880}", 4, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, qHash(QString::fromUtf8("Slash_2_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ) батареи (роты рода войск) (свои факт)")},
        {"{af6d3a91-13e0-424e-b380-87160a885ff0}", 5, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, qHash(QString::fromUtf8("Slash_3_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ) дивизиона (б-на рода войск) (свои факт)")},
        {"{d07bde95-da6d-40d0-8436-f12f6fb13234}", 6, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) полка рода войск (свои факт)")},
        {"{8bf4be11-2f84-4fdb-ba58-79a739ef3b66}", 7, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) бригады рода войск (свои факт)")},
        {"{c7e9990a-afea-4906-ad31-74611af098f0}", 8, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) дивизии рода войск (свои факт)")},
        {"{60eb9650-82f3-4cc8-8103-127e31fc192a}", 2,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) отд-я (свои план)")},
        {"{31712259-70aa-40cf-ac3e-f5088a2f76f8}", 3,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, qHash(QString::fromUtf8("Slash_1_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) взвода (свои план)")},
        {"{fa74eacc-a5b2-4a76-9151-0e21d8024cf2}", 4,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, qHash(QString::fromUtf8("Slash_2_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) роты (свои план)")},
        {"{1fda23bb-8a4e-408e-96e5-45a1bae30613}", 5,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, qHash(QString::fromUtf8("Slash_3_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) б-на (свои план)")},
        {"{bb736707-356c-4c79-a700-9df9fec323d8}", 6,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) полка (свои план)")},
        {"{64096de3-1be2-41b2-ab67-15dcc94d9a50}", 7,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) бригады (свои план)")},
        {"{0373b2ab-df7d-4739-a055-a7d7f45a8898}", 8,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) дивизии (свои план)")},
        {"{318cc1a0-1e77-4fff-acd3-856290090638}", 2, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) отд-я рода войск (свои план)")},
        {"{ef2c905f-4759-463a-900a-34d4bf517989}", 3, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, qHash(QString::fromUtf8("Slash_1_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) взвода рода войск (свои план)")},
        {"{c89b1cb3-ac73-48d4-a6f6-13c1a6029a6c}", 4, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, qHash(QString::fromUtf8("Slash_2_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ) батареи (роты рода войск) (свои план)")},
        {"{c4358a6c-d1c6-4a42-83f7-9c02fff3c7ad}", 5, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, qHash(QString::fromUtf8("Slash_3_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ) дивизиона (б-на рода войск) (свои план)")},
        {"{13bf59a0-755a-4ecb-8b01-8578e415832e}", 6, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) полка рода войск (свои план)")},
        {"{ab5cfe0d-bdd0-4ed7-a3fb-80e9ed3e0418}", 7, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) бригады рода войск (свои план)")},
        {"{9b9fb20a-ac16-40f8-ad7f-19b56a943802}", 8, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общее обозначение) дивизии рода войск (свои план)")},
        {"{30c8aff2-ffa2-42e9-a392-8162d0fbbc96}", 2,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня отделение (пр-к факт)")},
        {"{337e91bf-ee61-4bc0-a49b-3e94662dbf53}", 3,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, qHash(QString::fromUtf8("Slash_1_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня взвод (пр-к факт)")},
        {"{e67dfbf3-1bf4-4389-a001-f3cf3f8f5e19}", 4,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, qHash(QString::fromUtf8("Slash_2_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня рота (пр-к факт)")},
        {"{bda3c1a6-4b31-4060-998b-0ed1edc2d47b}", 5,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, qHash(QString::fromUtf8("Slash_3_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня батальон (пр-к факт)")},
        {"{1c819034-b319-451f-9abf-0dd98fe2f244}", 6,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня полк (пр-к факт)")},
        {"{1ad53ead-440d-433c-94ff-9f5593ac35d6}", 7,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня бригада (пр-к факт)")},
        {"{4549b844-8a3e-47e7-b931-c62cd1a8a155}", 8,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня дивизия (пр-к факт)")},
        {"{1682acb4-d96c-4938-80ac-6047614783ea}", 2,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня отделение (пр-к план)")},
        {"{3208d7d0-f22c-44fb-b403-28c08be6eebf}", 3,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, qHash(QString::fromUtf8("Slash_1_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня взвод (пр-к план)")},
        {"{fdf532ae-74f5-4d74-9fe9-1685a0e1f540}", 4,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, qHash(QString::fromUtf8("Slash_2_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня рота (пр-к план)")},
        {"{4c46a0fa-13c5-4d85-a4a4-4de446e0274a}", 5,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, qHash(QString::fromUtf8("Slash_3_Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня батальон (пр-к план)")},
        {"{f4faf176-5996-4c79-a341-5c20606deda9}", 6,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня полк (пр-к план)")},
        {"{2cde59ea-e8bd-499b-983c-c6c638ebb354}", 7,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня бригада (пр-к план)")},
        {"{6347d8df-fa38-45a6-acb0-9ebeb8adfca5}", 8,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000,   qHash(QString::fromUtf8("Arrow")), 0, true, LineDrawerSettings::TypeColumnPos, QString::fromUtf8("Местоположение колонны (общ)  ВФ уровня дивизия (пр-к план)")},

        // Районы расположения (Полная граница района)
        {"{89119672-eebd-4504-843a-dad6cdd188d4}",  2,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня отделение (свои факт)")},
        {"{24421c8c-114f-40c4-a870-7619ceab19cf}",  3,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня взвод (свои факт)")},
        {"{23763427-4902-45fd-ab47-294ee690ee78}",  4,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня рота (свои факт)")},
        {"{3764861c-9893-489a-b77e-ddf1c41818c3}",  5,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня батальон (свои факт)")},
        {"{b2ea3c01-9a3c-400b-9d3e-26c4f66ecfef}",  6,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня полк (свои факт)")},
        {"{9a91d8af-3722-4d61-bc40-5e684117c08b}",  7,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня бригада (свои факт)")},
        {"{3cfd7a4c-2645-4a5b-b09c-ff9dd52f23b9}",  8,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня дивизия (свои факт)")},
        {"{d3110ff4-5ad3-471a-92fe-3c3ff75f9a15}",  9,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня армия (АК) (свои факт)")},
        {"{9f7e92cf-961d-4b30-83b0-3c94536a9db3}", 10,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района оперативно-стратегического (стратегического) воинского формирования (свои, факт)")},
        {"{8456ed3c-4b36-4fc0-9181-3167246ab07f}",  2, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня отделение (свои факт)")},
        {"{61f7cca7-5bf3-4f17-84a9-f6965ac577fc}",  3, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня взвод (свои факт)")},
        {"{2c0d52be-c5c8-4bcb-bbfd-0be4fd24b2b3}",  4, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня рота (свои факт)")},
        {"{b8e606ce-7cb4-4cc9-b0e9-86b542d3671d}",  5, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня батальон (свои факт)")},
        {"{762b995e-ec51-4871-80bc-c1d309c577f7}",  6, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня полк (свои факт)")},
        {"{53304014-d95e-456f-9841-39aba04bad14}",  7, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня бригада (свои факт)")},
        {"{4b3a26da-e627-43a4-b36e-eb3a39b8f23d}",  8, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня дивизия (свои факт)")},
        {"{f3678eae-9dd3-4a06-a820-1ff12d50eb17}",  2,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня отделение (свои план)")},
        {"{41f47098-4407-4437-bf37-0c24a3a577a2}",  3,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня взвод (свои план)")},
        {"{cb786997-baf9-4aac-a7a5-c8b2d2eb082d}",  4,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня рота (свои план)")},
        {"{c69c7a7c-9728-4636-9251-de477bcf8d99}",  5,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня батальон (свои план)")},
        {"{db1a7f73-5b8d-476b-84a0-50a5cc6331f3}",  6,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня полк (свои план)")},
        {"{b7838f72-a4ce-4632-bb05-59ed31105cd3}",  7,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня бригада (свои план)")},
        {"{532636ee-5750-41da-bbed-aae9c781c689}",  8,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня дивизия (свои план)")},
        {"{cd0c0704-7c46-4fb9-aa20-5b010b3cdc80}",  9,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения общевойскового ВФ уровня армия (АК) (свои план)")},
        {"{9cab48ec-414c-47dd-84d6-46549b61a7cf}", 10,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района оперативно-стратегического (стратегического) воинского формирования (свои, план)")},
        {"{d8e5fc2e-fd15-493f-97b8-6036cfd01bbe}",  2, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня отделение (свои план)")},
        {"{676a5999-3d1a-451c-8c5c-8a889b970c1d}",  3, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня взвод (свои план)")},
        {"{24ab5935-e39c-4b77-90fa-520858e2ffd5}",  4, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня рота (свои план)")},
        {"{0c3fd312-e6a9-4fda-b0dc-c32967a2aa78}",  5, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня батальон (свои план)")},
        {"{cda9667c-4db3-41dd-88c5-a964984422f9}",  6, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня полк (свои план)")},
        {"{fae36357-fb11-4f8c-b47c-2fe11339fa48}",  7, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня бригада (свои план)")},
        {"{9c8d7f08-2e3f-43d2-88f5-3bbacca0450e}",  8, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ рода войск уровня дивизия (свои план)")},
        {"{dc1ebb03-925c-46fc-bd0d-333542c528b1}",  2,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня отделение (противник факт)")},
        {"{1ee8b96c-8349-4ad0-b50e-0f0628c76bf0}",  3,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня взвод (противник факт)")},
        {"{3f801d86-cebb-44f9-bd8b-d4cfbff7d7b5}",  4,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня рота (противник факт)")},
        {"{bad5fe1e-824e-4bbe-9669-7ce1c2650c56}",  5,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня батальон (противник факт)")},
        {"{6ab208a5-fbe3-42ee-93ea-3b5f4bb3daa3}",  6,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня полк (противник факт)")},
        {"{1a1b0fed-d8d0-4c8d-9fd9-9101a9c3bc53}",  7,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня бригада (противник факт)")},
        {"{dd106bfe-d04c-471c-a5ac-47ac08d83060}",  8,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня дивизия (противник факт)")},
        {"{c53b1f01-6828-4946-bab9-d5666f984ef5}",  9,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня армия (АК) (противник факт)")},
        {"{140f6d17-75b2-43eb-a628-dbebf64be767}", 10,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района оперативно-стратегического (стратегического) воинского формирования (противник, факт)")},
        {"{3d1b073e-b8b6-4182-a485-30df812334c5}",  2,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня отделение (противник план)")},
        {"{ad320039-facd-434a-b019-358932323c32}",  3,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня взвод (противник план)")},
        {"{d5857947-8ac5-47aa-b61a-3ca636a11ce2}",  4,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня рота (противник план)")},
        {"{a57c8243-38c1-4365-9368-678900278c97}",  5,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня батальон (противник план)")},
        {"{2514dfca-f624-4bcf-bc10-3397f4bd2bf1}",  6,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня полк (противник план)")},
        {"{c3c06337-ced3-4859-b3d6-32614bfb9b1a}",  7,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня бригада (противник план)")},
        {"{946ad018-d130-4bf9-866c-88c272056486}",  8,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня дивизия (противник план)")},
        {"{7c33c481-b3a7-4eb3-884f-2e38b11d4cc6}",  9,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района расположения ВФ уровня армия (АК) (противник план)")},
        {"{d4717236-60fa-43ce-8c18-8066720759ba}", 10,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, true, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypeFullEdge, QString::fromUtf8("Полная граница района оперативно-стратегического (стратегического) воинского формирования (противник, план)")},

        // Районы расположения (часть границы)
        {"{ea90d0d3-0110-40a9-90cf-0d4379474a70}",  2,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня отделение (свои факт)")},
        {"{410290d0-08e4-4dde-b387-36a1fa36dbf5}",  3,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня взвод (свои факт)")},
        {"{58e025a0-f741-4fe2-8dd4-7e3933520e9c}",  4,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня рота (свои факт)")},
        {"{b93384b3-490b-49e4-a848-cb4b12fd9738}",  5,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня батальон (свои факт)")},
        {"{8f38b9f3-a110-4768-99ad-29301d38007e}",  6,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня полк (свои факт)")},
        {"{3fb5b1a3-4ace-492b-a6ae-2caa95c71939}",  7,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня бригада (свои факт)")},
        {"{88e956ca-1fe6-4626-9b8b-62c004429a4e}",  8,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня дивизия (свои факт)")},
        {"{82e7c29f-cd3f-4f4d-97da-4f44eed1c244}",  9,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня армия (АК) (свои факт)")},
        {"{5148f700-ea11-4ced-83eb-9c9364cbe1e6}", 10,   Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района оперативно-стратегического (стратегического) воинского формирования (свои, факт)")},
        {"{a082e878-fd54-4d88-a52e-2199c97228ac}",  2, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня отделение (свои факт)")},
        {"{827bbea1-c550-4623-a44e-40d60f29e0eb}",  3, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня взвод (свои факт)")},
        {"{193bac45-af68-4840-afde-b803da5e5f75}",  4, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня рота (свои факт)")},
        {"{ee53d6cd-2d31-4c1c-978d-e7e07872224c}",  5, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня батальон (свои факт)")},
        {"{20493398-f5aa-4d02-b83e-941608e1fb72}",  6, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня полк (свои факт)")},
        {"{d031d20e-090c-4325-8525-2ca5f493789b}",  7, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня бригада (свои факт)")},
        {"{6f8c13a2-29ae-4c9b-9221-c174524615bf}",  8, Qt::black, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня дивизия (свои факт)")},
        {"{4841d910-b692-4473-b92d-a5972060f6c6}",  2,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня отделение (свои план)")},
        {"{dbc66560-57c8-42cb-aece-040e709ffc9a}",  3,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня взвод (свои план)")},
        {"{2db31e8a-addc-4888-9f27-3de66ad415c2}",  4,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня рота (свои план)")},
        {"{fad86d9a-8172-47fc-ac4b-32c63d56f6c4}",  5,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня батальон (свои план)")},
        {"{90f5ed7e-e0ce-4af2-aec6-748945e23862}",  6,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня полк (свои план)")},
        {"{cd7ed838-e560-428b-bdfa-70855c8cedb5}",  7,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня бригада (свои план)")},
        {"{797f1bc9-1255-4535-a29b-6b98a5299962}",  8,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня дивизия (свои план)")},
        {"{0e6e33e7-af2c-4340-8949-f5aa8c0695e7}",  9,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения общевойскового ВФ уровня армия (АК) (свои план)")},
        {"{6b2d9abf-ab33-4de5-8998-83f4bee179df}", 10,   Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района оперативно-стратегического (стратегического) воинского формирования (свои, план)")},
        {"{054fb744-00b6-48be-9da8-4e496dde313b}",  2, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня отделение (свои план)")},
        {"{57395ccd-d813-4b8a-9c43-43ecea279d61}",  3, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня взвод (свои план)")},
        {"{6d6aed3e-eb61-404a-a462-e24a99276a77}",  4, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня рота (свои план)")},
        {"{9b9a5e35-e2f3-4d17-909c-a8322b64b433}",  5, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня батальон (свои план)")},
        {"{c36c37e0-8a40-4a62-a36e-0b576adba506}",  6, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня полк (свои план)")},
        {"{2932aa27-1f4f-4382-ae18-82b447f95539}",  7, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня бригада (свои план)")},
        {"{fe7f8383-541a-4871-adb5-632e7a98a70a}",  8, Qt::black,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ рода войск уровня дивизия (свои план)")},
        {"{600ce208-6892-4f70-877a-f64a96ec5eb3}",  2,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня отделение (противник факт)")},
        {"{31ab9940-77f7-4f31-92e8-f741c07e70f5}",  3,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня взвод (противник факт)")},
        {"{f436282f-0503-4cb8-9508-59e65b9e3235}",  4,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня рота (противник факт)")},
        {"{6ac640f8-9afb-46d2-8d46-4e67fc92518f}",  5,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня батальон (противник факт)")},
        {"{d56a0a22-70d7-4040-9428-0f9f2b4e5d36}",  6,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня полк (противник факт)")},
        {"{5ae3032a-7fd8-41c1-a1fe-f6e40e4109fc}",  7,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня бригада (противник факт)")},
        {"{49ccf535-d003-4f22-ac41-6da3259ffa02}",  8,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня дивизия (противник факт)")},
        {"{53958098-4355-4798-b0f1-f42f84465fca}",  9,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня армия (АК) (противник факт)")},
        {"{111a6f6e-db32-4d73-830f-05c989a0e11b}", 10,  Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района оперативно-стратегического (стратегического) воинского формирования (противник, факт)")},
        {"{6e8b9dbf-6f55-4864-86ca-354249ef976e}",  2,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   25000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня отделение (противник план)")},
        {"{2e2e8aa3-baaa-4f30-9a6a-c9998ac3f2e7}",  3,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,   50000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня взвод (противник план)")},
        {"{91a58859-20ff-4f57-8fa3-f7d717db414d}",  4,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  200000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня рота (противник план)")},
        {"{495b71d5-7250-4561-b6a0-c2bf890c5736}",  5,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0,  500000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня батальон (противник план)")},
        {"{742da70e-4007-43d9-b89a-e6bca94db91b}",  6,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня полк (противник план)")},
        {"{616901a1-4469-4eb6-9f0a-c738829f25d7}",  7,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня бригада (противник план)")},
        {"{9fdf989c-5186-4f2d-8b07-6abd7063fd02}",  8,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 1000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня дивизия (противник план)")},
        {"{d755b72c-938d-49e0-9676-f65e4701d138}",  9,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 3000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района расположения ВФ уровня армия (АК) (противник план)")},
        {"{528458e1-e91f-4fb5-a99b-e8517860b8cb}", 10,  Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::OddEvenFill, false, true, 0, 5000000, 0, 0, true, LineDrawerSettings::TypePartEdge, QString::fromUtf8("Часть границы района оперативно-стратегического (стратегического) воинского формирования (противник, план)")},

        // Траншеи
        {"{7b0128e1-1088-4e51-8cdc-a3d4994a915b}", 2,  Qt::red, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 10000, 0, 0, false, LineDrawerSettings::TypeTrench, QString::fromUtf8("Траншея (свои, факт)")},
        {"{df65ba88-e152-4473-9925-47990c88840d}", 2, Qt::blue, Qt::SolidLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 10000, 0, 0, false, LineDrawerSettings::TypeTrench, QString::fromUtf8("Траншея (противник, факт)")},
        {"{2cc1cf22-784d-4c48-8f9e-a429590324be}", 2,  Qt::red,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 10000, 0, 0, false, LineDrawerSettings::TypeTrench, QString::fromUtf8("Траншея (свои, план)")},
        {"{5672e395-fff1-47f1-a86c-880e216803af}", 2, Qt::blue,  Qt::DashLine, Qt::red, Qt::NoBrush, Qt::WindingFill, false, false, 0, 10000, 0, 0, false, LineDrawerSettings::TypeTrench, QString::fromUtf8("Траншея (противник, план)")},

        // Затопленные участки местности
        {"{076204cd-32a1-4418-a3b6-546682d22423}", 3, Qt::blue, Qt::SolidLine, Qt::blue, Qt::FDiagPattern, Qt::WindingFill, false, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneFlood, QString::fromUtf8("Затопленный участок местности (факт)")},
        {"{22133dda-7e4e-46c1-822f-d3a67e8be1e8}", 3, Qt::blue,  Qt::DashLine, Qt::blue, Qt::FDiagPattern, Qt::WindingFill, false, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneFlood, QString::fromUtf8("Затопленный участок местности (план)")},

        {"{55de8452-4ca6-4292-afa3-e33a4c96dbbe}", 2, Qt::blue,  Qt::DashLine, Qt::blue, Qt::VerPattern, Qt::WindingFill, false, true,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeZoneFlood, QString::fromUtf8("Зона возможного затопления")},
        {"{4660b7fb-fc3c-476f-9250-586aa3057f43}", 2, Qt::blue, Qt::SolidLine, Qt::blue, Qt::HorPattern, Qt::WindingFill, false, true,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeZoneFlood, QString::fromUtf8("Зона возможного наводнения (паводки)")},

        // Узлы заграждений
        {"{e9f3a12a-b642-4967-ad80-9239d77d6863}", 3, Qt::black, Qt::SolidLine, Qt::black, Qt::VerPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneBarries, QString::fromUtf8("Узел заграждений (свои, факт)")},
        {"{95197967-331a-4bbc-b080-41fd111cb619}", 3, Qt::black,  Qt::DashLine, Qt::black, Qt::VerPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneBarries, QString::fromUtf8("Узел заграждений (свои, план)")},
        {"{73a58786-c2a3-4f3d-9a81-bfd9b91f9e0a}", 3,  Qt::blue, Qt::SolidLine,  Qt::blue, Qt::VerPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneBarries, QString::fromUtf8("Узел заграждений (пр-к, факт)")},
        {"{2eadef26-1979-4f87-ba8d-7a573087f6f9}", 3,  Qt::blue,  Qt::DashLine,  Qt::blue, Qt::VerPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneBarries, QString::fromUtf8("Узел заграждений (пр-к, план)")},

        // Зараженные участки местности отравляющими веществами
        {"{22d9606a-d590-4218-9fda-b7f35f950f82}", 3, Qt::blue, Qt::DashLine, Qt::yellow, Qt::SolidPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZonePoison, QString::fromUtf8("Участок местности (район), зараженный ОВ (факт)")},
        {"{4ffa847f-49c9-49c8-8f01-7d5ab043c233}", 3, Qt::blue, Qt::DashLine, Qt::yellow, Qt::SolidPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZonePoison, QString::fromUtf8("Участок местности (район), зараженный ОВ (план)")},
        // Зараженные участки местности биологическими средствами
        {"{a8733533-9544-4986-9599-721e3ec16a90}", 3, Qt::blue, Qt::DashLine,   Qt::gray, Qt::BDiagPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneBio, QString::fromUtf8("Участок местности (район), зараженный биологическими средствами (факт)")},
        {"{0c17ae6e-59fb-4f17-86a0-79fd7356cb94}", 3, Qt::blue, Qt::DashLine,   Qt::gray, Qt::BDiagPattern, Qt::WindingFill, true, true, 0, MAXGEN, 0, 0, false, LineDrawerSettings::TypeZoneBio, QString::fromUtf8("Участок местности (район), зараженный биологическими средствами (план)")},

        // Зона неочещенная
        {"{9d78d536-7ce5-43dc-85f3-c37fe7c15f47}", 0, Qt::gray, Qt::NoPen, Qt::gray, Qt::DiagCrossPattern, Qt::OddEvenFill,  true, false, 0, 500000000, 0, 0, false, LineDrawerSettings::TypeZoneUncleaned, QString::fromUtf8("Зона, неочищенная от взрывоопасных предметов")},

        // Неизвестно
        {"{505d8be4-1644-4ad0-8da3-427d80d35b4a}", 2, Qt::red, Qt::SolidLine, Qt::black, Qt::NoBrush, Qt::WindingFill,  true, false,  0, 500000000, 0, 0, false, LineDrawerSettings::TypeDeafult, QString::fromUtf8("Граница поста (свои, факт)")},
    #endif
    };

    int size = sizeof(_line) / sizeof(LineDrawerSettings);

    QString cls;
    settings.clear();
    for (int i = 0; i < size; ++i) {
        cls = _line[i].cls;
        if (settings.contains(cls)) {
            qWarning() << "Error. Double classif " << cls;
            continue;
        }
        // TODO: валидация данных на ошибку
        settings[_line[i].cls] = &_line[i];
    }
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
