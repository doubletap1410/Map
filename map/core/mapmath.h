#ifndef MAPMATH_H
#define MAPMATH_H

#include <QList>
#include <QPolygonF>
#include <QPair>
#include <QLineF>

#include <QImage>
#include <QColor>
#include <QSvgRenderer>
#include <QTransform>
#include <QTextDocument>

// ---------------------------------------------

namespace minigis {

// ---------------------------------------------

class MapCamera;

// ---------------------------------------------

// polarAngle calculate polar angle (in radians)
double polarAngle(double x, double y);
inline double polarAngle(QPointF point) { return polarAngle(point.y(), point.x()); }
// rotateVect rotate point by angle (in radians)
QPointF rotateVect(QPointF vect, qreal angle);
// lengthR2Squared sum of coordinates squares
inline double lengthR2Squared(double x, double y) { return x * x + y * y; }
inline double lengthR2Squared(QPointF point) { return lengthR2Squared(point.x(), point.y()); }
// lengthR2Squared sqrt of sum of coordinates squares
inline double lengthR2(double x, double y) { return sqrt(x * x + y * y); }
inline double lengthR2(QPointF point) { return lengthR2(point.x(), point.y()); }
// vectProdABxAC - vector product of vectors AB and AC
inline double vectProdABxAC(double x1, double y1, double x2, double y2, double x3, double y3)
{
    return (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
}
inline double vectProdABxAC(QPointF A, QPointF B, QPointF C)
{
    return vectProdABxAC(A.x(), A.y(), B.x(), B.y(), C.x(), C.y());
}
// dotProduct
inline qreal dotProduct(const QPointF &u, const QPointF &v) { return u.x() * v.x() + u.y() * v.y(); }

//! функции для работы с ломаными -------------------------------------------------------------------------------------------------

// polygonSmoothing - polygon smoothing with Duglas-Pekker algorithm
QPolygonF polygonSmoothing(QPolygonF curve, qreal epsilon);

// convexHull
QPolygonF convexHull(const QPolygonF &metric, double epsilon = .5);

// pointAt - point on polygon
QPointF pointAt(const QPolygonF &p, qreal t, QLineF *segment = NULL);
// polyOrietation - polygon orientation (true - counterclockwise, false - clockwise)
bool polyOrietation(const QPolygonF &poly);
// changeOrientation - change polygon orientation
void changeOrientation(QPolygonF &poly);
// distanceToLine
qreal distanceToLine(const QPointF &p, const QLineF &l);
// distToSegment - distance to segment
double distToSegment(const QPointF &point, const QLineF &line);
// proection - point projectin on line
QPointF proection(const QPointF &point, const QLineF &line);
// pointOnLine - check is point belogns to line
bool pointOnLine(const QPointF &point, const QLineF &line, qreal &t);

// rectToPoly - transform rect to polygon
QPolygonF rectToPoly(QRectF r);

//! функции для работы с кубической кривой безье -------------------------------------------------------------------------------------------------

// updateEdge - update one spline edge
QLineF updateEdge(const QPolygonF &poly, int ip0, int ip1,int ip2, int ip3);
// generateCubicSpline - generate directing points of spline
QList<QLineF> generateCubicSpline(QPolygonF points, bool isClosed);
QList<QLineF> generateCubicSpline(QPolygonF points);
QList<QLineF> generateCubicSplineClosed(QPolygonF points);
// simpleCubicSpline - generate cubic Bezie (spline)
QPolygonF simpleCubicSpline(QPolygonF points, bool isClosed);
QPolygonF simpleCubicSpline(QPolygonF points);
QPolygonF simpleCubicSplineClosed(QPolygonF points);
// cubicPoint - point on spline
inline QPointF cubicPoint(qreal t, QPointF p1, QPointF p2, QPointF cp1, QPointF cp2);
// derivativeCubic - tangen to spline
QPointF derivativeCubic(QPointF p0, QPointF p1, QPointF p2, QPointF p3, qreal t);
// distance - distacnce from point to spline
qreal distanceToCubic(QPointF p, QPointF p1, QPointF p2, QPointF cp1, QPointF cp2);
// subcurveBezier - splice spline into two splines by param
void subcurveBezier(QPointF P0, QPointF P1, QPointF P2, QPointF P3, qreal param, QPolygonF *left = NULL, QPolygonF *right = NULL);

// solveCubicEquation a*x^3 + b*x^2 + c*x + d = 0; a != 0; (Only real roots)
QVector<double> solveCubicEquation(double aa, double bb, double cc, double dd);

//intersectCubicWithLine - intersection line with spline
QVector<QPair<double, double> > intersectCubicWithLine(QPointF P0, QPointF P1, QPointF P2, QPointF P3, QPointF L0, QPointF L1);

//! функции для работы с кривыми -------------------------------------------------------------------------------------------------

// pointCanBeRemovedLinear - can be point removed from polygon
bool pointCanBeRemovedLinear(const QPolygonF &poly, int index, bool isClosed);
// pointCanBeRemovedCubic - can be point removed from cubic spline
bool pointCanBeRemovedCubic(const QPolygonF &poly, int index, bool isClosed);

// nearControlPoint - nearest point of polygon closer then controlR
bool nearControlPoint(const QPolygonF &poly, const QPointF &point, int &num, qreal controlR = 30);
// nearEdgePoint - nearest edge of polygon closer then maxdist
bool nearEdgePoint(const QPolygonF &poly, const QPointF &point, int &num, bool closed = false, qreal maxdist = 20);
// nearEdgePointCubic - nearest segment of cubic spline colser then maxdist
bool nearEdgePointCubic(const QPolygonF &poly, const QPointF &point, int &num, bool closed = false, qreal maxdist = 20);

struct LineElement
{
    LineElement() : rect(QPolygonF()), ind(0), pos(QPointF()) { }
    LineElement(QPolygonF rect_, int ind_, QPointF pos_)
        : rect(rect_), ind(ind_), pos(pos_) { }

    QPolygonF rect;
    int ind;
    QPointF pos;
};

// calcPath - cut elements from metric
QVector<QPolygonF> calcPath(const QPolygonF &metric, const QVector<LineElement> &elements);
QPointF calcPoint(const QPolygonF &p, const QVector<qreal> &param, qreal length, qreal t, QLineF *l = NULL, int *number = NULL);
// lineParams - parametrize polygon
QVector<qreal> lineParams(const QPolygonF &metric, qreal &length, bool isClosed);
QVector<qreal> lineParams(const QPolygonF &metric, qreal &length);
QVector<qreal> lineParamsClosed(const QPolygonF &metric, qreal &length);

struct SplineElement
{
    SplineElement() : rect(QPolygonF()), ind(0), param(0.0) { }
    SplineElement(QPolygonF rect_, int ind_, qreal param_)
        : rect(rect_), ind(ind_), param(param_) { }

    QPolygonF rect;
    int ind;
    qreal param;
};

// calcPath - cut elements from cubic
QVector<QPainterPath> calcPathCubic(const QPolygonF &cubic, const QVector<SplineElement> &elements);
QPointF calcPointCubic(const QPolygonF &p, const QVector<qreal> &param, qreal length, qreal t, qreal *partT = NULL, QPolygonF *l = NULL, int *number = NULL);
// lineParams - parametrize spline
QVector<qreal> cubicParams(const QPolygonF &cubic, qreal &length);

//! -------------------------------------------------------------------------------------------------

// triang - polygon triangulation
QList<QPolygonF> triang(const QPolygonF &p);

// pointInTriangle - check is point in triangle
bool pointInTriangle(QPointF p, QPointF a, QPointF b, QPointF c);
bool pointInTriangle(const QPointF &p, const QPolygonF &poly);

//!//! ------------------------------------------------------------------------------------------------

// generateTransformImage
QImage generateTransformImage(const QImage &img, const QPointF &start, const QTransform &transform, const QTransform &backTransform, QPointF *newStart = NULL, qreal opacity = 1., bool smoothed = true);
// positionFromRectTransform
QPointF positionFromRectTransform(const QPointF &start, const QSize &size, const QTransform &transform);

// changeColorImageByPixel - change color of every pixel
void changeColorImageByPixel(QImage& im, QColor c);
// changeColorImageByPainter - fill image by color c using QPainter
void changeColorImageByPainter(QImage& im, QColor c);

namespace ConvertColor {
// Change color function
typedef void(*ColorFilterFunc)(QImage *, QVariantMap);

void emptyColor(QImage *img, QVariantMap options);
void hsvColor(QImage *img, QVariantMap options);
void invertedColor(QImage *img, QVariantMap options);
void invertedHsvColor(QImage *img, QVariantMap options);
void grayColor(QImage *img, QVariantMap options);
}
//!//! ------------------------------------------------------------------------------------------------

//! get pixel function
typedef QRgb(*pixelFunc)(const QImage &, const QPointF &);

// getPixel - direct get pixel
QRgb getPixel(const QImage &img, const QPointF &p);
// getPixelBilinear - get pixel using bilinear smoothing
QRgb getPixelBilinear(const QImage &img, const QPointF &p);

/**
 * @brief imgFilter from distored image
 * @param img source image
 * @param source polygon on source image (source <= dest)
 * @param dest result polygon
 * @param f get pixel function
 * @param insideL list of inside points in source polygon (insideL == insideR)
 * @param insideR list of inside points in result polygon
 * @return distored image
 */
QImage imgFilter(const QImage &img, QPolygonF source, QPolygonF dest, pixelFunc f,
               QPolygonF insideS = QPolygonF(), QPolygonF insideD = QPolygonF());

//!//! функции зависящие от параметров системы WGS ------------------------------------------------------------------------------------------------

/**
 * @brief solveInverseProblem solve inverse geodesic problem
 * @param lonlatS start point (in radians)
 * @param lonLatE end point (in radians)
 * @param dist distance between points
 * @param firstDir begin direction
 * @param secondDir end direction
 * @return выполнилось
 */
bool solveInverseProblem(const QPointF &latLonS, const QPointF &latLonE, qreal &dist, qreal &firstDir, qreal &secondDir);
bool solveInverseProblem(qreal lat1, qreal lon1, qreal lat2, qreal lon2, qreal &dist, qreal &firstDir, qreal &secondDir);

/**
 * @brief solveDirectProblem solve direct geodesic problem
 * @param lon longitude of start point (in radians)
 * @param lat latitude of start point (in radians)
 * @param brng begin direction
 * @param dist distance
 * @param endPoint end point
 * @param secondDir end direction
 */
void solveDirectProblem(qreal lat, qreal lon, qreal brng, qreal dist, QPointF &endPoint, qreal &secondDir);
void solveDirectProblem(const QPointF &latLon, qreal brng, qreal dist, QPointF &endPoint, qreal &secondDir);

// loxoLength расчет длины по локсодромии (по прямой в проекции Меркатора)
qreal loxoLength(const QPolygonF &line);
qreal geoLoxoLength(const QPolygonF &line);

// orthoLength расчет длины по ортодромии (по кратчайшему пути - длина геодезической линии)
qreal orthoLength(const QPolygonF &line);
qreal geoOrthoLength(const QPolygonF &line);

//!//! функции зависящие от камеры ------------------------------------------------------------------------------------------------

/**
 * @brief clustering кластеризует объекты
 * @param selectedObjects список объектов <позиция, id>
 * @param camera камера
 * @param gridSize размер сетки кластеризации
 * @return список групп
 */
QList<QList<QPair<QPointF, QString> > > clusterization(QList<QPair<QPointF, QString> > const &selectedObjects, const MapCamera *camera, int gridSize = 50);

//!//! ------------------------------------------------------------------------------------------------

// genSvg generate image from svg
QImage genSvg(QSvgRenderer &rscRender, const QString &uid, const QSizeF &size, QColor bg = Qt::transparent);

// genTransformSvg - generate transform image from svg
QImage genSvgTransform(QSvgRenderer &render, const QString &uid, const QSizeF &size, const QPointF &start,
                       const QTransform &transform, const QTransform &backTransform, QPointF *newStart = NULL, QColor bg = Qt::transparent);


// genText - genereate image with text
QImage genText(QTextDocument &txt, const QSizeF &size, QColor bg = Qt::transparent);
QImage genText(const QString &txt, const QFont &font, const QSizeF &size, QColor bg = Qt::transparent);

//!//! TEMPLATES ------------------------------------------------------------------------------------------------

template <bool IsClosed>
struct CubicSpline;

template <>
struct CubicSpline<false>
{
    inline static QPolygonF cubic(const QPolygonF &points) { return minigis::simpleCubicSpline(points); }
    inline static QList<QLineF> generateSpline(const QPolygonF &points) { return minigis::generateCubicSpline(points); }
    inline static QPainterPath spline(const QPolygonF &cubic)
    {
        if (cubic.size() < 4)
            return QPainterPath();

        QPainterPath bezie;
        bezie.moveTo(cubic.first());
        for (int i = 1; i < cubic.size() - 2; i += 3)
            bezie.cubicTo(cubic.at(i), cubic.at(i + 1), cubic.at(i + 2));

        return bezie;
    }
};

template <>
struct CubicSpline<true>
{
    inline static QPolygonF cubic(const QPolygonF &points) { return minigis::simpleCubicSplineClosed(points); }
    inline static QList<QLineF> generateSpline(const QPolygonF &points) { return minigis::generateCubicSplineClosed(points); }
    inline static QPainterPath spline(const QPolygonF &cubic)
    {
        if (cubic.size() < 4)
            return QPainterPath();

        QPainterPath bezie;
        bezie.moveTo(cubic.first());
        for (int i = 1; i < cubic.size() - 2; i += 3)
            bezie.cubicTo(cubic.at(i), cubic.at(i + 1), cubic.at(i + 2));

        bezie.closeSubpath();
        return bezie;
    }
};

// ------------------------------------

template <bool IsSpline, bool IsClosed>
struct ParamsHelper;

template <bool IsClosed> struct ParamsHelper<true, IsClosed>
{
    inline static QVector<qreal> params(const QPolygonF &metric, qreal &length) { return cubicParams(metric, length); }
};

template <> struct ParamsHelper<false, true>
{
    inline static QVector<qreal> params(const QPolygonF &metric, qreal &length) { return lineParamsClosed(metric, length); }
};

template <> struct ParamsHelper<false, false>
{
    inline static QVector<qreal> params(const QPolygonF &metric, qreal &length) { return lineParams(metric, length); }
};

// ---------------------------------------------

} // namespace minigis

// ---------------------------------------------

#endif // MAPMATH_H
