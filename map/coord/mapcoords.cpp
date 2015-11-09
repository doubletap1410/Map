
#include <qnumeric.h>

#include "coord/mapcoords.h"

#define APZ 6378136.3               // большая полуось
#define AlphaPZ 1 / 298.25784       // сжатие

#define AWGS 6378137                // большая полуось
#define AlphaWGS 1 / 298.257223563  // сжатие

#define AKr 6378245                 // большая полуось
#define AlphaKr 1 / 298.3           // сжатие

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

#if 1
static const double EarthRadius = 6378137.;
//Используется проекция Меркатора, EPSG:3395.
#else
static const double EarthRadius = 63710072.;
// in QT
#endif

static const double OriginShift = M_PI * EarthRadius;
static const double OriginDeg = OriginShift / 180.;
const double MaxLongitude = 180;

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

double TileSystem::MaxX = latLonToMeters(QPointF(LatMax, 0)).x();
double TileSystem::MaxY = latLonToMeters(QPointF(0, LonMax)).y();

double TileSystem::groundResolutionDataLayer[32] = {
    2 * OriginShift / (quint64(256) <<  0),
    2 * OriginShift / (quint64(256) <<  1),
    2 * OriginShift / (quint64(256) <<  2),
    2 * OriginShift / (quint64(256) <<  3),
    2 * OriginShift / (quint64(256) <<  4),
    2 * OriginShift / (quint64(256) <<  5),
    2 * OriginShift / (quint64(256) <<  6),
    2 * OriginShift / (quint64(256) <<  7),
    2 * OriginShift / (quint64(256) <<  8),
    2 * OriginShift / (quint64(256) <<  9),
    2 * OriginShift / (quint64(256) << 10),
    2 * OriginShift / (quint64(256) << 11),
    2 * OriginShift / (quint64(256) << 12),
    2 * OriginShift / (quint64(256) << 13),
    2 * OriginShift / (quint64(256) << 14),
    2 * OriginShift / (quint64(256) << 15),
    2 * OriginShift / (quint64(256) << 16),
    2 * OriginShift / (quint64(256) << 17),
    2 * OriginShift / (quint64(256) << 18),
    2 * OriginShift / (quint64(256) << 19),
    2 * OriginShift / (quint64(256) << 20),
    2 * OriginShift / (quint64(256) << 21),
    2 * OriginShift / (quint64(256) << 22),
    2 * OriginShift / (quint64(256) << 23),
    2 * OriginShift / (quint64(256) << 24),
    2 * OriginShift / (quint64(256) << 25),
    2 * OriginShift / (quint64(256) << 26),
    2 * OriginShift / (quint64(256) << 27),
    2 * OriginShift / (quint64(256) << 28),
    2 * OriginShift / (quint64(256) << 29),
    2 * OriginShift / (quint64(256) << 30),
    2 * OriginShift / (quint64(256) << 31)
};

// -------------------------------------------------------

QPointF TileSystem::latLonToMeters(const QPointF &latLon)
{
    qreal my = qLn(qTan(degToRad(90 + latLon.x()) * 0.5));
    return QPointF(my * EarthRadius, latLon.y() * OriginDeg);
}

QPointF TileSystem::metersToLatLon(const QPointF &xy)
{
    qreal lon = xy.y() / OriginDeg;
    qreal lat = xy.x() / OriginDeg;

    lat = radToDeg(2 * qAtan(qExp(degToRad(lat))) - M_PI_2);
    return QPointF(lat, lon);
}

QPointF TileSystem::metersToLatLon2(const QPointF &xy)
{
    double d = 180. / M_PI;
    return QPointF(
        (2 * qAtan(qExp(xy.y() / EarthRadius)) - (M_PI / 2)) * d,
        xy.x() * d / EarthRadius);
}

QPointF TileSystem::pixToMeters(const QPointF &pixXY, const int &zoom)
{
    qreal res = groundResolution(zoom);
    return QPointF(double(pixXY.x()) * res - OriginShift, double(pixXY.y()) * res - OriginShift);
}

QPointF TileSystem::metersToPix(const QPointF &xy, const int &zoom)
{
    qreal res = groundResolution(zoom);
    return QPointF((xy.x() + OriginShift) / res, (xy.y() + OriginShift) / res);
}

QPoint TileSystem::pixelToTile(const QPointF &pixXY)
{
    int tx = qFloor(pixXY.x() / qreal(TileSize));
    int ty = qFloor(pixXY.y() / qreal(TileSize));
    return QPoint(tx, ty);
}

QPoint TileSystem::tileToPixel(const QPoint &tileXY)
{
    return tileXY * TileSize;
}

QPoint TileSystem::metersToTile(const QPointF &xy, const int &zoom)
{
    return pixelToTile(metersToPix(xy, zoom));
}

QPointF TileSystem::metersToTileReal(const QPointF &xy, const int &zoom)
{
    return metersToPix(xy, zoom) / qreal(TileSize);
}

QRectF TileSystem::tileBounds(const QPoint &tile, const int &zoom)
{
    QPointF topLeft     = pixToMeters(QPointF( tile.x()      * TileSize,  tile.y()      * TileSize), zoom);
    QPointF bottomRight = pixToMeters(QPointF((tile.x() + 1) * TileSize, (tile.y() + 1) * TileSize), zoom);
    return QRectF(topLeft, bottomRight);
}

qreal TileSystem::groundResolution(qreal latitude, int levelOfDetail)
{
    latitude = qBound<qreal>(-LatMax, latitude, LatMax);
    double k =
            (levelOfDetail >= 0 && levelOfDetail < 32) ?
                groundResolutionDataLayer[levelOfDetail] :
                2 * OriginShift / double(quint64(256) << levelOfDetail);

    return qCos(degToRad(latitude)) * k;
}

qreal TileSystem::groundResolution(int levelOfDetail)
{
    return (levelOfDetail >= 0 && levelOfDetail < 32) ?
                groundResolutionDataLayer[levelOfDetail] :
                2 * OriginShift / double(quint64(256) << levelOfDetail);
}

int TileSystem::zoomForPixelSize(qreal pixSize)
{
    for (int i = 0; i < 32; ++i)
        if (pixSize > groundResolutionDataLayer[i])
            return i != 0 ? i - 1 : 0;
    return 0;
}

QString TileSystem::tileToQuadKey(const QPoint &tileXY, int levelOfDetail)
{
    return tileToQuadKey(tileXY.x(), tileXY.y(), levelOfDetail);
}

QString TileSystem::tileToQuadKey(int tileX, int tileY, int levelOfDetail)
{
    QString quadKey = "";
    for (int i = levelOfDetail; i > 0; --i) {
        char digit = '0';
        int mask = 1 << (i - 1);
        if ((tileX & mask) != 0)
            ++digit;
        if ((tileY & mask) != 0) {
            ++digit;
            ++digit;
        }
        quadKey.append(digit);
    }
    return quadKey;
}

bool TileSystem::quadKeyToTile(const QString &quadKey, QPoint &tileXY, int &levelOfDetail)
{
    int tileX = 0;
    int tileY = 0;
    levelOfDetail = quadKey.length();
    for (int i = levelOfDetail; i > 0; --i) {
        quint64 mask = quint64(1) << (i - 1);
        switch (quadKey[levelOfDetail - i].toLatin1()) {
        case '0':
            break;
        case '1':
            tileX |= mask;
            break;
        case '2':
            tileY |= mask;
            break;
        case '3':
            tileX |= mask;
            tileY |= mask;
            break;
        default:
            return false; // Invalid QuadKey digit sequence
        }
    }
    tileXY = QPoint(tileX, tileY);
    return true;
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

const qreal ParamsPz::a = 6378136.3;                   // большая полуось
const qreal ParamsPz::alpha = 1 / 298.25784;           // сжатие
const qreal ParamsPz::e2 = 2 * ParamsPz::alpha - ParamsPz::alpha * ParamsPz::alpha;
const qreal ParamsPz::e = qSqrt(ParamsPz::e2);
const qreal ParamsPz::OriginShift = M_PI * ParamsPz::a;
const qreal ParamsPz::OriginDeg = ParamsPz::OriginShift / 180.;

const qreal ParamsWGS::a = 6378137;                     // большая полуось
const qreal ParamsWGS::alpha = 1 / 298.257223563;       // сжатие
const qreal ParamsWGS::e2 = 2 * ParamsWGS::alpha - ParamsWGS::alpha * ParamsWGS::alpha;
const qreal ParamsWGS::e = qSqrt(ParamsWGS::e2);
const qreal ParamsWGS::OriginShift = M_PI * ParamsWGS::a;
const qreal ParamsWGS::OriginDeg = ParamsWGS::OriginShift / 180.;

const qreal ParamsKrass::a = 6378245;                     // большая полуось
const qreal ParamsKrass::alpha = 1 / 298.3;               // сжатие
const qreal ParamsKrass::e2 = 2 * ParamsKrass::alpha - ParamsKrass::alpha * ParamsKrass::alpha;
const qreal ParamsKrass::e = qSqrt(ParamsKrass::e2);
const qreal ParamsKrass::OriginShift = M_PI * ParamsKrass::a;
const qreal ParamsKrass::OriginDeg = ParamsKrass::OriginShift / 180.;

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

Coord TeSK42toPZ90::dr = Coord(25.0, -141.0, -80.0);
Coord TeSK42toPZ90::dAngRad = Coord(0.0, secToRad(-0.35), secToRad(-0.66));
Coord TeSK42toPZ90::dAngSec = Coord(0.0, -0.35, -0.66);
double TeSK42toPZ90::m = 0.0;

Coord TeSK95toPZ90::dr = Coord(25.9, -130.94, -81.76);
Coord TeSK95toPZ90::dAngRad = Coord(0.0, 0.0, 0.0);
Coord TeSK95toPZ90::dAngSec = Coord(0.0, 0.0, 0.0);
double TeSK95toPZ90::m = 0.0;

Coord TePZ90toWGS84::dr = Coord(-1.1, -0.3, -0.9);
Coord TePZ90toWGS84::dAngRad = Coord(0.0, 0.0, secToRad(-0.2));
Coord TePZ90toWGS84::dAngSec = Coord(0.0, 0.0, -0.2);
double TePZ90toWGS84::m = -0.12 * 1e-6;

Coord TeSK42toPZ9002::dr = Coord(23.93, -141.03, -79.98);
Coord TeSK42toPZ9002::dAngRad = Coord(0.0, secToRad(-0.35), secToRad(-0.79));
Coord TeSK42toPZ9002::dAngSec = Coord(0.0, -0.35, -0.79);
double TeSK42toPZ9002::m = -0.22 * 1e-6;

Coord TeSK95toPZ9002::dr = Coord(24.83, -130.97, -81.74);
Coord TeSK95toPZ9002::dAngRad = Coord(0.0, 0.0, secToRad(-0.13));
Coord TeSK95toPZ9002::dAngSec = Coord(0.0, 0.0, -0.13);
double TeSK95toPZ9002::m = -0.12 * 1e-6;

Coord TePZ9002toWGS84::dr = Coord(-0.36, 0.08, 0.18);
Coord TePZ9002toWGS84::dAngRad = Coord(0.0, 0.0, 0.0);
Coord TePZ9002toWGS84::dAngSec = Coord(0.0, 0.0, 0.0);
double TePZ9002toWGS84::m = 0.0;

Coord TePZ9002toPZ90::dr = Coord(1.07, 0.03, -0.02);
Coord TePZ9002toPZ90::dAngRad = Coord(0.0, 0.0, secToRad(0.13));
Coord TePZ9002toPZ90::dAngSec = Coord(0.0, 0.0, 0.13);
double TePZ9002toPZ90::m = 0.22 * 1e-6;

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

Coord CoordinateSystem::geoToRect(Coord geo, double alpha, double a)
{
    double x, y, z;
    double e, n;

    e = 2 * alpha - alpha * alpha;

    double sinx = qSin(geo.x);
    double cosx = qCos(geo.x);

    n = a / sqrt(1 - e * sinx * sinx);

    x = (n + geo.z) * cosx * qCos(geo.y);
    y = (n + geo.z) * cosx * qSin(geo.y);
    z = ((1 - e) * n + geo.z) * sinx;

    return Coord(x, y, z);
}

Coord CoordinateSystem::rectToGeo(Coord rect, double alpha, double a, double end)
{
    double b, l, h;
    double d = sqrt(rect.x * rect.x + rect.y * rect.y);

    double e = 2 * alpha - alpha * alpha;


    if (qFuzzyIsNull(d)) {
        if (qFuzzyIsNull(rect.z))
            b = 0;
        else
            b = rect.z > 0 ? M_PI_2 : -M_PI_2;

        l = 0;
        double sinx = qSin(b);
        h = rect.z * sinx - a * sqrt(1 - e * sinx * sinx);
    }
    else {
        double la = asin(rect.y / d);
        if (qFuzzyIsNull(rect.y)) {
            if (rect.x > 0)
                l = 0;
            else
                l = M_PI;
        }
        else if (rect.y > 0) {
            if (rect.x > 0)
                l = la;
            else
                l = M_PI - la;
        }
        else {
            if (rect.x > 0)
                l = 2 * M_PI - la;
            else
                l = M_PI + la;
        }

        if (qFuzzyIsNull(rect.z)) {
            b = 0;
            h = d - a;
        }
        else {
            double r, c, p;
            r = sqrt(rect.x * rect.x + rect.y * rect.y + rect.z * rect.z);
            c = asin(rect.z / r);
            p = e * a / (2 * r);
            double s1 = 0, s2 = 0, i_b = 0, i_d = 0;
            end = secToRad(end);
            do {
                s1 = s2;
                i_b = c + s1;
                double sin_i = qSin(i_b);
                s2 = asin(p * qSin(2 * i_b) / sqrt(1 - e * sin_i * sin_i));
                i_d = qAbs(s2 - s1);
            } while (i_d > end || qFuzzyCompare(i_d, end));
            b = i_b;

            double sinx = qSin(b);
            h = d * qCos(b) + rect.z * sinx - a * sqrt(1 - e * sinx * sinx);
        }
    }
    return Coord(b, l, h);
}

Coord CoordinateSystem::transformRect(Coord rect, Coord dr, Coord ang, double m)
{
    // ang в рад
    double x, y, z;

//    x = (1 + m) * (         rect.x + ang.z * rect.y - ang.y * rect.z) + dr.x;
//    y = (1 + m) * (-ang.z * rect.x +         rect.y + ang.x * rect.z) + dr.y;
//    z = (1 + m) * ( ang.y * rect.x - ang.x * rect.y +         rect.z) + dr.z;

    x = (1 + m) * rect.x +   ang.z * rect.y -   ang.y * rect.z + dr.x;
    y =  -ang.z * rect.x + (1 + m) * rect.y +   ang.x * rect.z + dr.y;
    z =   ang.y * rect.x -   ang.x * rect.y + (1 + m) * rect.z + dr.z;
    return Coord(x, y, z);
}

Coord CoordinateSystem::backTransformRect(Coord rect, Coord dr, Coord ang, double m)
{
    // ang в рад
    double x, y, z;

//    x = (1 - m) * (         rect.x - ang.z * rect.y + ang.y * rect.z) - dr.x;
//    y = (1 - m) * ( ang.z * rect.x +         rect.y - ang.x * rect.z) - dr.y;
//    z = (1 - m) * (-ang.y * rect.x + ang.x * rect.y +         rect.z) - dr.z;

    x = (1 - m) * rect.x -   ang.z * rect.y +   ang.y * rect.z - dr.x;
    y =   ang.z * rect.x + (1 - m) * rect.y -   ang.x * rect.z - dr.y;
    z =  -ang.y * rect.x +   ang.x * rect.y + (1 - m) * rect.z - dr.z;

    return Coord(x, y, z);
}

template <class T>
Coord CoordinateSystem::transformRect(Coord rect)
{
    return transformRect(rect, T::dr, T::dAngRad, T::m);
}

template <class T>
Coord CoordinateSystem::backTransformRect(Coord rect)
{
    return backTransformRect(rect, T::dr, T::dAngRad, T::m);
}

Coord CoordinateSystem::transformGeo(Coord geo, Coord dr, Coord ang, double m, double aa, double alpha_a, double ab, double alpha_b)
{
    // ang в секундах
    double B, L, H, dB, dL, dH;
    double p, da, de2, a, e2, N, M;

    double ea = 2 * alpha_a - alpha_a * alpha_a;
    double eb = 2 * alpha_b - alpha_b * alpha_b;

    da = ab - aa;
    de2 = eb - ea;
    a = (ab + aa) / 2;
    e2 = (eb + ea) / 2;

    double sinx = qSin(geo.x);
    double cosx = qCos(geo.x);
    double siny = qSin(geo.y);
    double cosy = qCos(geo.y);

    double W = sqrt(1 - e2 * sinx * sinx);
    M = a * (1 - e2) / (W * W * W);
    N = a / W;
    p = 206264.806;

    dB = p / (M + geo.z) * (N / a * e2 * sinx * cosx * da
                        + (N * N / (a * a) + 1) * N * sinx * cosx * de2 / 2
                        - (dr.x * cosy + dr.y * siny) * sinx + dr.z * cosx)
            - ang.x * siny * (1 + e2 * qCos(2 * geo.x)) + ang.y * cosy * (1 + e2 * qCos(2 * geo.x))
            - p * m * e2 * sinx * cosx;

    dL = p / ((N + geo.z) * cosx) * (-dr.x * siny + dr.y * cosy)
            + tan(geo.x) * (1 - e2) * (ang.x * cosy + ang.y * siny) - ang.z;

    dH = -a / N * da + N * sinx * sinx * de2 / 2 + (dr.x * cosy + dr.y * siny)
            * cosx + dr.z * sinx
            - N * e2 * sinx * cosx * (ang.x / p * siny - ang.y / p * cosy)
            + (a * a / N + geo.z) * m;

    B = geo.x + secToRad(dB);
    L = geo.y + secToRad(dL);
    H = geo.z + dH;
    return Coord(B, L, H);
}

Coord CoordinateSystem::backTransformGeo(Coord geo, Coord dr, Coord ang, double m, double aa, double alpha_a, double ab, double alpha_b)
{
    // ang в секундах
    double B, L, H, dB, dL, dH;
    double p, da, de2, a, e2, N, M;

    double ea = 2 * alpha_a - alpha_a * alpha_a;
    double eb = 2 * alpha_b - alpha_b * alpha_b;

    da = ab - aa;
    de2 = eb - ea;
    a = (ab + aa) / 2;
    e2 = (eb + ea) / 2;

    double sinx = qSin(geo.x);
    double cosx = qCos(geo.x);
    double siny = qSin(geo.y);
    double cosy = qCos(geo.y);

    double W = sqrt(1 - e2 * sinx * sinx);
    M = a * (1 - e2) / (W * W * W);
    N = a / W;
    p = 206264.806;

    dB = p / (M + geo.z) * (N / a * e2 * sinx * cosx * da
                        + (N * N / (a * a) + 1) * N * sinx * cosx * de2 / 2
                        - (dr.x * cosy + dr.y * siny) * sinx + dr.z * cosx)
            - ang.x * siny * (1 + e2 * qCos(2 * geo.x)) + ang.y * cosy * (1 + e2 * qCos(2 * geo.x))
            - p * m * e2 * sinx * cosx;

    dL = p / ((N + geo.z) * cosx) * (-dr.x * siny + dr.y * cosy)
            + tan(geo.x) * (1 - e2) * (ang.x * cosy + ang.y * siny) - ang.z;

    dH = -a / N * da + N * sinx * sinx * de2 / 2 + (dr.x * cosy + dr.y * siny)
            * cosx + dr.z * sinx
            - N * e2 * sinx * cosx * (ang.x / p * siny - ang.y / p * cosy)
            + (a * a / N + geo.z) * m;

    B = geo.x - secToRad(dB);
    L = geo.y - secToRad(dL);
    H = geo.z - dH;
    return Coord(B, L, H);
}

template <class T, class Pa, class Pb>
Coord CoordinateSystem::transformGeo(Coord geo)
{
    return transformGeo(geo, T::dr, T::dAngSec, T::m, Pa::a, Pa::alpha, Pb::a, Pb::alpha);
}

template <class T, class Pa, class Pb>
Coord CoordinateSystem::backTransformGeo(Coord geo)
{
    return backTransformGeo(geo, T::dr, T::dAngSec, T::m, Pa::a, Pa::alpha, Pb::a, Pb::alpha);
}

QPointF CoordinateSystem::SKtoFlat(Coord geo)
{
    return CoordinateSystem::SKtoFlat(geo, int((6 + radToDeg(geo.y)) / 6));
}

QPointF CoordinateSystem::SKfromFlat(QPointF flat)
{
    return CoordinateSystem::SKfromFlat(flat, int(flat.y() * 1e-6));
}

QPointF CoordinateSystem::SKtoFlat(Coord geo, int zone)
{
    double Y = radToDeg(geo.y);
    int n = zone;
    double l = (Y - (3 + 6 * (n - 1))) / 57.29577951;

    double sinx  = qSin(geo.x);
    double sinx2 = sinx  * sinx;
    double sinx4 = sinx2 * sinx2;
    double sinx6 = sinx2 * sinx4;

    double x = 6367558.4968 * geo.x - qSin(2 * geo.x) *
        (16002.89   + 66.9607    * sinx2 + 0.3515   * sinx4
         - l * l *
        (1594561.25 + 5336.535   * sinx2 + 26.790   * sinx4 + 0.149  * sinx6
         + l * l *
        (672483.4   - 811219.9   * sinx2 + 5420.0   * sinx4 - 10.6   * sinx6
         + l * l *
        (278194     - 830174     * sinx2 + 572434   * sinx4 - 16010  * sinx6
         + l * l *
        (109500     - 574700     * sinx2 + 863700   * sinx4 - 398600 * sinx6)))));

    double y = (5 + 10 * n) * 1e+5 + l * qCos(geo.x) *
        (6378245    + 21346.1415 * sinx2 + 107.1590 * sinx4 + 0.5977 * sinx6
         + l * l *
        (1070204.16 - 2136826.66 * sinx2 + 17.98    * sinx4 - 11.99  * sinx6
         + l * l *
        (270806     - 1523417    * sinx2 + 1327645  * sinx4 - 21701  * sinx6
         + l * l *
        (79690      - 866190     * sinx2 + 1730360  * sinx4 - 945460 * sinx6))));

    return QPointF(x, y);
}

QPointF CoordinateSystem::SKfromFlat(QPointF flat, int zone)
{
    double B, L;
    double b0, dB;
    double l, n, beta, z;
    n = zone;
    beta = double(flat.x()) / 6367558.4968;

    double sinBeta  = qSin(beta);
    double sinBeta2 = sinBeta * sinBeta;

    b0 = beta + qSin(2 * beta) * (0.00252588685 - 0.0000149186 * sinBeta2 + 0.00000011904 * sinBeta2 * sinBeta2);

    z = (double(flat.y()) - (10 * n + 5) * 1e+5) / (6378245 * qCos(b0));
    double z2 = z * z;

    double sinb  = qSin(b0);
    double sinb2 = sinb  * sinb;
    double sinb4 = sinb2 * sinb2;
    double sinb6 = sinb2 * sinb4;

    dB = - z2 * qSin(2 * b0) *
         (0.251684631 - 0.003369263 * sinb2 + 0.000011276  * sinb4
         - z2 *
         (0.10500614  - 0.04559916  * sinb2 + 0.00228901   * sinb4 - 0.00002987  * sinb6
         - z2 *
         (0.042858    - 0.025318    * sinb2 + 0.014346     * sinb4 - 0.001264    * sinb6
         - z2 *
         (0.01672     - 0.0063      * sinb2 * 0.01188      * sinb4 - 0.00328     * sinb6))));

    l = z *
         (1          - 0.0033467108 * sinb2 - 0.0000056002 * sinb4 - 0.0000000187 * sinb6
         - z2 *
         (0.16778975 + 0.16273586   * sinb2 - 0.0005349    * sinb4 - 0.00000846   * sinb6
         - z2 *
         (0.0420025  + 0.1487407    * sinb2 + 0.005942     * sinb4 - 0.000015     * sinb6
         - z2 *
         (0.01225    + 0.09477      * sinb2 + 0.03282      * sinb4 - 0.00034      * sinb6
         - z2 *
         (0.0038     + 0.0524       * sinb2 + 0.0482       * sinb4 + 0.0032       * sinb6)))));

    B = b0 + dB;
    L = 6 * (n - 0.5) / 57.29577951 + l;
    return QPointF(B, L);
}

QString CoordinateSystem::debugSec(double X)
{
    X = radToSec(degToRad(X));
    int degree = int(X / 3600);
    X -= degree * 3600;
    int min = int(X / 60);
    X -= min * 60;
    //return QString::number(degree) + QChar(176) + QString::number(min) + QChar(39) + QString("%1").arg(QString::number(qRound(X)), 2, '0') + QChar(34);
    return QString("%1%2%3%4%5%6")
            .arg(degree).arg(QChar(176))
            .arg(min, 2, 10, QChar('0')).arg(QChar(39))
            .arg(qRound(X), 2, 10, QChar('0')).arg(QChar(34));
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

using namespace CoordinateSystem;
Coord::Coord(qreal _x, qreal _y, qreal _z, Coord::System sys, Coord::CoordType type)
{
    x = _x;
    y = _y;
    z = _z;
    _type = type;
    _system = sys;
}

Coord::System Coord::coordSystem() const
{
    return _system;
}

Coord::CoordType Coord::type()
{
    return _type;
}

Coord &Coord::toWGS84()
{
    CoordType t = _type;

    switch (_system) {
    case WGS84:
        return *this;
    case PZ90: {
        *this = _type == Rectangular ? transformRect<TePZ90toWGS84>(*this) : transformGeo<TePZ90toWGS84, ParamsPz, ParamsWGS>(*this);
        break;
    }
    case PZ9002:
        *this = _type == Rectangular ? transformRect<TePZ9002toWGS84>(*this) : transformGeo<TePZ9002toWGS84, ParamsPz, ParamsWGS>(*this);
        break;
    case SK42:
        *this = _type == Rectangular ? transformRect<TePZ9002toWGS84>(transformRect<TeSK42toPZ9002>(*this)) :
                                       transformGeo<TePZ9002toWGS84, ParamsPz, ParamsWGS>(
                    transformGeo<TeSK42toPZ9002, ParamsKrass, ParamsPz>(*this));
        break;
    case SK95:
        *this = _type == Rectangular ? transformRect<TePZ9002toWGS84>(transformRect<TeSK95toPZ9002>(*this)) :
                                       transformGeo<TePZ9002toWGS84, ParamsPz, ParamsWGS>(
                    transformGeo<TeSK95toPZ9002, ParamsKrass, ParamsPz>(*this));
        break;
    default:
        return *this;
    }

    _type = t;
    _system = WGS84;

    return *this;
}

Coord &Coord::toPZ90()
{
    CoordType t = _type;

    switch (_system) {
    case WGS84:
        *this = _type == Rectangular ? backTransformRect<TePZ90toWGS84>(*this) : backTransformGeo<TePZ90toWGS84, ParamsPz, ParamsWGS>(*this);
        break;
    case PZ90:
        return *this;
    case PZ9002:
        *this = _type == Rectangular ? transformRect<TePZ9002toPZ90>(*this) : transformGeo<TePZ9002toPZ90, ParamsPz, ParamsPz>(*this);
        break;
    case SK42:
        *this = _type == Rectangular ? transformRect<TeSK42toPZ90>(*this) : transformGeo<TeSK42toPZ90, ParamsKrass, ParamsPz>(*this);
        break;
    case SK95:
        *this = _type == Rectangular ? transformRect<TeSK95toPZ90>(*this) : transformGeo<TeSK95toPZ90, ParamsKrass, ParamsPz>(*this);
        break;
    default:
        return *this;
    }

    _type = t;
    _system = PZ90;

    return *this;
}

Coord &Coord::toPZ9002()
{
    CoordType t = _type;

    switch (_system) {
    case WGS84:
        *this = _type == Rectangular ? backTransformRect<TePZ9002toWGS84>(*this) : backTransformGeo<TePZ9002toWGS84, ParamsPz, ParamsWGS>(*this);
        break;
    case PZ90:
        *this = _type == Rectangular ? backTransformRect<TePZ9002toPZ90>(*this) : backTransformGeo<TePZ9002toPZ90, ParamsPz, ParamsPz>(*this);
        break;
    case PZ9002:
        return *this;
    case SK42:
        *this = _type == Rectangular ? transformRect<TeSK42toPZ9002>(*this) : transformGeo<TeSK42toPZ9002, ParamsKrass, ParamsPz>(*this);
        break;
    case SK95:
        *this = _type == Rectangular ? transformRect<TeSK95toPZ9002>(*this) : transformGeo<TeSK95toPZ9002, ParamsKrass, ParamsPz>(*this);
        break;
    default:
        return *this;
    }

    _type = t;
    _system = PZ9002;

    return *this;
}

Coord &Coord::toSK42()
{
    CoordType t = _type;

    switch (_system) {
    case WGS84:
        *this = _type == Rectangular ?  backTransformRect<TeSK42toPZ9002>(backTransformRect<TePZ9002toWGS84>(*this)) :
                                        backTransformGeo<TeSK42toPZ9002, ParamsKrass, ParamsPz>(
                    backTransformGeo<TePZ9002toWGS84, ParamsPz, ParamsWGS>(*this));
        break;
    case PZ90:
        *this = _type == Rectangular ? backTransformRect<TeSK42toPZ90>(*this) : backTransformGeo<TeSK42toPZ90, ParamsKrass, ParamsPz>(*this);
        break;
    case PZ9002:
        *this = _type == Rectangular ? backTransformRect<TeSK42toPZ9002>(*this) : backTransformGeo<TeSK42toPZ9002, ParamsKrass, ParamsPz>(*this);
        break;
    case SK42:
        return *this;
    case SK95:
        *this = _type == Rectangular ? backTransformRect<TeSK42toPZ9002>(transformRect<TeSK95toPZ9002>(*this)) :
                                       backTransformGeo<TeSK42toPZ9002, ParamsKrass, ParamsPz>(
                    transformGeo<TeSK95toPZ9002, ParamsKrass, ParamsPz>(*this));
        break;
    default:
        return *this;
    }

    _type = t;
    _system = SK42;
    return *this;
}

Coord &Coord::toSK95()
{
    CoordType t = _type;

    switch (_system) {
    case WGS84:
        *this = _type == Rectangular ? backTransformRect<TeSK95toPZ9002>(backTransformRect<TePZ9002toWGS84>(*this)) :
                                       backTransformGeo<TeSK95toPZ9002, ParamsKrass, ParamsPz>(
                    backTransformGeo<TePZ9002toWGS84, ParamsPz, ParamsWGS>(*this));
        break;
    case PZ90:
        *this = _type == Rectangular ? backTransformRect<TeSK95toPZ90>(*this) : backTransformGeo<TeSK95toPZ90, ParamsKrass, ParamsPz>(*this);
        break;
    case PZ9002:
        *this = _type == Rectangular ? backTransformRect<TeSK95toPZ9002>(*this) : backTransformGeo<TeSK95toPZ9002, ParamsKrass, ParamsPz>(*this);
        break;
    case SK42:
        *this = _type == Rectangular ? backTransformRect<TeSK95toPZ9002>(transformRect<TeSK42toPZ9002>(*this)) :
                                      backTransformGeo<TeSK95toPZ9002, ParamsKrass, ParamsPz>(
                    transformGeo<TeSK42toPZ9002, ParamsKrass, ParamsPz>(*this));
        break;
    case SK95:
        return *this;
    default:
        return *this;
    }

    _type = t;
    _system = SK95;
    return *this;
}

Coord &Coord::toGeo()
{
    System s = _system;

    if (_type == Rectangular) {
        if (_system == PZ90 || _system == PZ9002)
            *this = rectToGeo(*this, ParamsPz::alpha, ParamsPz::a);
        else if (_system == WGS84)
            *this = rectToGeo(*this, ParamsWGS::alpha, ParamsWGS::a);
        else if (_system == SK42 || _system == SK95)
            *this = rectToGeo(*this, ParamsKrass::alpha, ParamsKrass::a);
    }
    else
        return *this;

    _type = Geodetic;
    _system = s;
    return *this;
}

Coord &Coord::toRect()
{
    System s = _system;

    if (_type == Geodetic) {
        if (_system == PZ90 || _system == PZ9002)
            *this = geoToRect(*this, ParamsPz::alpha, ParamsPz::a);
        else if (_system == WGS84)
            *this = geoToRect(*this, ParamsWGS::alpha, ParamsWGS::a);
        else if (_system == SK42 || _system == SK95)
            *this = geoToRect(*this, ParamsKrass::alpha, ParamsKrass::a);
    }
    else
        return *this;

    _type = Rectangular;
    _system = s;
    return *this;
}

QString Coord::dbg()
{
    // радианы
    if (_type == Rectangular)
        return "X: " + QString::number(x, 'f', 3) + " Y:" + QString::number(y, 'f', 3) + " Z:" + QString::number(z, 'f', 3);
    else
//        return "B: " + QString::number(x * RadToDeg, 'f', 8) + " L: " + QString::number(y * RadToDeg, 'f', 8) + " H: " + QString::number(z, 'f', 8);
        return "B: " + debugSec(x) + "L: " + debugSec(y) + "H: " + QString::number(z, 'f', 3);
}

QPointF Coord::toPoint()
{
    return QPointF(x, y);
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

QPointF CoordTranform::worldToGeo(const QPointF &xy)
{
    return TileSystem::metersToLatLon(xy);
}

QPointF CoordTranform::geoToWorld(const QPointF &latLon)
{
    return TileSystem::latLonToMeters(latLon);
}

QPointF CoordTranform::worldToSK42(const QPointF &xy)
{
    return geoToSK42(worldToGeo(xy));
}

QPointF CoordTranform::sk42ToWorld(const QPointF &sk42)
{
    return geoToWorld(sk42ToGeo(sk42));
}

QPointF CoordTranform::geoToSK42(const QPointF &latLon)
{
#warning неправильно считаем координаты
    return CoordinateSystem::SKtoFlat(Coord(
                                          degToRad(latLon.x()),
                                          degToRad(latLon.y()),
                                          0,
                                          Coord::WGS84,
                                          Coord::Geodetic)/*.toSK42()*/
                                      );
}

QPointF CoordTranform::sk42ToGeo(const QPointF &sk42)
{
    QPointF geoSK42 = CoordinateSystem::SKfromFlat(sk42);
    Coord wgs = Coord(geoSK42.x(), geoSK42.y(), 0, Coord::SK42, Coord::Geodetic)/*.toWGS84()*/;
    return radToDeg(wgs.toPoint());
}

int CoordTranform::zoneSK42FromSK42(const QPointF &sk42)
{
    return int(sk42.y() * 1e-6);
}

int CoordTranform::zoneSK42FromGeo(const QPointF &latLon)
{
    return int((6 + latLon.y()) / 6);
}

int CoordTranform::zoneSK42FromWorld(const QPointF &xy)
{
    return int((6 + xy.y() / OriginDeg) / 6);
}

QPointF CoordTranform::worldToSK42(const QPointF &xy, int zone)
{
    return geoToSK42(worldToGeo(xy), zone);
}

QPointF CoordTranform::sk42ToWorld(const QPointF &sk42, int zone)
{
    return geoToWorld(sk42ToGeo(sk42, zone));
}

QPointF CoordTranform::geoToSK42(const QPointF &latLon, int zone)
{
    return CoordinateSystem::SKtoFlat(Coord(
                                          degToRad(latLon.x()),
                                          degToRad(latLon.y()),
                                          0,
                                          Coord::WGS84,
                                          Coord::Geodetic)/*.toSK42()*/,
                                      zone
                                      );
}

QPointF CoordTranform::sk42ToGeo(const QPointF &sk42, int zone)
{
    QPointF geoSK42 = CoordinateSystem::SKfromFlat(sk42, zone);
    Coord wgs = Coord(geoSK42.x(), geoSK42.y(), 0, Coord::SK42, Coord::Geodetic)/*.toWGS84()*/;
    return radToDeg(wgs.toPoint());
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

template <typename T>
ProjEll<T>::ProjEll() {}

template <typename T>
QPointF ProjEll<T>::geoToWorld(const QPointF &latLon)
{
    qreal xTmp = degToRad(Params::a * latLon.y());
    qreal sin = qSin(degToRad(latLon.x()));
    qreal eSin = Params::e * sin;
    qreal yTmp = Params::a * 0.5 * (qLn((1 + sin) / (1 - sin)) - Params::e * qLn((1 + eSin) / (1 - eSin)));

    // Если будет слишком большая погрешность можно попробовать следующую формулу
//    qreal yTmp = a * log(qTan(M_PI_4 + y() * 0.5 * DegToRad) * pow((1 - eSinLat) / (1 + eSinLat), e * 0.5)) * RadToDeg;
    return QPointF(yTmp, xTmp);
}

template <typename T>
QPointF ProjEll<T>::worldToGeo(const QPointF &xy)
{
//    int n = 10;
//    qreal phi = 0;
//    for (int i = 0; i < n; ++i) {
//        qreal sin = qSin(phi);
//        qreal esin = Params::e * sin;
//        phi = qAsin(1 - (1 + sin) * pow((1 - esin), Params::e) / (qExp(2 * xy.x() / Params::a) * pow((1 + esin), Params::e)));
//    }

    // Если будет слишком большая погрешность можно попробовать следующую формулу
    static const qreal end = 0.00001;
    qreal t = qExp(-xy.x() / Params::a);
    qreal phi = M_PI_2 - 2 * qAtan(t);
    qreal dphi = 1.;
    int i = 0;
    while (qAbs(dphi) > end && i < 15) {
        qreal esin = Params::e * qSin(phi);
        dphi = M_PI_2 - 2 * qAtan(t * pow((1 - esin) / (1 + esin), Params::e * 0.5)) - phi;
        phi += dphi;
        ++i;
    }

    return QPointF(radToDeg(phi), radToDeg(xy.y() / Params::a));
}

// -------------------------------------------------------------------

template <typename T>
ProjSph<T>::ProjSph() {}

template <typename T>
QPointF ProjSph<T>::geoToWorld(const QPointF &latLon)
{
    qreal my = qLn(qTan(degToRad(90 + latLon.x()) * 0.5));
    return QPointF(my * Params::a, latLon.y() * Params::OriginDeg);
}

template <typename T>
QPointF ProjSph<T>::worldToGeo(const QPointF &xy)
{
    qreal lat = radToDeg(2 * qAtan(qExp(xy.x() / Params::a)) - M_PI_2);
    return QPointF(lat, xy.y() / Params::OriginDeg);
}

// -------------------------------------------------------------------


QPoint MyUtils::metersToEllipticTile(const QPointF &xy, int zoom)
{
    QPointF xyEll = ProjEll<ParamsWGS>::geoToWorld(ProjSph<ParamsWGS>::worldToGeo(xy));
    return TileSystem::metersToTile(xyEll, zoom);
}

QRectF MyUtils::tileBounds(const QPoint &tile, int zoom)
{
    QPointF topLeft     = tileToGeo(tile, zoom);
    QPointF bottomRight = tileToGeo(tile + QPoint(1, 1), zoom);

    topLeft     = ProjSph<ParamsWGS>::geoToWorld(topLeft);
    bottomRight = ProjSph<ParamsWGS>::geoToWorld(bottomRight);

    return QRectF(topLeft, bottomRight);
}

QRectF MyUtils::tileBoundsElliptic(const QPoint &tile, int zoom)
{
    QPointF topLeft     = tileToGeoSperical(tile, zoom);
    QPointF bottomRight = tileToGeoSperical(tile + QPoint(1, 1), zoom);

    topLeft     = ProjEll<ParamsWGS>::geoToWorld(topLeft);
    bottomRight = ProjEll<ParamsWGS>::geoToWorld(bottomRight);

    return QRectF(topLeft, bottomRight);
}

QPointF MyUtils::tileToGeo(const QPoint &tile, int zoom)
{
    QPointF point = TileSystem::pixToMeters(QPointF(tile.x() * TileSize, tile.y() * TileSize), zoom);
    return ProjEll<ParamsWGS>::worldToGeo(point);
}

QPointF MyUtils::tileToGeoSperical(const QPoint &tile, int zoom)
{
    QPointF point = TileSystem::pixToMeters(QPointF(tile.x() * TileSize, tile.y() * TileSize), zoom);
    return ProjSph<ParamsWGS>::worldToGeo(point);
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
