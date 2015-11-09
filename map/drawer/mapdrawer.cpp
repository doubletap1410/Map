
#include <QObject>
#include <QStringList>
#include <QPainter>
#include <QDir>
#include <QSvgRenderer>
#include <QSvgGenerator>
#include <QByteArray>
#include <QBuffer>

// -------------------------------------------------------

#include "mapdrawercommon.h"
#include "mapdrawer_p.h"
#include "mapdrawer.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

double GeneralizationExpCoefficient(double x)
{
    static const double c1 = 10000000; // параметр при котором показатель экспоненты равен 1
    static const double c2 = 50000;    // параметр при котором показатель экспоненты равен 0
    static const double c3 = 0;        // параметр при котором показатель экспоненты равен -1

    double denom = x > c2 ? c1 - c2 : c2 - c3;
    return (x - c2) / denom;
}

// -------------------------------------------------------

MapDrawer::MapDrawer(QObject *parent)
    : QObject(parent), d_ptr(new MapDrawerPrivate)
{
    setObjectName("MapDrawer");
}

// -------------------------------------------------------

MapDrawer::MapDrawer(MapDrawerPrivate &dd, QObject *parent)
    : QObject(parent),d_ptr(&dd)
{
    setObjectName("MapDrawer");
}

// -------------------------------------------------------

MapDrawer::~MapDrawer()
{
    if (d_ptr) {
        MapDrawerPrivate *tmp = d_ptr;
        d_ptr = NULL;
        delete tmp;
    }
}

// -------------------------------------------------------

QHash<int, QVariant> MapDrawer::attributes(QString const &uid, Attributes const &attr) const
{
    QHash<int, QVariant> res;
    foreach (int a, attr)
        res[a] = attribute(uid, a);
    return res;
}

// -------------------------------------------------------

bool MapDrawer::isClosed(const QString &)
{
    return false;
}

// -------------------------------------------------------

QImage MapDrawer::grayScaled(const QImage &source)
{
    QImage result(source.size(), source.format());
    unsigned int *data = (unsigned int *)source.bits();
    unsigned int *resultData = (unsigned int *)result.bits();
    int pixels = source.width() * source.height();
    for (int i = 0; i < pixels; ++i) {
        int val = qGray(data[i]);
        resultData[i] = qRgba(val, val, val, qAlpha(data[i]));
    }

    return result;
}

QVector<QLineF> MapDrawer::lines(const MapObject *object)
{
    Q_UNUSED(object);
    return QVector<QLineF>();
}

void MapDrawer::hide(MapObject const *object)
{
    Q_UNUSED(object);
}

bool MapDrawer::needLayerVisibilityChangedCatch()
{
    return false;
}

QSvgRenderer* MapDrawer::rendererForElement(const QString &elementId, const QColor &color, const QRectF &bounds, QSvgRenderer &sourceRenderer)
{
    static QMap<QString, QSvgRenderer*> sharedRenderers;
    QString key = QString("%1_%2").arg(elementId).arg(color.rgba());
    QSvgRenderer *renderer = sharedRenderers[key];

    if (renderer)
        return renderer;

    QImage img = grayScaled(svgGenerator(sourceRenderer, elementId, bounds));
    SAVEALPHACHANNEL(img, alpha);
    DRAWSELECTEDIMAGE(p, img, color);
    RESTOREALPHACHANNEL(img, alpha);

    QRectF b = bounds;
    b.moveTopLeft(QPointF());

    QByteArray svg;
    QBuffer svgBuffer(&svg);

    QSvgGenerator generator;
    generator.setOutputDevice(&svgBuffer);
    generator.setViewBox(b);

    QPainter p(&generator);
    p.fillRect(b, Qt::transparent);
    p.drawImage(QPointF(), img);
    p.end();

    renderer = new QSvgRenderer(this);
    renderer->load(svg);
    return renderer;
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
