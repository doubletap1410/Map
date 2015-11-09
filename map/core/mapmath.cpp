#include <QStack>
#include <QRgb>
#include <QPainter>

#include <qmath.h>

#include "coord/mapcamera.h"
#include "coord/mapcoords.h"
#include "core/mapdefs.h"
#include "mapmath.h"

// ---------------------------------------------

namespace minigis {

// ---------------------------------------------

namespace {

// ---------------------------------------------

// коодината Х соответсвующая координате Y прямой ab
qreal xLineCoord(QPointF a, QPointF b, qreal y)
{
    double A = a.y() - b.y();
    double B = b.x() - a.x();
    double C = double(a.x()) * b.y() - double(a.y()) * b.x();
    if (qFuzzyIsNull(A))
        return 0;
    return ((-1) * (C + B * y) / A);
}

// матрица трансформации двух полигонов (первая точка считается началом координат)
QMatrix triangleTransormation(QPolygonF p1, QPolygonF p2)
{
    if (p1.size() < 3 && p2.size() < 3)
        return QMatrix();

    QPointF start = p1.first();
    for (int i = 0; i < p1.size(); ++i)
        p1[i] -= start;

    start = p2.first();
    for (int i = 0; i < p2.size(); ++i)
        p2[i] -= start;

    qreal delta = double(p1.at(1).x()) * p1.at(2).y() - double(p1.at(1).y()) * p1.at(2).x();
    if (delta == 0.0)
        return QMatrix();

    qreal a = (double(p2.at(1).x()) * p1.at(2).y() - double(p1.at(1).y()) * p2.at(2).x()) / delta;
    qreal b = (double(p1.at(1).x()) * p2.at(2).x() - double(p2.at(1).x()) * p1.at(2).x()) / delta;
    qreal c = (double(p2.at(1).y()) * p1.at(2).y() - double(p1.at(1).y()) * p2.at(2).y()) / delta;
    qreal d = (double(p1.at(1).x()) * p2.at(2).y() - double(p2.at(1).y()) * p1.at(2).x()) / delta;

    return QMatrix(a, c, b, d, 0, 0);
}

typedef QPair<QPointF, int> PointNum;
bool sortByY(const PointNum &a, const PointNum &b)
{
    return qFuzzyCompare(a.first.y(), b.first.y()) ? a.first.x() < b.first.x() : a.first.y() < b.first.y();
}

void getTriangImg(const QImage &img, QPolygonF pin, QPolygonF p, QPointF topLeft, QImage &newImg, pixelFunc f)
{
    if (pin.size() < 3 || p.size() < 3)
        return;

    QList<PointNum> sorted;
    sorted.reserve(3);
    for (int i = 0; i < 3; ++i)
        sorted.append(PointNum(p.at(i), i));
    qSort(sorted.begin(), sorted.end(), sortByY);

    QList<int> id;
    id.reserve(3);
    for (int i = 0; i < 3; ++i)
        id.append(sorted.at(i).second);

    int topB    = qRound(p.at(id.at(0)).y() - topLeft.y());
    int bottomB = qRound(p.at(id.at(1)).y() - topLeft.y());

    QPointF fromP =   p.first();
    QPointF toP   = pin.first();

    QMatrix m(triangleTransormation(p, pin));
    m.translate((topLeft - fromP).x(), (topLeft - fromP).y());

    for (int y = topB; y < bottomB; ++y) {
        QRgb* line = reinterpret_cast< QRgb* >(newImg.scanLine(y));

        qreal l = qMin(xLineCoord(p.at(id.at(0)), p.at(id.at(1)), y + topLeft.y()),
                xLineCoord(p.at(id.at(0)), p.at(id.at(2)), y + topLeft.y())) - topLeft.x();
        qreal r = qMax(xLineCoord(p.at(id.at(0)), p.at(id.at(1)), y + topLeft.y()),
                xLineCoord(p.at(id.at(0)), p.at(id.at(2)), y + topLeft.y())) - topLeft.x();

        int leftB = qFloor(l);
        int rightB = qFloor(r);

        for (int x = leftB; x < rightB; ++x)
            if (x >= 0 && x < newImg.width())
                line[x] = f(img, m.map(QPointF(x, y)) + toP);
    }

    topB    = qRound(p.at(id.at(1)).y() - topLeft.y());
    bottomB = qRound(p.at(id.at(2)).y() - topLeft.y());

    for (int y = topB; y < bottomB; ++y) {
        QRgb* line = reinterpret_cast< QRgb* >(newImg.scanLine(y));

        qreal l = qMin(xLineCoord(p.at(id.at(1)), p.at(id.at(2)), y + topLeft.y()),
                xLineCoord(p.at(id.at(0)), p.at(id.at(2)), y + topLeft.y())) - topLeft.x();
        qreal r = qMax(xLineCoord(p.at(id.at(1)), p.at(id.at(2)), y + topLeft.y()),
                xLineCoord(p.at(id.at(0)), p.at(id.at(2)), y + topLeft.y())) - topLeft.x();

        int leftB  = qFloor(l);
        int rightB = qFloor(r);

        for (int x = leftB; x < rightB; ++x) {
            if (x >= 0 && x < newImg.width())
                line[x] = f(img, m.map(QPointF(x, y)) + toP);
        }
    }
}

//----------------------------------------------

void lineIntersectIndexMin(QPointF &point, const QLineF &origin, const QLineF &current, qreal leng, QPointF &p, qreal &min)
{
    if (origin.intersect(current, &p) == QLineF::BoundedIntersection) {
        qreal tmp = minigis::lengthR2(p - current.p1()) / leng;
        if (tmp < min) {
            point = p;
            min = tmp;
        }
    }
}

void lineIntersectIndexMax(QPointF &point, const QLineF &origin, const QLineF &current, qreal leng, QPointF &p, qreal &max)
{
    if (origin.intersect(current, &p) == QLineF::BoundedIntersection) {
        qreal tmp = minigis::lengthR2(p - current.p1()) / leng;
        if (tmp > max) {
            point = p;
            max = tmp;
        }
    }
}

int currentIndexMin(const QPolygonF &metric, const QPolygonF &rect, int startInd, QPointF &point)
{
    QLineF left(rect.at(3), rect.at(0));
    QLineF top(rect.at(0), rect.at(1));
    QLineF bottom(rect.at(2), rect.at(3));
    QLineF right(rect.at(1), rect.at(2));

    int number = startInd;
    QLineF current(metric.at(startInd), metric.at(startInd + 1));

    qreal leng = minigis::lengthR2(current.p2() - current.p1());
    QPointF p;
    qreal min = 1;

    lineIntersectIndexMin(point, left  , current, leng, p, min);
    lineIntersectIndexMin(point, top   , current, leng, p, min);
    lineIntersectIndexMin(point, bottom, current, leng, p, min);
    lineIntersectIndexMin(point, right , current, leng, p, min);

    return number;
}

int currentIndexMax(const QPolygonF &metric, const QPolygonF &rect, int startInd, QPointF &point)
{
    QLineF right(rect.at(1), rect.at(2));
    QLineF bottom(rect.at(2), rect.at(3));
    QLineF top(rect.at(0), rect.at(1));
    QLineF left(rect.at(3), rect.at(0));

    int number = startInd + 1;
    QLineF current(metric.at(startInd), metric.at(startInd + 1));

    qreal leng = minigis::lengthR2(current.p2() - current.p1());
    QPointF p;
    qreal max = 0;

    lineIntersectIndexMax(point, right , current, leng, p, max);
    lineIntersectIndexMax(point, bottom, current, leng, p, max);
    lineIntersectIndexMax(point, top   , current, leng, p, max);
    lineIntersectIndexMax(point, left  , current, leng, p, max);

    return number;
}

int nextIndexMin(const QPolygonF &metric, const QPolygonF &rect, int startInd, QPointF &point)
{
    QLineF left(rect.at(3), rect.at(0));
    QLineF top(rect.at(0), rect.at(1));
    QLineF bottom(rect.at(2), rect.at(3));
    QLineF right(rect.at(1), rect.at(2));

#define CALCINTERSECTMINC(T) \
if (current.intersect(T, &point) == QLineF::BoundedIntersection) { \
    number = ind; \
    break; \
}

    int number = -1;
    int ind = startInd - 1;
    while (ind >= 0) {
        QLineF current(metric.at(ind), metric.at(ind + 1));

        CALCINTERSECTMINC(left);
        CALCINTERSECTMINC(top);
        CALCINTERSECTMINC(bottom);
        CALCINTERSECTMINC(right);

        --ind;
    }

#undef CALCINTERSECTMINC

    if (ind == -1) {
        number = 0;
        point = metric.first();
    }
    return number;
}

int nextIndexMax(const QPolygonF &metric, const QPolygonF &rect, int startInd, QPointF &point)
{
    QLineF right(rect.at(1), rect.at(2));
    QLineF bottom(rect.at(2), rect.at(3));
    QLineF top(rect.at(0), rect.at(1));
    QLineF left(rect.at(3), rect.at(0));

#define CALCINTERSECTMAXC(T) \
if (current.intersect(T, &point) == QLineF::BoundedIntersection) { \
    number = ind + 1; \
    break; \
}

    int number = -1;
    int ind = startInd + 1;
    while (ind < metric.size() - 1) {
        QLineF current(metric.at(ind), metric.at(ind + 1));

        CALCINTERSECTMAXC(right);
        CALCINTERSECTMAXC(bottom);
        CALCINTERSECTMAXC(top);
        CALCINTERSECTMAXC(left);

        ++ind;
    }

#undef CALCINTERSECTMAXC

    if (ind == metric.size() - 1) {
        number = ind;
        point = metric.last();
    }
    return number;
}

// --------------------------------------------------

qreal calcCubicParam(QPointF P0, QPointF P1, QPointF P2, QPointF P3, const QLineF &line, bool &ok)
{
    typedef QPair<double, double> DoublePair;

    ok = false;
    qreal max = 0;
    QVector<DoublePair> sol = minigis::intersectCubicWithLine(P0, P1, P2, P3, line.p1(), line.p2());
    foreach (DoublePair uv, sol) {
        double u = uv.first;
        double v = uv.second;
        if ((qFuzzyIsNull(u) || qFuzzyIsNull(u - 1.) || (0. < u && u < 1.))
                && (qFuzzyIsNull(v) || qFuzzyIsNull(v - 1.) || (0. < v && v < 1.))) {

            if (u > max) {
                ok = true;
                max = u;
            }

        }
    }
    return max;
}

QList<qreal> caclCubicValues(const QPolygonF &cubic, int ind, const QLineF &leftL, const QLineF &rightL, const QLineF &topL, const QLineF &bottomL)
{
    QList<qreal> values;

    int num = (ind + 1) * 3;
    QPointF P0 = cubic.at(num - 3);
    QPointF P1 = cubic.at(num - 2);
    QPointF P2 = cubic.at(num - 1);
    QPointF P3 = cubic.at(num);

    bool ok = false;
    qreal val = calcCubicParam(P0, P1, P2, P3, leftL, ok);
    if (ok)
        values.append(val);
    val = calcCubicParam(P0, P1, P2, P3, topL, ok);
    if (ok)
        values.append(val);
    val = calcCubicParam(P0, P1, P2, P3, bottomL, ok);
    if (ok)
        values.append(val);
    val = calcCubicParam(P0, P1, P2, P3, rightL, ok);
    if (ok)
        values.append(val);

    return values;
}

int calcCubicMinIndex(const QPolygonF &cubic, const QLineF &leftL, const QLineF &rightL, const QLineF &topL, const QLineF &bottomL, int startInd, qreal &param)
{
    int left = -1;
    int ind = startInd - 1;
    while (left == -1) {
        if (ind < 0) {
            param = 0;
            left = 0;
            break;
        }

        QList<qreal> values = caclCubicValues(cubic, ind, leftL, rightL, topL, bottomL);

        if (!values.isEmpty()) {
            qSort(values);
            left = ind;
            param = values.last();
        }

        --ind;
    }
    return left;
}

int calcCubicMaxIndex(const QPolygonF &cubic, const QLineF &leftL, const QLineF &rightL, const QLineF &topL, const QLineF &bottomL, int startInd, qreal &param)
{
    int right = -1;
    int ind = startInd + 1;
    while (right == -1) {
        if ((ind + 1) * 3 >= cubic.size()) {
            param = 1;
            right = cubic.size() / 3 - 1;
            break;
        }

        QList<qreal> values = caclCubicValues(cubic, ind, leftL, rightL, topL, bottomL);

        if (!values.isEmpty()) {
            qSort(values);
            right = ind;
            param = values.first();
        }
        ++ind;
    }
    return right;
}

QList<QPair<int, qreal> > caclRectBoundsOnCubic(const QPolygonF &cubic, const QPolygonF &rect, int startInd, qreal startParam)
{
    QLineF leftL(rect.last(), rect.first());
    QLineF topL(rect.first(), rect.at(1));
    QLineF bottomL(rect.at(2), rect.last());
    QLineF rightL(rect.at(1), rect.at(2));

    QList<qreal> values = caclCubicValues(cubic, startInd, leftL, rightL, topL, bottomL);
    qSort(values);

    int left = startInd;
    qreal minParam = 0;
    if (!values.isEmpty() && values.first() < startParam)
        minParam = values.first();
    else
        left = calcCubicMinIndex(cubic, leftL, rightL, topL, bottomL, startInd, minParam);

    int right = startInd;
    qreal maxParam = 0;
    if (!values.isEmpty() && values.last() > startParam)
        maxParam = values.last();
    else
        right = calcCubicMaxIndex(cubic, leftL, rightL, topL, bottomL, startInd, maxParam);

    QList<QPair<int, qreal> > list;
    list.append(qMakePair(left , minParam));
    list.append(qMakePair(right, maxParam));
    return list;
}

// ---------------------------------------------

int classify(const QPointF &current, const QPointF &p0, const QPointF &p1)
{
    // LEFT = 1; RIGHT = -1; ELSE = 0;
    QPointF a = p1 - p0;
    QPointF b = current - p0;
    double sa = double(a.x()) * b.y() - double(b.x()) * a.y();
    if (qFuzzyIsNull(sa))
        return 0;
    else if (sa > 0)
        return 1;
    else
        return -1;
}

int findConvexVertex(const QPolygonF &list, int id = 0)
{
    int size = list.size();

    int indA = (id - 1) % size;
    int indB = id;
    int indC = (id + 1) % size;

    int num = 0;
    while (classify(
               list.at(indC),
               list.at(indA),
               list.at(indB)) != -1) {
        indA = indB;

        id = ++id % size;

        indB = id;
        indC = (id + 1) % size;

        ++num;
        if (num == size)
            break;
    }
    return id;
}

int findIntrudingVertex(const QPolygonF &list, int id)
{
    int size = list.size();
    int indA = (id - 1) % size;

    QPointF a = list.at(indA);
    QPointF b = list.at(id);
    QPointF c = list.at((id + 1) % size);

    id = ++id % size;

    int bestInd = -1;     // лучший кандидат на данный момент
    double bestD = -1.0;  // расстояние до лучшего кандидата
    QLineF ca(c, a);

    int ind = id + 1 % size;

    id = ++id % size;

    while (ind != indA) {
        QPointF v = list.at(ind);
        if (pointInTriangle(v, a, b, c)) {
            double dist = distanceToLine(v, ca);
            if (dist > bestD) {
                bestInd = ind;
                bestD = dist;
            }
        }

        id = ++id % size;
        ind = id;
    }
    return bestInd;
}

QPolygonF split(QPolygonF &list, int id, int indB)
{
    if (id >= indB)
        ++id;
    list.insert(indB, list.at(indB));
    ++indB;

    if (id + 1 == list.size())
        list.append(list.at(id));
    else {
        if (indB >= id + 1)
            ++indB;
        list.insert(id + 1, list.at(id));
    }

    int N = (indB - id - 1) % list.size();

    QPolygonF exit;
    int size = list.size();
    for (int i = 0; i < N; ++i)
        exit.append(list.at((id + 1 + i) % size));

    N = (id + 1 - indB) % size;

    QPolygonF temp;
    for (int i = 0; i < N; ++i)
        temp.append(list.at((indB + i) % size));

    list = temp;
    return exit;
}

// ---------------------------------------------

} // namespace

// ---------------------------------------------

double polarAngle(double x, double y)
{
    double t = atan2(y, x);
    if (t < 0) t += Pi2;
    return t;
}

QPointF rotateVect(QPointF vect, qreal angle)
{
    qreal cos = qCos(angle);
    qreal sin = qSin(angle);
    return QPointF(vect.x() * cos - vect.y() * sin, vect.y() * cos + vect.x() * sin);
}

QPolygonF polygonSmoothing(QPolygonF curve, qreal epsilon)
{
    QPolygonF result;
    if (curve.size() <= 1)
        return curve;
    result.append(curve.first());

    QStack<QPolygonF> st;
    QPolygonF poly;

    st.push(curve);

    while (!st.isEmpty()) {
        poly = st.pop();

        qreal dmax = 0;
        int index = 0;

        int N = poly.size() - 2;
        for (int i = 1; i < N; ++i) {
            qreal d = distanceToLine(poly.at(i), QLineF(poly.first(), poly.last()));
            if (d > dmax) {
                dmax = d;
                index = i;
            }
        }

        if (dmax > epsilon) {
            st.push(poly.mid(index));
            st.push(poly.mid(0, index + 1));
        }
        else
            result.append(poly.last());
    }

    return result;
}

QPolygonF convexHull(const QPolygonF &metric, double epsilon)
{
    int pointsSize = metric.size();
    QPolygonF hullsPoints;

    if (metric.isEmpty() || pointsSize < 4) {
        hullsPoints = metric;
        return hullsPoints;
    }

    QList<int> pointsIndex;
    for (int i = 0; i < pointsSize; ++i)
        pointsIndex.append(i);

    int firstPoint = 0;
    foreach (int i, pointsIndex) {
        QPointF fPoint = metric.at(firstPoint);
        QPointF sPoint = metric.at(pointsIndex.at(i));
        if (qFuzzyIsNull(fPoint.y() - sPoint.y()) && fPoint.x() > sPoint.x())
            firstPoint = i;
        else if (fPoint.y() < sPoint.y())
            firstPoint = i;
    }

    QPointF p = metric.at(firstPoint);
    hullsPoints.append(p);
    pointsIndex.removeAt(firstPoint);

    QLineF lastVector(p + QPointF(10, 0), p);
    QLineF currentVector;

    int indexMinAngle = -1;
    double lastLength = 0.;

    // находим вторую точку выпуклой оболочки
    qreal currentAngle;
    qreal lastAngle = Pi2;
    currentVector.setP1(lastVector.p2());

    int i = 0;
    int indSize = pointsIndex.size();

    while (i < indSize) {
        currentVector.setP2(metric.at(pointsIndex.at(i)));

        double len = lengthR2(currentVector.p2() - currentVector.p1());
        if (len < epsilon) {
            pointsIndex.removeAt(i);
            --indSize;
        } else {
//            currentAngle = lastVector.angleTo(currentVector);
            currentAngle = fmod(Pi2 - polarAngle(currentVector.p2() - currentVector.p1()) + polarAngle(lastVector.p2() - lastVector.p1()), Pi2);
            if (qFuzzyIsNull(currentAngle - lastAngle) && len > lastLength) {
                indexMinAngle = i;
                lastLength = len;
            }
            else if (currentAngle < lastAngle) {
                indexMinAngle = i;
                lastAngle = currentAngle;
            }
            ++i;
        }
    }

    // ищем остальные точки выпуклой оболочки
    pointsIndex.append(firstPoint);
    while (pointsIndex.at(indexMinAngle) != firstPoint) {
        if (indexMinAngle == -1)
            break;

        lastVector.setP1(lastVector.p2());
        lastVector.setP2(metric.at(pointsIndex.at(indexMinAngle)));

        hullsPoints.append(lastVector.p2());
        pointsIndex.removeAt(indexMinAngle);

        lastAngle = Pi2;
        indexMinAngle = -1;
        currentVector.setP1(lastVector.p2());
        i = 0;
        indSize = pointsIndex.size();
        while (i < indSize) {
            currentVector.setP2(metric.at(pointsIndex.at(i)));
            double len = lengthR2(currentVector.p2() - currentVector.p1());
            if (len < epsilon || qFuzzyIsNull(len - epsilon)) {
                pointsIndex.removeAt(i);
                --indSize;
            } else {
//                currentAngle = lastVector.angleTo(currentVector);
                currentAngle = fmod(Pi2 - polarAngle(currentVector.p2() - currentVector.p1()) + polarAngle(lastVector.p2() - lastVector.p1()), Pi2);
                if (qFuzzyIsNull(currentAngle - lastAngle) && len > lastLength && !qFuzzyIsNull(len - lastLength)) {
                    indexMinAngle = i;
                    lastLength = len;
                }
                else if (currentAngle < lastAngle) {
                    indexMinAngle = i;
                    lastAngle = currentAngle;
                }
                ++i;
            }
        }
    }
    return hullsPoints;
}

QPointF pointAt(const QPolygonF &p, qreal t, QLineF *segment)
{
    if (p.size() < 2)
        return QPointF();

    if (qFuzzyIsNull(t)) {
        if (segment)
            *segment = QLineF(p.first(), p.at(1));
        return p.first();
    }
    if (qFuzzyCompare(qreal(1), t)) {
        if (segment)
            *segment = QLineF(p.at(p.size() - 2), p.last());
        return p.last();
    }

    qreal len;
    QVector<qreal> param = lineParams(p, len);

    int k = 0;
    for (int i = 0; i < param.size(); ++i)
        if (t < param.at(i)) {
            k = i;
            break;
        }

    qreal delta = k == 0 ? 0 : param.at(k - 1);

    qreal dt = t - delta;
    QPointF vect = p.at(k + 1) - p.at(k);
    vect /= lengthR2(vect);

    qreal sum = 0.;
    for (int i = 1; i < p.size(); ++i)
        sum += lengthR2(p.at(i) - p.at(i-1));

    if (segment)
        *segment = QLineF(p.at(k), p.at(k + 1));
    return QPointF(p.at(k + 1) + vect * dt * sum);
}

bool polyOrietation(const QPolygonF &poly)
{
    if (poly.size() < 3)
        return true;
    int k = 0;
    QPointF minP = poly.first();
    for (int i = 1; i < poly.size(); ++i) {
        QPointF tmp = poly.at(i);
        if (qFuzzyCompare(minP.x(), tmp.x())) {
            if (minP.y() > tmp.y()) {
                k = i;
                minP = tmp;
            }
        }
        if (minP.x() > tmp.x()) {
            k = i;
            minP = tmp;
        }
    }
    if (vectProdABxAC(poly.at((k - 1 + poly.size()) % poly.size()), poly.at(k), poly.at((k+1) % poly.size())) > 0)
        return true;
    else
        return false;
}

void changeOrientation(QPolygonF &poly)
{
    for (int i = 0; i < int(poly.size() / 2); ++i)
        qSwap(poly[i], poly[poly.size() - 1 - i]);
}

qreal distanceToLine(const QPointF &p, const QLineF &l)
{
    if (qFuzzyCompare(l.x1(), l.x2()) && qFuzzyCompare(l.y1(), l.y2()))
        return lengthR2(p - l.p1());

    double A = l.y1() - l.y2();
    double B = l.x2() - l.x1();
    double C = double(l.x1()) * l.y2() - double(l.x2()) * l.y1();

    return qAbs((A * p.x() + B * p.y() + C) / lengthR2(A, B));
}

double distToSegment(const QPointF &point, const QLineF &line)
{
    QPointF v = line.p2() - line.p1();
    QPointF w = point - line.p1();

    double c1 = dotProduct(w, v);
    if (c1 < 0. || qFuzzyIsNull(c1))
        return lengthR2(point - line.p1());

    double c2 = lengthR2Squared(v);
    if (c2 < c1 || qFuzzyCompare(c1, c2))
        return lengthR2(point - line.p2());

    double b = c1 / c2;
    QPointF n = line.p1() + b * v;
    return lengthR2(point - n);
}

QPointF proection(const QPointF &point, const QLineF &line)
{
    double A, B, C;
    A = line.y1() - line.y2();
    B = line.x2() - line.x1();
    C = line.x1() * line.y2() - line.x2() * line.y1();

    QPointF p;
    if (!qFuzzyIsNull(B)) {
        double B2 = B * B;
        p.setX((point.x() * B2 - point.y() * A * B - A * C) / (A * A + B2));
        p.setY((-C - A * p.x()) / B);
    }
    else {
        double A2 = A * A;
        p.setY((point.y() * A2 - point.x() * A * B - B * C) / (A2 + B * B));
        p.setX((-C - B * p.y()) / A);
    }
    return p;
}

bool pointOnLine(const QPointF &point, const QLineF &line, qreal &t)
{
    if (qFuzzyIsNull(line.length()))
        return false;

    qreal dx = line.x2() - line.x1();
    qreal dy = line.y2() - line.y1();
    if (qFuzzyIsNull(dx)) {
        t = (point.y() - line.y1()) / dy;
        return qFuzzyCompare(line.x1(), point.x());
    }
    if (qFuzzyIsNull(dy)) {
        t = (point.x() - line.x1()) / dx;
        return qFuzzyCompare(line.y1(), point.y());
    }

    qreal tx = (point.x() - line.x1()) / dx;
    qreal ty = (point.y() - line.y1()) / dy;
    if (qFuzzyCompare(tx, ty)) {
        t = tx;
        return true;
    }
    return false;
}


QPolygonF rectToPoly(QRectF r)
{
    return QPolygonF() << r.topLeft() << r.topRight() << r.bottomRight() << r.bottomLeft();
}

QLineF updateEdge(const QPolygonF &poly, int ip0, int ip1, int ip2, int ip3)
{
    QPointF p1, p2;         // узловые точки (вершины базового полигона) теущего ребра
    QPointF m1, m2;
    QPointF c1, c2, c3;     // центры ребер (предыдущего,текущего и следующего)
    QPointF p0;             // предыдущая вершина относительно р1
    QPointF p3;             // следующая вершина р2
    double len1, len2, len3; // длины ребер
    double k1, k2;           // коэффициент пропорциональности ребер
    double smoothValue = 1;  // коэффициент сглаживания кривой Безье лежит в [0;1]

    //Вычисляем для первого ребра контрольные точки
    p0 = poly[ip0];
    p1 = poly[ip1];
    p2 = poly[ip2];
    p3 = poly[ip3];

    c1 = (p0 + p1)*.5;
    c2 = (p1 + p2)*.5;
    c3 = (p2 + p3)*.5;

    len1 = lengthR2(p1 - p0);
    len2 = lengthR2(p2 - p1);
    len3 = lengthR2(p3 - p2);

    k1 = len1 / (len1 + len2);
    k2 = len2 / (len2 + len3);

    m1 = c1 + k1 * (c2 - c1);
    m2 = c2 + k2 * (c3 - c2);

    return QLineF(
                p1 + smoothValue*(c2 - m1),
                p2 + smoothValue*(c2 - m2)
                );
}

QList<QLineF> generateCubicSpline(QPolygonF points, bool isClosed)
{
    return isClosed ? generateCubicSplineClosed(points) : generateCubicSpline(points);
}

QList<QLineF> generateCubicSpline(QPolygonF points)
{
    QList<QLineF> cp;
    int pSize = points.size();
    if (pSize <= 1)
        return cp;

    if (pSize == 2) {
        QPointF vect = points.last() - points.first();
        qreal len = lengthR2(vect);
        vect /= len;
        cp.append(QLineF(points.first() + vect * len * 0.25, points.first() + vect * len * 0.5));
        cp.append(QLineF(points.last()  - vect * len * 0.25, points.last()  - vect * len * 0.5));
        return cp;
    }

    cp.append(updateEdge(points, 0, 0, 1, 2));
    for (int i = 1; i < points.size() - 2; ++i)
        cp.append(updateEdge(points, i - 1, i, i + 1, i + 2));
    cp.append(updateEdge(points, pSize - 3, pSize - 2, pSize - 1, pSize - 1));

    return cp;
}

QList<QLineF> generateCubicSplineClosed(QPolygonF points)
{
    QList<QLineF> cp;
    int pSize = points.size();
    if (pSize <= 1)
        return cp;

    if (pSize == 2) {
        QPointF vect = points.last() - points.first();
        qreal len = lengthR2(vect);
        vect /= len;
        cp.append(QLineF(points.first() + vect * len * 0.25, points.first() + vect * len * 0.5));
        cp.append(QLineF(points.last()  - vect * len * 0.25, points.last()  - vect * len * 0.5));
        return cp;
    }

    cp.append(updateEdge(points, pSize - 1, 0, 1, 2));
    for (int i = 1; i < points.size() - 2; ++i)
        cp.append(updateEdge(points, i - 1, i, i + 1, i + 2));
    cp.append(updateEdge(points, pSize - 3, pSize - 2, pSize - 1, 0));
    cp.append(updateEdge(points, pSize - 2, pSize - 1, 0, 1));

    return cp;
}

QPolygonF simpleCubicSpline(QPolygonF points, bool isClosed)
{
    return isClosed ? simpleCubicSplineClosed(points) : simpleCubicSpline(points);
}

QPolygonF simpleCubicSpline(QPolygonF points)
{
    QList<QLineF> cp = generateCubicSpline(points);
    QPolygonF p;
    p.append(points.first());
    int count = points.size() - 1;
    for (int i = 0; i < count; ++i) {
        p.append(cp.at(i).p1());
        p.append(cp.at(i).p2());
        p.append(points.at(i + 1));
    }
    return p;
}

QPolygonF simpleCubicSplineClosed(QPolygonF points)
{
    QList<QLineF> cp = generateCubicSplineClosed(points);
    QPolygonF p;
    p.append(points.first());
    int count = points.size() - 1;
    for (int i = 0; i < count; ++i) {
        p.append(cp.at(i).p1());
        p.append(cp.at(i).p2());
        p.append(points.at(i + 1));
    }
    p.append(cp.last().p1());
    p.append(cp.last().p2());
    p.append(points.first());
    return p;
}

QPointF cubicPoint(qreal t, QPointF p1, QPointF p2, QPointF cp1, QPointF cp2)
{
    qreal d = 1 - t;
    qreal d2 = d * d;
    qreal t2 = t * t;
    return d2 * d * p1 + 3 * t * d2 * cp1 + 3 * t2 * d * cp2 + t2 * t * p2;
}

QPointF derivativeCubic(QPointF p0, QPointF p1, QPointF p2, QPointF p3, qreal t)
{
    qreal t1 = 1 - t;
    return 3 * t1 * t1 * (p1 - p0) + 6 * t * t1 * (p2 - p1) + 3 * t * t * (p3 - p2);
}

qreal distanceToCubic(QPointF p, QPointF p1, QPointF p2, QPointF cp1, QPointF cp2)
{
    double epsilon = 10; // 10 точек на сглаживание. погрешность

    double minD = lengthR2(p - p2);

    QPainterPath curve(p1);
    curve.cubicTo(cp1, cp2, p2);
    double dist;

    if (qFuzzyIsNull(curve.length()))
        return minD;

    int i = 0;
    for (qreal t = 0.; t < 1.; t = curve.percentAtLength(++i * epsilon))
        if ((dist = lengthR2(p - curve.pointAtPercent(t))) < minD)
            minD = dist;

    return minD;
}

void subcurveBezier(QPointF P0, QPointF P1, QPointF P2, QPointF P3, qreal param, QPolygonF *left, QPolygonF *right)
{
    QLineF a1(P0, P1);
    QLineF a2(P1, P2);
    QLineF a3(P2, P3);

    QPointF aEnd = a2.pointAt(param);
    QLineF b1(a1.pointAt(param), aEnd);
    QLineF b2(aEnd, a3.pointAt(param));

    QLineF c(b1.pointAt(param), b2.pointAt(param));
    QPointF mid = c.pointAt(param);
    if (left) {
        QPolygonF p;
        p << P0 << b1.p1() << c.p1() << mid;
        *left = p;
    }
    if (right) {
        QPolygonF p;
        p << mid << c.p2() << b2.p2() << P3;
        *right = p;
    }
}

QVector<double> solveCubicEquation(double aa, double bb, double cc, double dd)
{
    if (qFuzzyIsNull(aa))
        return QVector<double>();

    double b = bb / aa;
    double c = cc / aa;
    double d = dd / aa;

    double p = c - b * b / 3.;
    double q = 2. * b * b * b / 27. - b * c / 3. + d;
    double k[12];

    double k1, k2, xx, in;
    double tmp = q * q / 4. + p * p * p / 27.;
    if (tmp < 0.) {
        k1 = -q / 2.;
        k2 = sqrt(fabs(tmp));
        xx = pow(k1 * k1 + k2 * k2, 1. / 6.);

        in = atan(k2 / k1);
        if (k1 < 0.)
            in += M_PI;

        k[0] = xx * cos( in / 3.);
        k[1] = xx * cos((in + 2 * M_PI) / 3.);
        k[2] = xx * cos((in + 4 * M_PI) / 3.);

        k[3] = xx * sin( in / 3.);
        k[4] = xx * sin((in + 2 * M_PI) / 3.);
        k[5] = xx * sin((in + 4 * M_PI) / 3.);

        in = atan(-k2 / k1);
        if (k1 < 0.)
            in += M_PI;

        k[6] = xx * cos( in / 3.);
        k[7] = xx * cos((in + 2 * M_PI) / 3.);
        k[8] = xx * cos((in + 4 * M_PI) / 3.);

        k[9]  = xx * sin( in / 3.);
        k[10] = xx * sin((in + 2 * M_PI) / 3.);
        k[11] = xx * sin((in + 4 * M_PI) / 3.);
    }
    else {
        k1 = -q / 2. + sqrt(fabs(tmp));
        k2 = -q / 2. - sqrt(fabs(tmp));
        in = (k1 < 0. ? M_PI : 0.);

        double v = pow(fabs(k1), 1. / 3.);
        k[0] = v * cos( in / 3.);
        k[1] = v * cos((in + 2 * M_PI) / 3.);
        k[2] = v * cos((in + 2 * M_PI) / 3.);

        k[3] = v * sin( in / 3.);
        k[4] = v * sin((in + 2 * M_PI) / 3.);
        k[5] = v * sin((in + 4 * M_PI) / 3.);

        in = (k2 < 0. ? M_PI : 0.);

        v = pow(fabs(k2), 1. / 3.);
        k[6] = v * cos( in / 3.);
        k[7] = v * cos((in + 2 * M_PI) / 3.);
        k[8] = v * cos((in + 4 * M_PI) / 3.);

        k[9]  = v * sin( in / 3.);
        k[10] = v * sin((in + 2 * M_PI) / 3.);
        k[11] = v * sin((in + 4 * M_PI) / 3.);
    }

    double test[9];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            xx = k[i] * k[6 + j] - k[3 + i] * k[9 + j];
            test[i * 3 + j] = fabs(xx + p / 3.);
        }
    }

    int qqq[3];
    double res[6];

    xx = test[0];
    qqq[0] = 0;
    for (int i = 0; i < 9; ++i) {
        if (test[i] < xx) {
            xx = test[i];
            qqq[0] = i;
        }
    }

    if (qqq[0] == 0) {
        xx = test[1];
        qqq[1] = 1;
    }
    else {
        xx = test[0];
        qqq[1] = 0;
    }
    for (int i = 0; i < 9; ++i) {
        if (test[i] < xx && i != qqq[0]) {
            xx = test[i];
            qqq[1] = i;
        }
    }

    if (qqq[0] == 0 || qqq[1] == 0) {
        xx = test[1];
        qqq[2] = 1;
        if (qqq[0] == 1 || qqq[1] == 1) {
            xx = test[2];
            qqq[2] = 2;
        }
    }
    else {
        xx = test[0];
        qqq[2] = 0;
    }
    for (int i = 0; i < 9; ++i) {
        if (test[i] < xx && i != qqq[0] && i != qqq[1]) {
            xx = test[i];
            qqq[2] = i;
        }
    }

    int ind = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            xx = k[i] * k[6 + j] - k[3 + i] * k[9 + j];
            if (fabs(xx + p / 3.) < 1e-4) {
                int t = i * 3 + j;
                if (t == qqq[0] || t == qqq[1] || t == qqq[2]) {
                    res[ind] = k[i] + k[6 + j];
                    res[ind + 3] = k[i + 3] + k[9 + j];
                    ++ind;
                }
            }
        }
    }

    for (int i = 0; i < 3; ++i)
        res[i] = res[i] - b / 3.;

    QVector<double> solve;
    for (int i = 0; i < 3; ++i) {
        if (qFuzzyIsNull(fabs(res[i + 3]))) {
            solve.append(res[i]);
//            double dt = fabs(aa * y * y * y + bb * y * y + cc * y + dd);
        }
    }
    return solve;
}

QVector<QPair<double, double> > intersectCubicWithLine(QPointF P0, QPointF P1, QPointF P2, QPointF P3, QPointF L0, QPointF L1)
{
   QPointF da = P3 - P0 + 3 * (P1 - P2);
   QPointF db = 3 * (P0 + P2 - 2 * P1);
   QPointF dc = 3 * (P1 - P0);
   QPointF dd = L1 - L0;

   QVector<QPair<double, double> > solution;
   if (qFuzzyIsNull(dd.x())) {
       double dl = dd.x() / double(dd.y());

       double fa = da.x() - da.y() * dl;
       double fb = db.x() - db.y() * dl;
       double fc = dc.x() - dc.y() * dl;
       double fd = P0.x() - L0.x() - (P0.y() - L0.y()) * dl;

       QVector<double> solveU = solveCubicEquation(fa, fb, fc, fd);
       foreach (double u, solveU) {
           double u2 = u * u;
           double u3 = u2 * u;
           double v = (da.y() * u3 + db.y() * u2 + dc.y() * u + P0.y() - L0.y()) / dd.y();
           solution.append(qMakePair(u, v));
       }
   }
   else {
       double dl = dd.y() / double(dd.x());

       double fa = da.y() - da.x() * dl;
       double fb = db.y() - db.x() * dl;
       double fc = dc.y() - dc.x() * dl;
       double fd = P0.y() - L0.y() - (P0.x() - L0.x()) * dl;

       QVector<double> solveU = solveCubicEquation(fa, fb, fc, fd);
       foreach (double u, solveU) {
           double u2 = u * u;
           double u3 = u2 * u;
           double v = (da.x() * u3 + db.x() * u2 + dc.x() * u + P0.x() - L0.x()) / dd.x();
           solution.append(qMakePair(u, v));
       }
   }
   return solution;
}

bool pointCanBeRemovedLinear(const QPolygonF &poly, int index, bool isClosed)
{
    qreal epsilon = 10; // 10 точек на сглаживание. погрешность

    int pSize = poly.size();

    if (pSize < 2 || index < 0 || index >= pSize)
        return false;

    int prev = index - 1;
    int next = index + 1;

    if (isClosed) {
        prev = prev < 0 ? prev + pSize : prev;
        next = next >= pSize ? next - pSize : next;
    }
    else {
        if (index == 0)
            return QLineF(poly.at(index), poly.at(next)).length() <= epsilon;
        if (index == pSize - 1)
            return QLineF(poly.at(index), poly.at(prev)).length() <= epsilon;
    }

    QPointF pi = poly.at(index);
    QPointF pn = poly.at(next);
    QPointF pp = poly.at(prev);

    if (pi.x() < qMin(pn.x(), pp.x()) - epsilon)
        return false;
    if (pi.x() > qMax(pn.x(), pp.x()) + epsilon)
        return false;
    if (pi.y() < qMin(pn.y(), pp.y()) - epsilon)
        return false;
    if (pi.y() > qMax(pn.y(), pp.y()) + epsilon)
        return false;

    return distanceToLine(pi, QLineF(pn, pp)) <= epsilon;
}

bool pointCanBeRemovedCubic(const QPolygonF &poly, int index, bool isClosed)
{
    qreal epsilon = 10; // 10 точек на сглаживание. погрешность

    const int pSize = poly.size();
    if (pSize < 2 || index < 0 || index >= pSize)
        return false;

    int pInd1 = index - 1;
    int pInd2 = index - 2;
    int nInd1 = index + 1;
    int nInd2 = index + 2;

    if (pSize == 2) {
        nInd1 = nInd1 >= pSize ? nInd1 - pSize : nInd1;
        return QLineF(poly[index], poly[nInd1]).length() < epsilon;
    }

    if (isClosed) {
        pInd1 = pInd1 < 0 ? pInd1 + pSize : pInd1;
        pInd2 = pInd2 < 0 ? pInd2 + pSize : pInd2;
        nInd1 = nInd1 >= pSize ? nInd1 - pSize : nInd1;
        nInd2 = nInd2 >= pSize ? nInd2 - pSize : nInd2;

    }
    else {
        if (index == 0)
            return QLineF(poly[index], poly[nInd1]).length() < epsilon;
        if (index == pSize - 1)
            return QLineF(poly[index], poly[pInd1]).length() < epsilon;

        pInd2 = pInd2 < 0 ? 0 : pInd2;
        nInd2 = nInd2 >= pSize ? nInd1 : nInd2;
    }

    QLineF cp = updateEdge(poly, pInd2, pInd1, nInd1, nInd2);

    return distanceToCubic(poly[index], poly[pInd1], poly[nInd1], cp.p1(), cp.p2()) < epsilon;
}

bool nearControlPoint(const QPolygonF &poly, const QPointF &point, int &num, qreal controlR)
{
    int k = -1;
    double mindist = controlR * controlR; // минимальное расстояние до полигона
    for (int i = 0; i < poly.size(); ++i) {
        double dist = lengthR2Squared(poly.at(i) - point);
        if (dist < mindist) {
            mindist = dist;
            k = i;
        }
    }
    num = k;
    return k == -1 ? false : true;
}

bool nearEdgePoint(const QPolygonF &poly, const QPointF &point, int &num, bool closed, qreal maxdist)
{
    if (poly.size() < 2)
        return false;

    int k = -1;
    qreal mindist = maxdist;
    for (int i = 1; i < poly.size(); ++i) {
        QLineF l(poly.at(i - 1), poly.at(i));
        qreal len = distToSegment(point, l);
        if (len < mindist) {
            mindist = len;
            k = i;
        }
    }
    if (closed) {
        QLineF l(poly.last(), poly.first());
        qreal len = distToSegment(point, l);
        if (len < mindist) {
            mindist = len;
            k = poly.size();
        }
    }
    num = k;
    return k == -1 ? false : true;
}

bool nearEdgePointCubic(const QPolygonF &poly, const QPointF &point, int &num, bool closed, qreal maxdist)
{
    if (poly.size() < 2)
        return false;
    if (poly.size() == 2)
        return nearEdgePoint(poly, point, num, maxdist);

    QList<QLineF> cp = generateCubicSpline(poly, closed);

    qreal mindist = maxdist;
    int k = -1;
    int count = poly.size() - 1;
    for (int i = 0; i < count; ++i) {
        QLineF l = cp.at(i);
        qreal dist = distanceToCubic(point, poly.at(i), poly.at(i + 1), l.p1(), l.p2());
        if (dist < mindist) {
            mindist = dist;
            k = i + 1;
        }
    }
    if (closed) {
        QLineF l = cp.last();
        qreal dist = distanceToCubic(point, poly.last(), poly.first(), l.p1(), l.p2());
        if (dist < mindist) {
            mindist = dist;
            k = poly.size();
        }
    }
    num = k;
    return k == -1 ? false : true;
}

QVector<QPolygonF> calcPath(const QPolygonF &metric, const QVector<LineElement> &elements)
{
    QVector<QPolygonF> result;

    QPolygonF part;
    int prevInd = 0;
    for (QVector<LineElement>::const_iterator it = elements.begin(); it != elements.end(); ++it) {
        const LineElement &elem = *it;

        if (elem.rect.size() != 4)
            continue;

        double r = lengthR2(elem.rect.at(0) - elem.rect.at(1)) * 0.5;

        double leftVal  = lengthR2(metric.at(elem.ind) - elem.pos);
        double rightVal = lengthR2(metric.at(elem.ind + 1) - elem.pos);

        QPointF pLeft, pRight;
        int left  = (leftVal  > r) ?
                    currentIndexMin(metric, elem.rect, elem.ind, pLeft) :
                    nextIndexMin(metric, elem.rect, elem.ind, pLeft);
        int right = (rightVal > r) ?
                    currentIndexMax(metric, elem.rect, elem.ind, pRight) :
                    nextIndexMax(metric, elem.rect, elem.ind, pRight);

        for (int i = prevInd; i <= left; ++i)
            part.append(metric.at(i));
        part.append(pLeft);

        result.append(part);

        part.clear();
        part.append(pRight);
        prevInd = right;
    }

    int n = metric.size();
    for (int i = prevInd; i < n; ++i)
        part.append(metric.at(i));

    result.append(part);

    return result;
}

QPointF calcPoint(const QPolygonF &p, const QVector<qreal> &param, qreal length, qreal t, QLineF *l, int *number)
{
    if (qFuzzyIsNull(t)) {
        if (l)
            *l = QLineF(p.first(), p.at(1));
        return p.first();
    }
    if (qFuzzyIsNull(t - 1)) {
        if (l)
            *l = QLineF(p.at(p.size() - 2), p.last());
        return p.last();
    }

    int k = 0;
    for (int i = 0; i < param.size(); ++i)
        if (t < param.at(i)) {
            k = i;
            break;
        }

    qreal delta = k == 0 ? 0 : param.at(k - 1);

    qreal dt = t - delta;
    QPointF vect = p.at(k + 1) - p.at(k);
    vect /= lengthR2(vect);

    if (l)
        *l = QLineF(p.at(k), p.at(k + 1));
    if (number)
        *number = k;

    return QPointF(p.at(k) + vect * dt * length);
}

QVector<qreal> lineParams(const QPolygonF &metric, qreal &length, bool isClosed)
{
    return isClosed ? lineParamsClosed(metric, length) :  lineParams(metric, length);
}

QVector<qreal> lineParams(const QPolygonF &metric, qreal &length)
{
    QVector<qreal> param;
    length = 0;

    int metricSize = metric.size();
    if (metricSize < 2)
        return param;

    param.reserve(metricSize - 1);
    for (int i = 1; i < metricSize; ++i) {
        length += lengthR2(metric.at(i) - metric.at(i - 1));
        param.append(length);
    }

    for (int i = 0; i < param.size(); ++i)
        param[i] /= length;

    return param;
}

QVector<qreal> lineParamsClosed(const QPolygonF &metric, qreal &length)
{
    QVector<qreal> param;
    length = 0;

    int metricSize = metric.size();
    if (metricSize < 2)
        return param;

    param.reserve(metricSize);
    for (int i = 1; i < metricSize; ++i) {
        length += lengthR2(metric.at(i) - metric.at(i - 1));
        param.append(length);
    }
    length += lengthR2(metric.first() - metric.last());
    param.append(length);

    for (int i = 0; i < param.size(); ++i)
        param[i] /= length;

    return param;
}

QVector<QPainterPath> calcPathCubic(const QPolygonF &cubic, const QVector<SplineElement> &elements)
{
    QVector<QPainterPath> result;

    int prevInd = 0;
    QPolygonF prevPart;
    prevPart << cubic.at(0) << cubic.at(1) << cubic.at(2) << cubic.at(3);
    for (QVector<SplineElement>::const_iterator it = elements.begin(); it != elements.end(); ++it) {
        const SplineElement &elem = *it;

        if (elem.rect.size() != 4)
            continue;

        QList<QPair<int, qreal> > pos  = caclRectBoundsOnCubic(cubic, elem.rect, elem.ind, elem.param);
        QPair<int, qreal> minVal = pos.first();
        QPair<int, qreal> maxVal = pos.last();

        int number = 0;
        QPainterPath part;
        if (prevInd == minVal.first) {
            QPolygonF tmp;
            subcurveBezier(prevPart.at(0), prevPart.at(1), prevPart.at(2), prevPart.at(3), minVal.second, &tmp);
            part.moveTo(tmp.at(0));
            part.cubicTo(tmp.at(1), tmp.at(2), tmp.at(3));
        }
        else {
            part.moveTo(prevPart.at(0));
            part.cubicTo(prevPart.at(1), prevPart.at(2), prevPart.at(3));
            for (int i = prevInd + 1; i < minVal.first; ++i) {
                int n = (i + 1) * 3;
                part.cubicTo(cubic.at(n - 2), cubic.at(n - 1), cubic.at(n));
            }

            number = (minVal.first + 1) * 3;
            QPolygonF tmp;
            subcurveBezier(cubic.at(number - 3), cubic.at(number - 2), cubic.at(number - 1), cubic.at(number), minVal.second, &tmp);
            part.cubicTo(tmp.at(1), tmp.at(2), tmp.at(3));
        }
        result.append(part);

        prevInd = maxVal.first;

        number = (maxVal.first + 1) * 3;
        subcurveBezier(cubic.at(number - 3), cubic.at(number - 2), cubic.at(number - 1), cubic.at(number), maxVal.second, NULL, &prevPart);
    }

    int number = (prevInd + 1) * 3;
    QPainterPath part;
    part.moveTo(prevPart.at(0));
    part.cubicTo(prevPart.at(1), prevPart.at(2), prevPart.at(3));
    int n = cubic.size();
    for (int i = number + 3; i < n; i += 3)
        part.cubicTo(cubic.at(i - 2), cubic.at(i - 1), cubic.at(i));
    result.append(part);

    return result;
}

QPointF calcPointCubic(const QPolygonF &p, const QVector<qreal> &param, qreal length, qreal t, qreal *partT, QPolygonF *l, int *number)
{
    if (qFuzzyIsNull(t)) {
        if (l)
            *l = QPolygonF() << p.first() << p.at(1) << p.at(2) << p.at(3);
        return p.first();
    }
    if (qFuzzyIsNull(t - 1)) {
        if (l) {
            int n = p.size();
            *l = QPolygonF() << p.at(n - 4) << p.at(n - 3) << p.at(n - 2) << p.last();
        }
        return p.last();
    }

    int k = 0;
    for (int i = 0; i < param.size(); ++i)
        if (t < param.at(i)) {
            k = i;
            break;
        }

    qreal delta = k == 0 ? 0 : param.at(k - 1);
    qreal dt = t - delta;

    int num = (k + 1) * 3;
    QPolygonF tmp;
    tmp << p.at(num - 3) << p.at(num - 2) << p.at(num - 1) << p.at(num);
    if (l)
        *l = tmp;

    QPainterPath vect;
    vect.moveTo(tmp.first());
    vect.cubicTo(tmp.at(1), tmp.at(2), tmp.last());

    qreal partL = vect.length();
    qreal dl = length * dt;

    qreal tmpT = dl / partL;
    if (partT)
        *partT = tmpT;
    if (number)
        *number = k;
    return vect.pointAtPercent(tmpT);
}

QVector<qreal> cubicParams(const QPolygonF &cubic, qreal &length)
{
    length = 0;
    QVector<qreal> param;

    int cubicSize = cubic.size();
    if (cubicSize < 4)
        return param;

    param.reserve((cubicSize - 1) / 3);

    QPainterPath path;
    path.moveTo(cubic.first());
    for (int i = 3; i < cubic.size(); i += 3) {
        path.cubicTo(cubic.at(i - 2), cubic.at(i - 1), cubic.at(i));
        param.append(path.length());
    }

    length = param.last();
    for (int i = 0; i < param.size(); ++i)
        param[i] /= length;

    return param;
}

// -----------------------------------------------------------------------------------------------------------------------

QList<QPolygonF> triang(const QPolygonF &p)
{
    QList<QPolygonF> list;
    if (p.size() < 3)
        return list;
    if (p.size() == 3) {
        list.append(p);
        return list;
    }

    QStack<QPolygonF> stack;
    stack.push(p);
    while (!stack.isEmpty()) {
        QPolygonF current = stack.pop();
        if (current.size() < 3)
            continue;
        if (current.size() == 3)
            list.append(current);
        else {
            QPolygonF temp;
            temp = current;

            int id = findConvexVertex(temp);
            int indD = findIntrudingVertex(temp, id);
            if (indD == -1) {        // нет вторгающихся вершин
                int indC = (id + 1) % temp.size();

                id = --id % temp.size();

                QPolygonF q = split(temp, id, indC);
                stack.push(temp);
                stack.push(q);
            } else {                  // d - вторгающаяся вершина
                QPolygonF q = split(temp, id, indD);
                stack.push(q);
                stack.push(temp);
            }
        }
    }

    return list;
}

bool pointInTriangle(QPointF p, QPointF a, QPointF b, QPointF c)
{
    return (
                (classify(p, a, b) != 1) &&
                (classify(p, b, c) != 1) &&
                (classify(p, c, a) != 1)
                );
}

bool pointInTriangle(const QPointF &p, const QPolygonF &poly)
{
    if (poly.size() != 3)
        return false;
    return pointInTriangle(p, poly.first(), poly.at(1), poly.at(2));
}

// -----------------------------------------------------------------------------------------------------------------------

QImage generateTransformImage(const QImage &img, const QPointF &start, const QTransform &transform, const QTransform &backTransform, QPointF *newStart, qreal opacity, bool smoothed)
{
    // FIXME: jumping image position
    QRectF rect(start, img.size());
    rect = transform.mapRect(rect);

    QImage newImg(rect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    newImg.fill(Qt::transparent);

    QPainter p(&newImg);
    p.setOpacity(opacity);
    if (smoothed)
        p.setRenderHints(QPainter::SmoothPixmapTransform);

    p.setTransform(transform);
    p.drawImage(start + QPointF(backTransform.dx(), backTransform.dy()) - backTransform.map(rect.topLeft()), img);
    p.resetTransform();

    p.end();

    if (newStart)
        *newStart = rect.topLeft();

    return newImg;
}

QPointF positionFromRectTransform(const QPointF &start, const QSize &size, const QTransform &transform)
{
    return transform.mapRect(QRectF(start, size)).topLeft();
}

void changeColorImageByPixel(QImage &im, QColor c)
{
    if (im.colorCount() != 0)
        return;

    if (im.hasAlphaChannel()) {
        for (int y = 0; y < im.height(); ++y) {
            QRgb* line = reinterpret_cast< QRgb* >(im.scanLine(y));
            for (int x = 0; x < im.width(); ++x)
                line[x] = qRgba(c.red(), c.green(), c.blue(), qAlpha(line[x]));
        }
    }
    else {
        for (int y = 0; y < im.height(); ++y) {
            QRgb* line = reinterpret_cast< QRgb* >(im.scanLine(y));
            for (int x = 0; x < im.width(); ++x)
                line[x] = qRgb(c.red(), c.green(), c.blue());
        }
    }
}


void changeColorImageByPainter(QImage &im, QColor c)
{
    QPainter p(&im);
    p.setCompositionMode(QPainter::CompositionMode_SourceAtop);

    p.fillRect(QRect(QPoint(), im.size()), c);
    p.end();
}


void ConvertColor::emptyColor(QImage *, QVariantMap )
{
    return;
}

void ConvertColor::hsvColor(QImage *img, QVariantMap options)
{
//    qreal hue = options.value("hue", 0).toReal();
    qreal sat = options.value("saturation", 0).toReal();
    qreal val = options.value("value", 0).toReal();
    if (qFuzzyIsNull(sat) && qFuzzyIsNull(val))
        return;

    int h = 0, s = 0, v = 0, a = 0;
    QColor c;
    if (img->colorCount() == 0) {
        int wh = img->width() * img->height();
        QRgb *data = (QRgb*)img->bits();
        if (img->hasAlphaChannel())
            for (int i = 0; i < wh; ++i) {
                QColor::fromRgba(data[i]).getHsv(&h, &s, &v, &a);
//                h = hue > 0 ? h + (360 - h) * hue : h + h * hue;
                s = sat > 0 ? s + (255 - s) * sat : s + s * sat;
                v = val > 0 ? v + (255 - v) * val : v + v * val;
                c.setHsv(h, s, v, a);
                data[i] = c.rgba();
            }
        else
            for (int i = 0; i < wh; ++i) {
                QColor::fromRgb(data[i]).getHsv(&h, &s, &v, &a);
//                h = hue > 0 ? h + (360 - h) * hue : h + h * hue;
                s = sat > 0 ? s + (255 - s) * sat : s + s * sat;
                v = val > 0 ? v + (255 - v) * val : v + v * val;
                c.setHsv(h, s, v);
                data[i] = c.rgb();
            }
    }
    else {
        QVector<QRgb> colors = img->colorTable();
        for (int i = 0; i < img->colorCount(); ++i) {
            QColor::fromRgb(colors[i]).getHsv(&h, &s, &v);
//            h = h > 0 ? h + (360 - h) * hue : h + h * hue;
            s = s > 0 ? s + (255 - s) * sat : s + s * sat;
            v = v > 0 ? v + (255 - v) * val : v + v * val;
            c.setHsv(h, s, v);
            colors[i] = c.rgb();
        }
    }
}

void ConvertColor::invertedColor(QImage *img, QVariantMap /*options*/)
{
    int a = 0, r = 0, g = 0, b = 0;
    if (img->colorCount() == 0) {
        int wh = img->width() * img->height();
        QRgb *data = (QRgb*)img->bits();
        if (img->hasAlphaChannel())
            for (int i = 0; i < wh; ++i) {
                QColor::fromRgba(data[i]).getRgb(&r, &g, &b, &a);
                data[i] = qRgba(255 - r, 255 - g, 255 - b, a);
            }
        else
            for (int i = 0; i < wh; ++i) {
                QColor::fromRgb(data[i]).getRgb(&r, &g, &b);
                data[i] = qRgb(255 - r, 255 - g, 255 - b);
            }
    }
    else {
        QVector<QRgb> colors = img->colorTable();
        for (int i = 0; i < img->colorCount(); ++i) {
            QColor::fromRgb(colors[i]).getRgb(&r, &g, &b, &a);
            colors[i] = qRgba(255 - r, 255 - g, 255 - b, a);
        }
    }
}

void ConvertColor::invertedHsvColor(QImage *img, QVariantMap options)
{
    qreal sat = options.value("saturation", 0).toReal();
    qreal val = options.value("value", 0).toReal();

    int h = 0, s = 0, v = 0, a = 0, r = 0, g = 0, b = 0;
    QColor c;
    if (img->colorCount() == 0) {
        int wh = img->width() * img->height();
        QRgb *data = (QRgb*)img->bits();
        if (img->hasAlphaChannel())
            for (int i = 0; i < wh; ++i) {
                QColor::fromRgba(data[i]).getRgb(&r, &g, &b, &a);
                c = qRgba(255 - r, 255 - g, 255 - b, a);

                c.getHsv(&h, &s, &v, &a);
                s = sat > 0 ? s + (255 - s) * sat : s + s * sat;
                v = val > 0 ? v + (255 - v) * val : v + v * val;
                c.setHsv(h, s, v, a);
                data[i] = c.rgba();
            }
        else
            for (int i = 0; i < wh; ++i) {
                QColor::fromRgb(data[i]).getRgb(&r, &g, &b);
                c = qRgb(255 - r, 255 - g, 255 - b);

                c.getHsv(&h, &s, &v);
                s = sat > 0 ? s + (255 - s) * sat : s + s * sat;
                v = val > 0 ? v + (255 - v) * val : v + v * val;
                c.setHsv(h, s, v);

                data[i] = c.rgb();
            }
    }
    else {
        QVector<QRgb> colors = img->colorTable();
        for (int i = 0; i < img->colorCount(); ++i) {
            QColor::fromRgb(colors[i]).getRgb(&r, &g, &b, &a);
            c = qRgba(255 - r, 255 - g, 255 - b, a);

            c.getHsv(&h, &s, &v, &a);
            s = s > 0 ? s + (255 - s) * sat : s + s * sat;
            v = v > 0 ? v + (255 - v) * val : v + v * val;
            c.setHsv(h, s, v, a);

            colors[i] = c.rgba();
        }
    }
}

void ConvertColor::grayColor(QImage *img, QVariantMap /*options*/)
{
    if (img->colorCount() == 0) {
        if (img->hasAlphaChannel())
            for (int y = 0; y < img->height(); ++y) {
                QRgb* line = reinterpret_cast< QRgb* >(img->scanLine(y));
                for (int x = 0; x < img->width(); x++) {
                    int g = qGray(line[x]);
                    line[x] = qRgba(g, g, g, qAlpha(line[x]));
                }
            }
        else
            for (int y = 0; y < img->height(); ++y) {
                QRgb* line = reinterpret_cast< QRgb* >(img->scanLine(y));
                for (int x = 0; x < img->width(); ++x) {
                    int g = qGray(line[x]);
                    line[x] = qRgb(g, g, g);
                }
            }
    }
    else {
        QVector<QRgb> colors = img->colorTable();
        for (int i = 0; i < img->colorCount(); ++i)
            colors[i] = qGray(colors[i]);
    }
}

// -----------------------------------------------------------------------------------------------------------------------

QRgb getPixel(const QImage &img, const QPointF &p)
{
    if (qFloor(p.x()) < 0 || qRound(p.x() + 0.5) > img.width() - 1 || qFloor(p.y()) < 0 || qRound(p.y() + 0.5) > img.height() - 1)
        return QRgb();
    return img.pixel(p.toPoint());
}

QRgb getPixelBilinear(const QImage &img, const QPointF &p)
{
    if (qFloor(p.x()) < 0 || qRound(p.x() + 0.5) > img.width() - 1 || qFloor(p.y()) < 0 || qRound(p.y() + 0.5) > img.height() - 1)
        return QRgb();

    int l = int(p.x());
    if (l < 0)
        l = 0;
    else if (l >= img.width() - 1)
        l = img.width() - 2;
    double u = p.x() - l;

    int c = int(p.y());
    if (c < 0)
        c = 0;
    else if (c >= img.height() - 1)
        c = img.height() - 2;
    double t = p.y() - c;

    /* Коэффициенты */
    double d1 = (1 - t) * (1 - u);
    double d2 = t * (1 - u);
    double d3 = t * u;
    double d4 = (1 - t) * u;

    /* Окрестные пиксели: a[i][j] */
    u_int p1 = img.pixel(l,   c);
    u_int p2 = img.pixel(l,   c+1);
    u_int p3 = img.pixel(l+1, c+1);
    u_int p4 = img.pixel(l+1, c);

    u_int a = u_int(qAlpha(p1) * d1 + qAlpha(p2) * d2 + qAlpha(p3) * d3 + qAlpha(p4) * d4);
    u_int r = u_int(  qRed(p1) * d1 + qRed(p2)   * d2 +   qRed(p3) * d3 +   qRed(p4) * d4);
    u_int g = u_int(qGreen(p1) * d1 + qGreen(p2) * d2 + qGreen(p3) * d3 + qGreen(p4) * d4);
    u_int b = u_int( qBlue(p1) * d1 + qBlue(p2)  * d2 +  qBlue(p3) * d3 +  qBlue(p4) * d4);

    return qRgba(r, g, b, a);
}

QImage imgFilter(const QImage &img, QPolygonF source, QPolygonF dest, pixelFunc f, QPolygonF insideS, QPolygonF insideD)
{
    // TODO: оптимизировать + уточнение размеров
    if (img.isNull())
        return QImage();

    if (source.size() < 3 || dest.size() < 3)
        return QImage();

    if (insideS.size() != insideD.size())
        return QImage();

    if (dest.size() > source.size())
        dest.remove(source.size(), dest.size() - source.size());
    if (source.size() != dest.size())
        return QImage();

    if (polyOrietation(source)) {
        changeOrientation(source);
        changeOrientation(dest);
    }

    QList<QPolygonF> triangles = triang(source);
    foreach (QPointF p, insideS) {
        for (int i = 0; i < triangles.size(); ++i) {
            if (pointInTriangle(p, triangles[i])) {
                QPolygonF temp = triangles[i];
                QPolygonF add = temp;
                add.replace(0, p);
                triangles.replace(i, add);
                add = temp;
                add.replace(1, p);
                triangles.insert(i, add);
                add = temp;
                add.replace(2, p);
                triangles.insert(i, add);
                break;
            }
        }
    }
    for (int i = 0; i < insideS.size(); ++i) {
        source.append(insideS[i]);
        dest.append(insideD[i]);
    }

    QPointF move = dest.boundingRect().topLeft();
    QImage newImg(dest.boundingRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    newImg.fill(Qt::transparent);
    foreach (QPolygonF triangle, triangles) {
        if (triangle.size() < 3)
            continue;
        QPolygonF origin;
        for (int i = 0; i < 3; ++i)
            origin.append(dest.at(source.indexOf(triangle.at(i))));
        getTriangImg(img, triangle, origin, move, newImg, f);
    }
    return newImg;
}

// ------------------------------------------------------------------------


bool solveInverseProblem(const QPointF &latLonS, const QPointF &latLonE, qreal &dist, qreal &firstDir, qreal &secondDir)
{
    double accuracy = 1e-14;

    double a = 6378137;
    // TODO: какой взять b
    //    qreal b = (1 - f) * a;
    double b = 6356752.314245179;
    //    qreal b = 6378137;
    double f = 1. / 298.257223563;

    double L = latLonE.y() - latLonS.y();
    double U_1 = qAtan((1 - f) * qTan(latLonS.x()));
    double U_2 = qAtan((1 - f) * qTan(latLonE.x()));

    double sinU1 = qSin(U_1);
    double cosU1 = qCos(U_1);
    double sinU2 = qSin(U_2);
    double cosU2 = qCos(U_2);

    double lambda = L;
    double lambdaP = 0.0;
    int lim = 100;

    double cosSqAlpha, sinSigma, cos2SigmaM, cosSigma, sigma, sinLambda, cosLambda;
    bool equatorial = false;
    do {
        sinLambda = qSin(lambda);
        cosLambda = qCos(lambda);

        double t_1 = cosU2 * sinLambda;
        double t_2 = cosU1 * sinU2 - sinU1 * cosU2 * cosLambda;
        sinSigma = sqrt(t_1 * t_1 + t_2 * t_2);
        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;

        if (qFuzzyIsNull(sinSigma)) { // mark3: sinSigma == 0; end point is equal to start or diametrically opposite
//            qDebug() << "sinSigma is zero";

            // co-incident
            dist = 0.;
            firstDir = 0.;
            secondDir = 0.;
            return true;
        }

        sigma = atan2(sinSigma, cosSigma);

        double sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;

        cosSqAlpha = 1 - sinAlpha * sinAlpha;

        if (qFuzzyIsNull(cosSqAlpha)) { // mark4
            if (!equatorial)
                qDebug() << "equatorial line";
            equatorial = true;
        }

        cos2SigmaM = cosSigma - 2 * sinU1 * sinU2 / cosSqAlpha;
//        if (cos2SigmaM == NAN) cos2SigmaM = 0; // equatirial line: cosSqAlpha == 0

        double C = 0.;
        lambdaP = lambda;
        if (equatorial) {
            C = 0.;
            cos2SigmaM = -1;
            lambda = L + f * sinAlpha * sigma;
        }
        else {
            C = f / 16 * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));
            cos2SigmaM = cosSigma - 2 * sinU1 * sinU2 / cosSqAlpha;
            lambda = L + (1 - C) * f * sinAlpha * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1 + 2 * cos2SigmaM)));
        }

        if (qAbs(lambda) > M_PI) {
            qDebug() << "two nearly antipodal points" << degToRad(latLonS) << degToRad(latLonE);
//            return antipodalInverseProblem(L, U_1, U_2, f, a, b, cosSigma, sinSigma);
            return false;
        }

    } while (qAbs(lambda - lambdaP) > accuracy && --lim > 0);

    if (lim == 0) {
        qDebug() << "formula failed to converge";
        return false;
    }

    double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
    double A = 1 + uSq / 16384. * (4096 + uSq * (-768 + uSq * (320 -175 * uSq)));
    double B = uSq / 1024. * (256 + uSq * (-128 + uSq * (74 -47 * uSq)));
    double dSigma = B * sinSigma *
            (cos2SigmaM + B / 4. * (cosSigma * (-1 + 2 * cos2SigmaM)
                                     - B / 6. * cos2SigmaM * (-3 + 4 * sinSigma * sinSigma) * (-3 + 4 * cos2SigmaM * cos2SigmaM)));

    dist = b * A * (sigma - dSigma);

    secondDir = qAtan2(cosU2 * sinLambda, ( cosU1 * sinU2 - sinU1 * cosU2 * cosLambda));
    firstDir = qAtan2(cosU1 * sinLambda, (-sinU1 * cosU2 + cosU1 * sinU2 * cosLambda));

    return true;
}

bool solveInverseProblem(qreal lat1, qreal lon1, qreal lat2, qreal lon2, qreal &dist, qreal &firstDir, qreal &secondDir)
{
    return solveInverseProblem(QPointF(lat1, lon1), QPointF(lat2, lon2), dist, firstDir, secondDir);
}

void solveDirectProblem(qreal lat, qreal lon, qreal brng, qreal dist, QPointF &endPoint, qreal &secondDir)
{
    double accuracy = 1e-12;
    double a = 6378137;
    // TODO: какой взять b
    //    qreal b = (1 - f) * a;
    double b = 6356752.314245179;
    //    qreal b = 6378137;
    double f = 1. / 298.257223563;

    double s = dist;
    double alpha1 = brng;
    double sinAlpha1 = qSin(alpha1);
    double cosAlpha1 = qCos(alpha1);

    double tanU1 = (1. - f) * qTan(lat);
    double cosU1 = 1. / qSqrt(1 + tanU1 * tanU1);
    double sinU1 = tanU1 * cosU1;

    double sigma1 = qAtan2(tanU1, cosAlpha1);
    double sinAlpha = cosU1 * sinAlpha1;
    double cosSqAlpha = 1. - sinAlpha * sinAlpha;

    double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
    double A = 1 + uSq / 16384.* (4096 + uSq * (-768 + uSq * (320 - 175 * uSq)));
    double B = uSq / 1024. * (256 + uSq * (-128 + uSq * (74 - 47 * uSq)));

    double cos2SigmaM = 0.0;
    double sinSigma = 0.0;
    double cosSigma = 0.0;

    double sigma = s / (b * A);
    double sigmaPrev = 2 * M_PI;
    while (qAbs(sigma - sigmaPrev) > accuracy) {
        cos2SigmaM = qCos(2 * sigma1 + sigma);

        sinSigma = qSin(sigma);
        cosSigma = qCos(sigma);

        qreal dSigma = B * sinSigma * (cos2SigmaM +
                                       B / 4. * (cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM) -
                                                   B / 6. * cos2SigmaM * (-3 + 4 * sinSigma * sinSigma) * (-3 + 4 * cos2SigmaM * cos2SigmaM)));
        sigmaPrev = sigma;
        sigma = s / (b * A) + dSigma;
    }

    double temp = sinU1 * sinSigma - cosU1 * cosSigma * cosAlpha1;
    double lat2 = qAtan2(sinU1 * cosSigma + cosU1 * sinSigma * cosAlpha1,
                       (1 - f) * qSqrt(sinAlpha * sinAlpha + temp * temp));

    double lambda = qAtan2(sinSigma * sinAlpha1, cosU1 * cosSigma - sinU1 * sinSigma * cosAlpha1);
    double C = f / 16. * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));

    double L = lambda - (1 - C) * f * sinAlpha
            * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM)));

    double lon2 = fmod(lon + L + 3 * M_PI, 2 * M_PI) - M_PI;

    secondDir = qAtan2(sinAlpha, -temp);
    endPoint.setX(lat2);
    endPoint.setY(lon2);
}

void solveDirectProblem(const QPointF &latLon, qreal brng, qreal dist, QPointF &endPoint, qreal &secondDir)
{
    solveDirectProblem(latLon.x(), latLon.y(), brng, dist, endPoint, secondDir);
}

qreal loxoLength(const QPolygonF &line)
{
    qreal s = 0;
    for (int i = 1; i < line.size(); ++i) {
        QPointF start = line.at(i - 1);
        QPointF end = line.at(i);

        QPointF geoS = degToRad(TileSystem::metersToLatLon(start));
        QPointF geoE = degToRad(TileSystem::metersToLatLon(end));

        qreal alpha = polarAngle(end - start);
        qreal dt = (1 - 0.25 * ParamsWGS::e2) * (geoE.x() - geoS.x()) - 0.375 * ParamsWGS::e2 * (qSin(2 * geoE.x()) - qSin(2 * geoS.x()));
        s += ParamsWGS::a / qCos(alpha) * dt;
    }
    return s;
}

qreal geoLoxoLength(const QPolygonF &line)
{
    qreal s = 0;
    for (int i = 1; i < line.size(); ++i) {
        QPointF start = line.at(i - 1);
        QPointF end = line.at(i);

        QPointF geoS = degToRad(start);
        QPointF geoE = degToRad(end);

        qreal alpha = polarAngle(TileSystem::latLonToMeters(end) - TileSystem::latLonToMeters(start));
        qreal dt = (1 - 0.25 * ParamsWGS::e2) * (geoE.x() - geoS.x()) - 0.375 * ParamsWGS::e2 * (qSin(2 * geoE.x()) - qSin(2 * geoS.x()));
        s += ParamsWGS::a / qCos(alpha) * dt;
    }
    return s;
}

qreal orthoLength(const QPolygonF &line)
{
    if (line.size() < 2)
        return 0;

    qreal length = 0;
    for (int i = 1; i < line.size(); ++i) {
        QPointF geoS = degToRad(TileSystem::metersToLatLon(line.at(i-1)));
        QPointF geoE = degToRad(TileSystem::metersToLatLon(line.at(i)));

        // защита от "two nearly antipodal points"
        qreal dx = geoS.y() - geoE.y();
        if (qAbs(dx) > M_PI) {
            if (dx < 0)
                geoE.ry() -= Pi2;
            else
                geoS.ry() -= Pi2;
        }


        qreal len = 0;
        qreal fD = 0;
        qreal sD = 0;
        solveInverseProblem(geoS, geoE, len, fD, sD);

        length += len;
    }
    return length;
}

qreal geoOrthoLength(const QPolygonF &line)
{
    if (line.size() < 2)
        return 0;

    qreal length = 0;
    for (int i = 1; i < line.size(); ++i) {
        QPointF geoS = degToRad(line.at(i-1));
        QPointF geoE = degToRad(line.at(i));

        // защита от "two nearly antipodal points"
        qreal dx = geoS.y() - geoE.y();
        if (qAbs(dx) > M_PI) {
            if (dx < 0)
                geoE.ry() -= Pi2;
            else
                geoS.ry() -= Pi2;
        }

        qreal len = 0;
        qreal fD = 0;
        qreal sD = 0;
        solveInverseProblem(geoS, geoE, len, fD, sD);

        length += len;
    }
    return length;
}

// ---------------------------------------------

QList<QList<QPair<QPointF, QString> > > clusterization(const QList<QPair<QPointF, QString> > &selectedObjects, const MapCamera *camera, int gridSize)
{
    if (!camera || selectedObjects.isEmpty())
        return QList<QList<QPair<QPointF, QString> > >();

    typedef QPair<QPointF, QString> LocalObjects;
    typedef QPair<int, int> IntKey;
    QMap<IntKey, QList<QPair<QPointF, QString> > > cache;

    QPointF first = camera->toWorld(QPointF());
    QPointF second = camera->toWorld(QPointF(gridSize, -gridSize));

    qreal stepx = second.x() - first.x();
    qreal stepy = second.y() - first.y();

    for (QListIterator<LocalObjects> it(selectedObjects); it.hasNext(); ) {
        LocalObjects mo = it.next();
        const QPointF &pos = mo.first;
        IntKey key = qMakePair(int(pos.x() / stepx), int(pos.y() / stepy));
        cache[key].append(mo);
    }

    return cache.values();
}

// ---------------------------------------------

QImage genSvg(QSvgRenderer &rscRender, const QString &uid, const QSizeF &size, QColor bg)
{
    if (!size.isValid())
        return QImage();

    QImage img(size.toSize(), QImage::Format_ARGB32_Premultiplied);
    img.fill(bg.rgba());

    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    if (rscRender.elementExists(uid))
        rscRender.render(&painter, uid, img.rect());

    return img;
}

QImage genSvgTransform(QSvgRenderer &render, const QString &uid, const QSizeF &size, const QPointF &start, const QTransform &transform, const QTransform &backTransform, QPointF *newStart, QColor bg)
{
    if (!size.isValid())
        return QImage();

    if (!render.elementExists(uid))
        return QImage();

    QRectF rect = transform.mapRect(QRectF(start, size));
    QImage img(rect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    img.fill(bg.rgba());

    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setTransform(transform);

    QPointF p = start + QPointF(backTransform.dx(), backTransform.dy()) - backTransform.map(rect.topLeft());
    render.render(&painter, uid, QRectF(p, size));

    if (newStart)
        *newStart = rect.topLeft();

    return img;
}

QImage genText(QTextDocument &txt, const QSizeF &size, QColor bg)
{
    if (!size.isValid())
        return QImage();

    QImage img(size.toSize(), QImage::Format_ARGB32_Premultiplied);
    img.fill(bg.rgba());

    QPainter painter(&img);
    txt.drawContents(&painter);

    return img;
}

QImage genText(const QString &txt, const QFont &font, const QSizeF &size, QColor bg)
{
    if (!size.isValid())
        return QImage();

    QImage img(size.toSize(), QImage::Format_ARGB32_Premultiplied);
    img.fill(bg.rgba());

    QPainter painter(&img);
    painter.setFont(font);
    painter.drawText(img.rect(), Qt::AlignCenter, txt);

    return img;
}

// ---------------------------------------------

} // namespace minigis

// ---------------------------------------------
