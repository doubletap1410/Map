#ifndef MAPCOORDS_H
#define MAPCOORDS_H

#include <QPointF>
#include <QRectF>

#include <qmath.h>

#include "core/mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

//! groundResolution on equator == 1 / camera->scale;

//! математическая обработка тайлов в сферической проекции меркатора (совмещенная Google и Microsoft);
struct TileSystem {
    //! из геодезический системы координат в сферическую проекцию меркатора
    static QPointF latLonToMeters(const QPointF &latLon);
    //! из сферической проекции меркатора в геодезическую систему координат
    static QPointF metersToLatLon(const QPointF &xy);
    static QPointF metersToLatLon2(const QPointF &xy);

    //! из локальных в мировые
    static QPointF pixToMeters(const QPointF &pixXY, const int &zoom);
    //! из мировых координтат в локальные
    static QPointF metersToPix(const QPointF &xy, const int &zoom);

    //! преобразует координаты пикселя XY координат в координаты XY плитки, содержащие указанный пиксел.
    static QPoint pixelToTile(const QPointF &pixXY);

    //! преобразует XY координаты плитки в пиксельные координатах XY верхнего левого пиксела, указанной плитки.
    static QPoint tileToPixel(const QPoint &tileXY);
    //! индексы подложки в которую попала точка в мировых координатах
    static QPoint metersToTile(const QPointF &xy, const int &zoom);

    static QPointF metersToTileReal(const QPointF &xy, const int &zoom);

    //! размер подложки в мировых координатах
    static QRectF tileBounds(const QPoint &tile, const int &zoom);

    //! разрешение карты (метров на пиксел) для данной широты и уровне детализации
    static qreal groundResolution(qreal latitude, int levelOfDetail);
    //! разрешение карты (метров на пиксел) на экваторе и данном уровне детализации
    static qreal groundResolution(int levelOfDetail);
    //! zoom
    static int zoomForPixelSize(qreal pixSize);

    //! преобразует XY координаты плитки в QuadKey на определенном уровне детализации
    static QString tileToQuadKey(const QPoint &tileXY, int levelOfDetail);
    static QString tileToQuadKey(int tileX, int tileY, int levelOfDetail);

    //! преобразует QuadKey в плитки XY координаты
    static bool quadKeyToTile(const QString &quadKey, QPoint &tileXY, int &levelOfDetail);

    static double MaxX;
    static double MaxY;
    static double groundResolutionDataLayer[32];
};

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

// параметры различных эллипсойдов Земли
struct ParamsPz
{
    static const qreal a;           // большая полуось
    static const qreal alpha;       // сжатие
    static const qreal e2;          // эксцентриситет в квадрате
    static const qreal e;           // эксцентриситет
    static const qreal OriginShift;
    static const qreal OriginDeg;
};

struct ParamsWGS
{
    static const qreal a;           // большая полуось
    static const qreal alpha;       // сжатие
    static const qreal e2;          // эксцентриситет в квадрате
    static const qreal e;           // эксцентриситет
    static const qreal OriginShift;
    static const qreal OriginDeg;
};

struct ParamsKrass
{
    static const qreal a;            // большая полуось
    static const qreal alpha;        // сжатие
    static const qreal e2;           // эксцентриситет в квадрате
    static const qreal e;            // эксцентриситет
    static const qreal OriginShift;
    static const qreal OriginDeg;
};

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

class Coord;
// Datum (стурктуры поправок для преобразований координат)
struct TeSK42toPZ90
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};
struct TeSK95toPZ90
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};
struct TePZ90toWGS84
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};
struct TeSK42toPZ9002
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};
struct TeSK95toPZ9002
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};
struct TePZ9002toWGS84
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};
struct TePZ9002toPZ90
{
    static Coord dr;
    static Coord dAngRad;
    static Coord dAngSec;
    static double m;
};

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

// класс преобразования координат между различными системами (WGS-84, SK-42, SK-95, PZ-90, PZ-90.02)
// класс создан на основе ГОСТа Р 51794-2008
namespace CoordinateSystem
{
// преобразование из прямогульных в геодезические координаты и обратно в одной системе координат
Coord geoToRect(Coord geo, double alpha, double a);
Coord rectToGeo(Coord rect, double alpha, double a, double end = 0.0001);

// преобразование прямоугольных координат между различными системами координат
Coord transformRect(Coord rect, Coord dr, Coord ang, double m);
Coord backTransformRect(Coord rect, Coord dr, Coord ang, double m);

template <class T>
Coord transformRect(Coord rect);
template <class T>
Coord backTransformRect(Coord rect);

// преобразование геодезических координат между различными системами координат
Coord transformGeo(Coord geo, Coord dr, Coord ang, double m, double aa, double alpha_a, double ab, double alpha_b);
Coord backTransformGeo(Coord geo, Coord dr, Coord ang, double m, double aa, double alpha_a, double ab, double alpha_b);

template <class T, class Pa, class Pb>
Coord transformGeo(Coord geo);
template <class T, class Pa, class Pb>
Coord backTransformGeo(Coord geo);

// преобразование геодезических координат в плоские прямоугольные координаты
// (только для референционной системы координат РФ)
QPointF SKtoFlat(Coord geo);
QPointF SKfromFlat(QPointF flat);
QPointF SKtoFlat(Coord geo, int zone);
QPointF SKfromFlat(QPointF flat, int zone);

// сформировать строку вывода из градусов в градусы, минуты и сикунды
QString debugSec(double X);
}

// -------------------------------------------------------

// Класс содержит 3 координаты (в зависимости от типа координатной системы)
// + систему кооринат + тип координататной системы (геодезическая или прямоугольная)
class Coord
{
public:
    enum System{WGS84, PZ90, PZ9002, SK42, SK95};
    enum CoordType{Geodetic, Rectangular};

    Coord(qreal _x = 0., qreal _y = 0., qreal _z = 0., System sys = WGS84, CoordType type = Geodetic);

    System coordSystem() const;
    CoordType type();

    Coord &toWGS84();
    Coord &toPZ90();
    Coord &toPZ9002();
    Coord &toSK42();
    Coord &toSK95();

    Coord &toGeo();
    Coord &toRect();

    QString dbg();
    QPointF toPoint();

public:
    double x;    // X, B - X / широта
    double y;    // Y, L - Y / долгота
    double z;    // Z, H - высота от центра массы Земли / высота над уровнем моря

private:
    System _system;
    CoordType _type; // true - rect; false - geo;
};

// -------------------------------------------------------
// Преобразвание координат между тремя системами
// сферической проекцией меркатора, геодезическими WGS84 и плоскими прямоугольными СК-42
// (без учета высот)
namespace CoordTranform {

// из "мировых" в геодезические WGS-84 и обратно
QPointF worldToGeo(const QPointF &xy);
QPointF geoToWorld(const QPointF &latLon);

// из "мировых" в плоские прямоугольные SK-42 и обратно
QPointF worldToSK42(const QPointF &xy);
QPointF sk42ToWorld(const QPointF &sk42);

// из геодезических в плоские прямоугольные SK-42 и обратно
QPointF geoToSK42(const QPointF &latLon);
QPointF sk42ToGeo(const QPointF &sk42);

// номер шестиградусной зоны в проекции Гаусса-Крюгера
int zoneSK42FromSK42(const QPointF &sk42);
int zoneSK42FromGeo(const QPointF &latLon);
int zoneSK42FromWorld(const QPointF &xy);

// из "мировых" в плоские прямоугольные SK-42 и обратно с указанием зоны
QPointF worldToSK42(const QPointF &xy, int zone);
QPointF sk42ToWorld(const QPointF &sk42, int zone);

// из геодезических в плоские прямоугольные SK-42 и обратно с указанием зоны
QPointF geoToSK42(const QPointF &latLon, int zone);
QPointF sk42ToGeo(const QPointF &sk42, int zone);

}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

// Эллиптическая проекция
template <typename T>
class ProjEll
{
public:
    ProjEll();
    //! из геодезических в мировые
    static QPointF geoToWorld(const QPointF &latLon);
    //! из мировых в геодезические
    static QPointF worldToGeo(const QPointF &xy);
    typedef T Params;
};

// -------------------------------------------------------

// Сферическая проекция
template <typename T>
class ProjSph
{
public:
    ProjSph();
    //! из геодезических в мировые
    static QPointF geoToWorld(const QPointF &latLon);
    //! из мировых в геодезические
    static QPointF worldToGeo(const QPointF &xy);
    typedef T Params;
};

// -------------------------------------------------------

// Преобразование тайлов подложки из эллиптической прекции в сферическую
struct MyUtils {
    static QPoint metersToEllipticTile(const QPointF &xy, int zoom);
    static QRectF tileBounds(const QPoint &tile, int zoom);
    static QRectF tileBoundsElliptic(const QPoint &tile, int zoom);
    static QPointF tileToGeo(const QPoint &tile, int zoom);
    static QPointF tileToGeoSperical(const QPoint &tile, int zoom);
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPCOORDS_H
