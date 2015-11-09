#include <qmath.h>

#include <QStack>

#include "core/mapmath.h"
#include "core/mapdefs.h"
#include "drawer/mapdrawer.h"

#include "mapdrawercommon.h"

QImage minigis::svgGenerator(QSvgRenderer &render, const QPointF &p, const qreal &ang, const QString &cls,
                             const qreal &svgSize, const quint8 &anchorX, const quint8 &anchorY, QPointF *start)
{
    QRectF rgn = svgRect(render, cls, p, svgSize);

    QPointF mid(rgn.left(), rgn.top());
    if (anchorX != 0 || anchorY != 0) {
        QPointF anchor(rgn.width() * anchorX * 0.01, rgn.height() * anchorY * 0.01);
        rgn.moveTopLeft(rgn.topLeft() - anchor);
        mid = QPointF(rgn.left() + anchor.x(), rgn.top() + anchor.y());
    }
    QTransform tr;
    tr.translate(mid.x(), mid.y()).rotate(ang).translate(-mid.x(), -mid.y());

    return svgGenerator(render, cls, rgn, tr, tr.inverted(), start);
}

QImage minigis::svgGenerator(QSvgRenderer &render, const QString &cls, const QRectF &rgn, const QTransform &tr, const QTransform &invertedTr, QPointF *start)
{
    if (tr.isIdentity()) {
        if (start)
            *start = rgn.topLeft();
        return genSvg(render, cls, rgn.size());
    }
    return genSvgTransform(render, cls, rgn.size(), rgn.topLeft(), tr, invertedTr, start);
//    return generateTransformImage(genSvg(render, cls, rgn.size()), rgn.topLeft(), tr, invertedTr, start);
}

QRectF minigis::svgRect(QSvgRenderer &render, const QString &cls, const QPointF &p, const qreal &svgSize)
{
    QSizeF element = render.boundsOnElement(cls).size(); // типовой размер
    if (element.width() > element.height())
        element = QSizeF(SVGsize, qreal(element.height() / element.width()) * SVGsize);
    else
        element = QSizeF(qreal(element.width() / element.height()) * SVGsize, SVGsize);

    QRectF rgn(
                p,
                element * svgSize
                );
    return rgn;
}

void minigis::getParentPos(MapObject const *object, MapCamera const *camera, QPointF &mid, qreal &angle)
{
    QStack<MapObject const *> stack;
    MapObject const *obj = object;

    if (object->parentObject())
        obj = object->parentObject();
    else {
        mid = QPointF(0, 0);
        angle = 0;
        return;
    }

    while (obj->parentObject()) {
        stack.append(obj);
        obj = obj->parentObject();
    }
    if (!obj->drawer()) {
        // TODO: временное неправильное решение
        mid = QPointF(-100000, -100000);
        return;
    }

    mid = camera->toScreen(obj->drawer()->pos(obj));
    angle = obj->drawer()->angle(obj, camera);
    while (!stack.isEmpty()) {
        MapObject const *tmp = stack.pop();
        mid += QTransform().rotate(angle).map(tmp->drawer()->pos(tmp));
        angle += tmp->drawer()->angle(tmp, camera);
    }
}

QPolygonF minigis::shiftPolygonSimple(const QPolygonF &origin, qreal delta, bool closed)
{
    QPolygonF path = origin;

    QPolygonF norm;
    // ----------------- Поиск нормалей к оригинальному полигону
    int N = path.size();
    for (int i = 1; i < N; ++i) {
        QPointF vect = path.at(i) - path.at(i-1);
        double len = lengthR2(vect);
        if (qFuzzyIsNull(len)) {
            path.remove(i);
            --N;
            --i;
            continue;
        }
        vect /= len;
        norm.append(QPointF(vect.y() * delta, -vect.x() * delta));
    }
    // ----
    if (closed) {
        QPointF vect = path.first() - path.last();
        double len = lengthR2(vect);
        if (qFuzzyIsNull(len))
            path.remove(path.size() - 1);
        else {
            vect /= len;
            norm.append(QPointF(vect.y() * delta, -vect.x() * delta));
        }
    }
    // ------------------

    QVector<QLineF> lines;
    // -------------------------- Построение смещенных линий
    for (int i = 1; i < path.size(); ++i)
        lines.append(QLineF(path.at(i) + norm.at(i-1), path.at(i-1) + norm.at(i-1)));
    // ----
    if (closed)
        lines.append(QLineF(path.first() + norm.last(), path.last() + norm.last()));
    // ------------------

    QPolygonF shell;
    if (lines.isEmpty())
        return shell;

    // -------------------------- Построение смещенного полигона
    N = lines.size();
    for (int i = 1; i < N; ++i) {
        QPointF tmp;
        QLineF::IntersectType type = lines.at(i-1).intersect(lines.at(i), &tmp);
        double ang = lines.at(i-1).angleTo(lines.at(i));
        if (type != QLineF::NoIntersection)
            shell.append(tmp);
        else {
            if (qFuzzyCompare(ang, 180.))
                shell.append(lines.at(i).p2() - 2 * norm.at(i));
            shell.append(lines.at(i).p2());
        }
    }
    // ----
    if (closed) {
        QPointF tmp;
        QLineF::IntersectType type = lines.last().intersect(lines.first(), &tmp);
        double ang = lines.last().angleTo(lines.first());
        if (type != QLineF::NoIntersection)
            shell.append(tmp);
        else {
            if (qFuzzyCompare(ang, 180.))
                shell.append(lines.first().p2() - 2 * norm.first());
            shell.append(lines.first().p2());
        }
        shell.append(shell.first());
    }
    else {
        shell.prepend(lines.first().p2());
        shell.append(lines.last().p1());
    }
    // ------------------

    return shell;
}

QPolygonF minigis::shiftPolygonDifficult(const QPolygonF &origin, qreal delta, bool closed)
{
    if (qFuzzyIsNull(delta))
        return origin;

    QPolygonF path = origin;

    QPolygonF norm;
    // ----------------- Поиск нормалей к оригинальному полигону
    int N = path.size();
    for (int i = 1; i < N; ++i) {
        QPointF vect = path.at(i) - path.at(i-1);
        double len = lengthR2(vect);
        if (qFuzzyIsNull(len)) {
            path.remove(i);
            --N;
            --i;
            continue;
        }
        vect /= len;
        norm.append(QPointF(vect.y() * delta, -vect.x() * delta));
    }
    // ----
    if (closed) {
        QPointF vect = path.first() - path.last();
        double len = lengthR2(vect);
        if (qFuzzyIsNull(len))
            path.remove(path.size() - 1);
        else {
            vect /= len;
            norm.append(QPointF(vect.y() * delta, -vect.x() * delta));
        }
    }
    // ------------------

    QVector<QLineF> lines;
    // -------------------------- Построение смещенных линий
    for (int i = 1; i < path.size(); ++i)
        lines.append(QLineF(path.at(i) + norm.at(i-1), path.at(i-1) + norm.at(i-1)));
    // ----
    if (closed)
        lines.append(QLineF(path.first() + norm.last(), path.last() + norm.last()));
    // ------------------

    QPolygonF shell;
    if (lines.isEmpty())
        return shell;

    // -------------------------- Построение смещенного полигона
    N = lines.size();
    for (int i = 1; i < N; ++i) {
        QPointF tmp;
        QLineF::IntersectType type = lines.at(i-1).intersect(lines.at(i), &tmp);
        qreal ang = lines.at(i-1).angleTo(lines.at(i));
        if (type != QLineF::NoIntersection)
            shell.append(tmp);
        else {
            if (qFuzzyCompare(ang, qreal(180)))
                shell.append(lines.at(i).p2() - 2 * norm.at(i));
            shell.append(lines.at(i).p2());
        }
    }
    // ----
    if (closed) {
        QPointF tmp;
        QLineF::IntersectType type = lines.last().intersect(lines.first(), &tmp);
        qreal ang = lines.last().angleTo(lines.first());
        if (type != QLineF::NoIntersection)
            shell.append(tmp);
        else {
            if (qFuzzyCompare(ang, qreal(180)))
                shell.append(lines.first().p2() - 2 * norm.first());
            shell.append(lines.first().p2());
        }
        shell.append(shell.first());
    }
    else {
        shell.prepend(lines.first().p2());
        shell.append(lines.last().p1());
    }
    // ------------------

    // -------------------------- обрезание острых углов
    int k  = 0;
    N = lines.size();
    for (int i = 1; i < N; ++i) {
        double ang = lines.at(i-1).angleTo(lines.at(i));

        bool first  = (120 < ang && ang < 180 && delta < 0) || (180 < ang && ang < 240 && delta > 0);
        bool second = (120 < ang && ang < 180 && delta > 0) || (180 < ang && ang < 240 && delta < 0);
        if (first) {
            int num = closed ? 1 : 0;
            QPointF v = shell.at(i + k - num) - path.at(i);
            v /= lengthR2(v);
            QPointF start = path.at(i) + v * qAbs(delta);
            QLineF tmp(start, start + QPointF(-v.y(), v.x()));
            QPointF a;
            if (tmp.intersect(lines.at(i  ), &a) != QLineF::NoIntersection)
                shell.replace(i + k - num, a);
            if (tmp.intersect(lines.at(i-1), &a) != QLineF::NoIntersection)
                shell.insert(i + k - num, a);
            ++k;
        }
        else if (second) {
            // TODO: cut corner
        }
    }
    // ----
    if (closed) {
        double ang = lines.last().angleTo(lines.first());

        int num = lines.size();
        int shellNum = (num + k - 1) % shell.size();
        bool first  = (120 < ang && ang < 180 && delta < 0) || (180 < ang && ang < 240 && delta > 0);
        bool second = (180 < ang && ang < 240 && delta < 0) || (120 < ang && ang < 180 && delta > 0);
        if (first) {
            QPointF v = shell.at(shellNum) - path.at(num % path.size());
            v /= lengthR2(v);
            QPointF start = path.at(num % path.size()) + v * qAbs(delta);
            QLineF tmp(start, start + QPointF(-v.y(), v.x()));
            QPointF a;
            if (tmp.intersect(lines.first(), &a) != QLineF::NoIntersection)
                shell.replace(shellNum, a);
            if (tmp.intersect(lines.last() , &a) != QLineF::NoIntersection)
                shell.insert(shellNum, a);
        }
        else if (second) {
            // TODO: cut corner
        }
    }
    // ------------------

    return shell;
}

QPolygonF minigis::shiftPolygon(const QPolygonF &origin, qreal delta, bool direction, QList<int> *doubleDots)
{
    if (doubleDots)
        doubleDots->clear();

    delta = qAbs(delta);

    QVector<QLineF> lines;
    for (int i = 1; i < origin.size(); ++i) {
        QLineF l(origin.at(i - 1), origin.at(i));
        QLineF norm = l.normalVector();

        qreal len = lengthR2(l.p2() - l.p1());
        QPointF normVect = (norm.p2() - norm.p1()) / len;
        lines.append(l.translated(normVect * (direction ? -delta : delta)));
    }

    QPolygonF path;

    QVectorIterator<QLineF> it(lines);
    QLineF base = it.next();
    int pointNumber = 0;

    path.append(base.p1());
    while (it.hasNext()) {
        QLineF next = it.next();
        ++pointNumber;

        double ang = base.angleTo(next);
        bool side = ang < 180 ? direction : !direction;
        if (ang > 180)
            ang = 360 - ang;

        if (qFuzzyIsNull(ang)) { // "I" + // линия-продолжение
            path.append(base.p2());
            base.setP2(next.p2());
        }
        else if (qFuzzyIsNull(ang - 180)) { // "IV" ? // коллинеарная линия в обраную строную
            // TODO: mb we don't need this
            path.append(base.p2());
            path.append(next.p1());
            base = next;

            if (doubleDots)
                doubleDots->append(pointNumber);
        }
        else if (ang < 120) { // "II"
            QPointF p;
            base.intersect(next, &p);
            if (side) { // "A" + // линия снаружи
                path.append(p);
                base = next;
            }
            else { // "B" - // линия внутри
                // TODO: correct algo
                path.append(p);
                base = next;
            }
        }
        else { // "III"
            if (side) { // "A" + // линия снаружи с острым углом
                QPointF p;
                base.intersect(next, &p);

                QPointF start = origin.at(pointNumber);
                QPointF vect = p - start;
                vect *= delta / lengthR2(vect);
                QPointF norm(vect.y(), -vect.x());

                QLineF tmp(start + vect, start + vect + norm);

                base.intersect(tmp, &p);
                path.append(p);
                next.intersect(tmp, &p);
                path.append(p);

                base = next;

                if (doubleDots)
                    doubleDots->append(pointNumber);
            }
            else { // "B" - // линия внутри с острым углом
                // TODO: correct algo
                QPointF p;
                base.intersect(next, &p);
                path.append(p);
                base = next;
            }
        }
    }
    path.append(base.p2());

    return path;
}

bool minigis::rectIntersectPolyHollow(const QRectF &rect, const QPolygonF &poly, bool closed)
{
    foreach (QPointF p, poly)
        if (rect.contains(p))
            return true;

    QLineF top(rect.topLeft(), rect.topRight());
    QLineF right(rect.topRight(), rect.bottomRight());
    QLineF bottom(rect.bottomRight(), rect.bottomLeft());
    QLineF left(rect.bottomLeft(), rect.topLeft());

    int N = closed ? poly.size() + 1 : poly.size();
    for (int i = 1; i < N; ++i) {
        QLineF tmp(poly[i-1], poly[i % poly.size()]);
        QPointF a;
        if (top.intersect(tmp, &a)    == QLineF::BoundedIntersection)
            return true;
        if (bottom.intersect(tmp, &a) == QLineF::BoundedIntersection)
            return true;
        if (right.intersect(tmp, &a)  == QLineF::BoundedIntersection)
            return true;
        if (left.intersect(tmp, &a)   == QLineF::BoundedIntersection)
            return true;
    }

    return false;
}

bool minigis::rectIntersectPolyFull(const QRectF &rect, const QPolygonF &poly)
{
    foreach (QPointF p, poly)
        if (rect.contains(p))
            return true;

    if (poly.containsPoint(rect.center(), Qt::OddEvenFill))
        return true;

    QLineF top   (rect.topLeft()    , rect.topRight());
    QLineF right (rect.topRight()   , rect.bottomRight());
    QLineF bottom(rect.bottomRight(), rect.bottomLeft());
    QLineF left  (rect.bottomLeft() , rect.topLeft());

    // т.к. экран задан пятью точками, то проверять концевой отрезок не обязательно
    //int N = poly.size() + 1;
    int N = poly.size();
    for (int i = 1; i < N; ++i) {
        //QLineF tmp(poly.at(i-1), poly.at(i % poly.size()));
        QLineF tmp(poly.at(i-1), poly.at(i));
        QPointF a;
        if (top.intersect(tmp   , &a) == QLineF::BoundedIntersection)
            return true;
        if (bottom.intersect(tmp, &a) == QLineF::BoundedIntersection)
            return true;
        if (right.intersect(tmp , &a) == QLineF::BoundedIntersection)
            return true;
        if (left.intersect(tmp  , &a) == QLineF::BoundedIntersection)
            return true;
    }
    return false;
}

bool minigis::rectIntersectLine(const QRectF &rect, const QLineF &line)
{
    if (rect.contains(line.p1()) || rect.contains(line.p2()))
        return true;
    QPointF a;
    if (QLineF(rect.topLeft()    , rect.topRight()   ).intersect(line, &a) == QLineF::BoundedIntersection)
        return true;
    if (QLineF(rect.topRight()   , rect.bottomRight()).intersect(line, &a) == QLineF::BoundedIntersection)
        return true;
    if (QLineF(rect.bottomRight(), rect.bottomLeft() ).intersect(line, &a) == QLineF::BoundedIntersection)
        return true;
    if (QLineF(rect.bottomLeft() , rect.topLeft()    ).intersect(line, &a) == QLineF::BoundedIntersection)
        return true;
    return false;
}

bool minigis::rectIntersectEllipse(const QRectF &rect, const QPointF &mid, const qreal &rx, const qreal &ry, const qreal &rotate, bool isHollow)
{
    QPolygonF rectPoly;
    rectPoly.append(rect.topLeft());
    rectPoly.append(rect.topRight());
    rectPoly.append(rect.bottomRight());
    rectPoly.append(rect.bottomLeft());
    int k = 0, l = 0;

    qreal a, b, c, d;

    ellipseCoeff(rx, ry, rotate, a, b, c, d); // внешняя окружность

    foreach (QPointF point, rectPoly) {
        QPointF dist = point - mid;
        double r = a * dist.x() * dist.x() + b * dist.y() * dist.y() + c * dist.x() * dist.y();
        if (r < d) { // количество точек внутри окружности
            if (!isHollow)
                return true;
            ++k;
        }
        else  // количество точек снаружи окружности
            ++l;
    }

    if (k == 0 && rect.contains(mid))
        return true;
    else if (k == 4 && isHollow)
        return false;

    // проверка пересечения прямоугольника с эллипсом
    if (ellipseHaveCoord(mid, a, b, c, d, rect.left()  , rect.top() , rect.bottom(), true))
        return true;
    if (ellipseHaveCoord(mid, a, b, c, d, rect.top()   , rect.left(), rect.right() , false))
        return true;
    if (ellipseHaveCoord(mid, a, b, c, d, rect.right() , rect.top() , rect.bottom(), true))
        return true;
    if (ellipseHaveCoord(mid, a, b, c, d, rect.bottom(), rect.left(), rect.right() , false))
        return true;
    return false;
}

bool minigis::rectIntersectCircle(const QRectF &rect, const QPointF &mid, const qreal &rx, bool isHollow)
{
    QRectF obj(QRectF(QPointF(mid.x() - rx, mid.y() - rx), QSizeF(2 * rx, 2 * rx)).normalized());
    if (rect.intersects(obj)) {
        qreal rx2 = rx * rx;
        bool inside = false;
        if (rect.contains(mid))
            inside = true;

        int k = 0;
        if (lengthR2Squared(rect.topLeft() - mid) < rx2)
            ++k;
        if (lengthR2Squared(rect.topRight() - mid) < rx2)
            ++k;
        if (lengthR2Squared(rect.bottomRight() - mid) < rx2)
            ++k;
        if (lengthR2Squared(rect.bottomLeft() - mid) < rx2)
            ++k;

        if (k == 0 && !inside) {
            if (rect.contains(QPointF(obj.center().x(), obj.top())))
                return true;
            if (rect.contains(QPointF(obj.center().x(), obj.bottom())))
                return true;
            if (rect.contains(QPointF(obj.left(), obj.center().y())))
                return true;
            if (rect.contains(QPointF(obj.right(), obj.center().y())))
                return true;
            return false;
        }
        if (k == 4 && isHollow)
            return false;
        return true;
    }
    return false;
}

bool minigis::pixAnalyse(const QImage &img, const QRectF &rect)
{
    int left   = qMax(0           , int(qMin(rect.left(), rect.right())));
    int right  = qMin(img.width() , int(qMax(rect.left(), rect.right())));
    int top    = qMax(0           , int(qMin(rect.top(), rect.bottom())));
    int bottom = qMin(img.height(), int(qMax(rect.top(), rect.bottom())));

    for (int i = top; i < bottom; ++i) {
        const QRgb *line = reinterpret_cast<const QRgb*>(img.scanLine(i));
        for (int j = left; j < right; ++j)
            if (qAlpha(line[j]) != 0)
                return true;
    }
    return false;
}

bool minigis::rectIntersectCubic(const QRectF &rect, QPointF P0, QPointF P1, QPointF P2, QPointF P3)
{
    QLineF top   (rect.topLeft()    , rect.topRight());
    QLineF right (rect.topRight()   , rect.bottomRight());
    QLineF bottom(rect.bottomRight(), rect.bottomLeft());
    QLineF left  (rect.bottomLeft() , rect.topLeft());

    typedef QPair<double, double> DoublePair;
    QVector<DoublePair> vect;
    vect << intersectCubicWithLine(P0, P1, P2, P3, top.p1(), top.p2());
    vect << intersectCubicWithLine(P0, P1, P2, P3, right.p1(), right.p2());
    vect << intersectCubicWithLine(P0, P1, P2, P3, bottom.p1(), bottom.p2());
    vect << intersectCubicWithLine(P0, P1, P2, P3, left.p1(), left.p2());
    foreach (DoublePair uv, vect) {
        double u = uv.first;
        double v = uv.second;
        if ((qFuzzyIsNull(u) || qFuzzyIsNull(u - 1.) || (0. < u && u < 1.))
                && (qFuzzyIsNull(v) || qFuzzyIsNull(v - 1.) || (0. < v && v < 1.))) {
            return true;
        }
    }
    return false;
}

bool minigis::rectIntersectCubic(const QRectF &rect, const QPolygonF &path)
{
    QLineF top   (rect.topLeft()    , rect.topRight());
    QLineF right (rect.topRight()   , rect.bottomRight());
    QLineF bottom(rect.bottomRight(), rect.bottomLeft());
    QLineF left  (rect.bottomLeft() , rect.topLeft());

    typedef QPair<double, double> DoublePair;
    for (int i = 3; i < path.size(); i += 3) {
        QPointF P0 = path.at(i - 3);
        QPointF P1 = path.at(i - 2);
        QPointF P2 = path.at(i - 1);
        QPointF P3 = path.at(i);

        QVector<DoublePair> vect;
        vect << intersectCubicWithLine(P0, P1, P2, P3, top.p1(), top.p2());
        vect << intersectCubicWithLine(P0, P1, P2, P3, right.p1(), right.p2());
        vect << intersectCubicWithLine(P0, P1, P2, P3, bottom.p1(), bottom.p2());
        vect << intersectCubicWithLine(P0, P1, P2, P3, left.p1(), left.p2());
        foreach (DoublePair uv, vect) {
            double u = uv.first;
            double v = uv.second;
            if ((qFuzzyIsNull(u) || qFuzzyIsNull(u - 1.) || (0. < u && u < 1.))
                    && (qFuzzyIsNull(v) || qFuzzyIsNull(v - 1.) || (0. < v && v < 1.))) {
                return true;
            }
        }
    }
    return false;
}

QList<QPolygonF> minigis::cutPolygon(const QPolygonF &poly, const QRectF &rect, bool closed, QList<qreal> *params)
{
    QList<QPolygonF> lines;

    if (poly.isEmpty())
        return lines;

    bool exist = true;
    if (!params) {
        params = new QList<qreal>();
        exist = false;
    }

    qreal len;
    QVector<qreal> paramList = closed ?
            lineParamsClosed(poly, len) : lineParams(poly, len);

    QLineF left  (rect.bottomLeft() , rect.topLeft()    );
    QLineF right (rect.topRight()   , rect.bottomRight());
    QLineF top   (rect.topLeft()    , rect.topRight()   );
    QLineF bottom(rect.bottomRight(), rect.bottomLeft() );

    QPolygonF current;
    bool inside = false;
    if (rect.contains(poly.first())) {
        inside = true;
        current.append(poly.first());
        params->append(0);
    }

    int N = closed ? poly.size() + 1 : poly.size();
    for (int i = 1; i < N; ++i) {
        int k(i);

        if (closed)
            if (i == poly.size())
                k = 0;

        bool contains = rect.contains(poly.at(k));
        if (contains) {
            if (!inside) {
                QPointF tmp;
                QLineF l(poly.at(k), poly.at(i-1));

                bool newPoint = false;
                if      (l.intersect(left  , &tmp) == QLineF::BoundedIntersection)
                    newPoint = true;
                else if (l.intersect(right , &tmp) == QLineF::BoundedIntersection)
                    newPoint = true;
                else if (l.intersect(top   , &tmp) == QLineF::BoundedIntersection)
                    newPoint = true;
                else if (l.intersect(bottom, &tmp) == QLineF::BoundedIntersection)
                    newPoint = true;

                if (newPoint) {
                    qreal t;
                    pointOnLine(tmp, l, t);
                    params->append((1 - t) * (k == 0 ? paramList.last() : paramList.at(k - 1)) + t * (i == 1 ? 0 : paramList.at(i - 2)));
                    current.append(tmp);
                }

                inside = true;
            }
            current.append(poly.at(k));

            if (current.isEmpty())
                params->append(k == 0 ? paramList.last() : paramList.at(k - 1));
        }
        else {
            if (inside) {
                QPointF tmp;
                QLineF l(poly.at(k), poly.at(i-1));
                if      (l.intersect(left  , &tmp) == QLineF::BoundedIntersection)
                    current.append(tmp);
                else if (l.intersect(right , &tmp) == QLineF::BoundedIntersection)
                    current.append(tmp);
                else if (l.intersect(top   , &tmp) == QLineF::BoundedIntersection)
                    current.append(tmp);
                else if (l.intersect(bottom, &tmp) == QLineF::BoundedIntersection)
                    current.append(tmp);

                lines.append(current);
                current.clear();
                inside = false;
            }
            else {
                current.clear();
                QPointF tmp;
                QLineF l(poly.at(k), poly.at(i-1));
                int n = 0;
                if (l.intersect(left  , &tmp) == QLineF::BoundedIntersection) {
                    current.append(tmp);
                    ++n;
                }
                if (l.intersect(right , &tmp) == QLineF::BoundedIntersection) {
                    current.append(tmp);
                    ++n;
                }
                if (l.intersect(top   , &tmp) == QLineF::BoundedIntersection) {
                    current.append(tmp);
                    ++n;
                }
                if (l.intersect(bottom, &tmp) == QLineF::BoundedIntersection) {
                    current.append(tmp);
                    ++n;
                }
                if (n == 2) {
                    qreal tOne;
                    qreal tTwo;
                    pointOnLine(current.first(), l, tOne);
                    pointOnLine(current.at(1)  , l, tTwo);

                    if (tOne > tTwo) {
                        params->append((1 - tOne) * (k == 0 ? paramList.last() : paramList.at(k - 1)) + tOne * (i == 1 ? 0 : paramList.at(i - 2)));
                        lines.append(current);
                    }
                    else {
                        params->append((1 - tTwo) * (k == 0 ? paramList.last() : paramList.at(k - 1)) + tTwo * (i == 1 ? 0 : paramList.at(i - 2)));
                        lines.append(QPolygonF() << current.at(1) << current.first());
                    }
                    current.clear();
                }
            }
        }
    }
    if (!current.isEmpty())
        lines.append(current);

    if (!exist) {
        delete params;
        params = NULL;
    }
    return lines;
}

void minigis::ellipseCoeff(qreal rx, qreal ry, qreal phi, qreal &a, qreal &b, qreal &c, qreal &d)
{
    qreal rx2 = rx * rx;
    qreal ry2 = ry * ry;
    qreal sin  = qSin(phi);
    qreal sin2 = sin * sin;
    qreal cos  = qCos(phi);
    qreal cos2 = cos * cos;

    a = rx2 * sin2 + ry2 * cos2;
    b = rx2 * cos2 + ry2 * sin2;
    c = 2 * cos * sin * (ry2 - rx2);
    d = ry2 * rx2;
}

bool minigis::ellipseHaveCoord(const QPointF &mid, const qreal &a, const qreal &b, const qreal &c, const qreal &d, const qreal &coord, const qreal &min, const qreal &max, bool xory)
{
    if (xory) { // ищем y
        qreal x = (coord - mid.x());
        qreal cc = c * x;
        qreal cc2 = cc * cc;
        qreal x2 = x * x;

        qreal disc = cc2 - 4 * b * (a * x2 - d);
        if (qFuzzyIsNull(disc)) {
            qreal y = mid.y() - cc / (2 * b);
            if (min < y && y < max)
                return true;
        }
        else if (disc > 0) {
            disc = sqrt(disc);
            qreal y = mid.y() + (-cc - disc) / (2 * b);
            if (min < y && y < max)
                return true;
            y = mid.y() + (-cc + disc) / (2 * b);
            if (min < y && y < max)
                return true;
        }
    }
    else { // ищем x
        qreal y = (coord - mid.y());
        qreal cc = c * y;
        qreal cc2 = cc * cc;
        qreal y2 = y * y;

        qreal disc = cc2 - 4 * a * (b * y2 - d);
        if (qFuzzyIsNull(disc)) {
            qreal x = mid.x() - cc / (2 * a);
            if (min < x && x < max)
                return true;
        }
        else if (disc > 0) {
            disc = sqrt(disc);
            qreal x = mid.x() + (-cc - disc) / (2 * a);
            if (min < x && x < max)
                return true;
            x = mid.x() + (-cc + disc) / (2 * a);
            if (min < x && x < max)
                return true;
        }
    }
    return false;
}

// -------------------------------------------------------

class minigis::ArrowTypePrivate
{
public:
    QByteArray arrowTemplate;
    QHash<uint, QPair<QByteArray, QRectF> > arrowMetric;
};

inline QRectF boundFromByteArray(QByteArray const &d)
{
    QRectF bound;
    QList<QByteArray> params = d.split(' ');
    if (params.size() == 4)
        bound = QRectF(QPointF(params.at(0).toDouble(), params.at(1).toDouble()), QSizeF(params.at(2).toDouble(), params.at(3).toDouble()));
    return bound;
}

minigis::ArrowType::ArrowType()
    :d_ptr(new ArrowTypePrivate)
{
    QFile f(":/map3/arrows_template.svg");
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Not found arrows_template.svg";
        return;
    }
    d_ptr->arrowTemplate = f.readAll();
    f.close();

    f.setFileName(":/map3/arrows_metric.soe");
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Not found arrows_metric.soe";
        return;
    }
    QXmlStreamReader xml(&f);
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartDocument)
            continue;
        if (token == QXmlStreamReader::StartElement)
            if (xml.name() == "marker") {
                QXmlStreamAttributes attributes = xml.attributes();
                QByteArray name;
                QByteArray metric;
                QRectF bound;
                name = attributes.value("draw:display-name").toUtf8();
                if (name.isEmpty())
                    name = attributes.value("draw:name").toUtf8();
                bound = boundFromByteArray(attributes.value("svg:viewBox").toUtf8());
                metric = attributes.value("svg:d").toUtf8();
                d_ptr->arrowMetric[qHash(name)] = qMakePair<QByteArray, QRectF>(metric, bound);
//                qDebug() << __FUNCTION__ << name << qHash(name);
            }
    }
    f.close();
}

minigis::ArrowType::~ArrowType()
{
    if (!d_ptr)
        return;
    delete d_ptr;
    d_ptr = NULL;
}

minigis::ArrowType &minigis::ArrowType::inst()
{
    static minigis::ArrowType arr;
    return arr;
}

QByteArray minigis::ArrowType::templateSVG()
{
    return inst().d_ptr->arrowTemplate;
}

QHash<uint, QPair<QByteArray, QRectF> > minigis::ArrowType::metric()
{
    return inst().d_ptr->arrowMetric;
}

QByteArray minigis::ArrowType::metric(uint key)
{
    return inst().d_ptr->arrowMetric.value(key).first;
}

QRectF minigis::ArrowType::bound(uint key)
{
    return inst().d_ptr->arrowMetric.value(key).second;
}

QList<uint> minigis::ArrowType::keys()
{
    return inst().d_ptr->arrowMetric.keys();
}
