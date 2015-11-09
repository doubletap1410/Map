#ifndef MAPDEFS_H
#define MAPDEFS_H

// -------------------------------------------------------

#include <QDebug>
#include <QColor>
#include <qmath.h>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

//#define TILEANIMATION     // включить анимацию тайлов
#define TILEUPLOADLEVEL 5   // уровень подмены верхних тайлов при отсутствии искомых

static const int TileAnimationTime = 250;                // время появления тайлов
static const qreal GenerlizationDissolveRange   = 0.4;   // процент плавной генерализации область
static const qreal GenerlizationDissolveOpacity = 0.4;   // процент плавной генерализации прозрачность

#define Pi2    2 * M_PI
#define Pi3_2  3 * M_PI_2

template <typename T>
inline T degToRad(T a) { return a * M_PI / 180.; }
//TODO: struct IfThenElse<true, Ta, Tb>
//template <typename T>
//inline T degToRad(T const &a) {return a * M_PI / 180.;}

template <typename T>
inline T radToDeg(T a) { return a * M_1_PI * 180; }
//template <typename T>
//inline T radToDeg(T const &a) {return a * M_1_PI * 180;}

template <typename T>
inline T secToRad(T a) { return a / 206264.806; }

template <typename T>
inline T radToSec(T a) { return a * 206264.806; }

static const double LatMax   = 85.05112878;  // максимальная широта
static const double LonMax   = 180;          // максимальная долгота

static const double SVGsize  = 32;           // эталон размера svg

static const int TileSize = 256;             // размер тайла

//TODO: зависимость от экрана
static const double PixTo1m  = 3770.739064857; // количество пикселей в 1м  (3770.739064857 = 1000 / 0.2652 (размер пикселя в миллиметрах))
static const double PixTo1cm = 37.70739064857;
// нужен класс QWindow у которого можно взять экран - QScreen
//static const double PixTo1m  = QScreen().logicalDotsPerInch() * 1000 / 2.54;
//static const double PixTo1cm = PixTo1m * 0.01;

static const double MaxScale = 6000000;      // максимальное удаление камеры (1см:XXXXXм)
static const double MinScale = 0.6;          // максимальное приближение камеры (1см:XXXXXм)

static const double MaxScaleParam = PixTo1m / (MaxScale * 100); // максимальный scale камеры
static const double MinScaleParam = PixTo1m / (MinScale * 100); // минимальный scale камеры

static const QColor HighlightedHigh(255, 0, 255, 240);
static const QColor  HighlightedLow(128, 128, 128, 240);
static const QColor   SelectedColor(220, 220, 0, 240);

static const QString SystemLayerName("SystemLayer");
static const QString StandartLayerName("StandartLayer");

// TODO: перенести в атрибуты
static const int ImageSemantic       = 10;         // QImage
static const int WidgetSemantic      = 20;         // QWidget
static const int ColorSemantic       = 40;         // QColor
static const int PenSemantic         = 50;         // QPen
static const int PenSemanticLine     = 51;         // QPen
static const int PenSemanticSquare   = 52;         // QPen
static const int BrushSemantic       = 60;         // QBrush
static const int BrushSemanticSquare = 62;         // QBrush
static const int MarkersSemantic     = 33;         // какой маркер (см. mapdrawersystemmarkers);
static const int PointSemantic       = 34;         // QPointF

// -------------------------------------------------------

//! Ключ подложки
struct TileKey {
public:
    TileKey() : x(0), y(0), z(0), type(0) {}
    TileKey(int _x, int _y, int _z, int _type = 0): x(_x), y(_y), z(_z), type(_type) {}
    TileKey(const TileKey &key): x(key.x), y(key.y), z(key.z), type(key.type) {}
    ~TileKey() {}
    int x;
    int y;
    int z;
    int type;

    inline quint64 hash() const
    {
        return (quint64(y) << 40) | (quint64(x) << 16) | (quint64(z) << 8) | quint64(type);
    }
};

inline bool operator<(const TileKey &t1, const TileKey &t2)
{
    return t1.hash() < t2.hash();
}

inline bool operator==(const TileKey &t1, const TileKey &t2)
{
    return t1.hash() == t2.hash();
}

inline bool operator!=(const TileKey &t1, const TileKey &t2)
{
    return t1.hash() != t2.hash();
}

// -------------------------------------------------------

//! Атрибуты объекта
enum Attribute {
    ATTR_LEVEL_1 = 1,
    ATTR_LEVEL_2 = 1000,
    ATTR_LEVEL_3 = 1000000,

    attrOpacity = 1,        // 0-255

    attrPenColor,           // QColor
    attrPenWidth,           // int
    attrPenStyle,           // Qt::PenStyle
    attrPenCapStyle,        // Qt::PenCapStyle
    attrPenJoinStyle,       // Qt::PenJoinStyle

    attrBrushColor,         // QColor
    attrBrushStyle,         // Qt::BrushStyle

    attrFontFamily,         // QString
    attrFontColor,          // QColor
    attrFontPixelSize,      // int
    attrFontStretch,        // QFont::Stretch
    attrFontStyle,          // QFont::Style
    attrFontWeight,         // QFont::Weight
    attrFontBold,           // bool
    attrFontItalic,         // bool
    attrFontUnderline,      // bool
    attrFontStrikeOut,      // bool

    attrRoundedRectXRadius, // int
    attrRoundedRectYRadius, // int

    attrSmoothed,           // bool

    // ...
    attrStartArrow,         // minigis::ArrowType
    attrEndArrow,           // minigis::ArrowType

    // ...

    attrDescriptionObject,  // QString

    attrGenMinObject,       // int
    attrGenMaxObject,       // int

    attrRotatableObject,    // bool
    attrMirroredObject,     // bool

    attrSize,               // qreal
    attrRadius,             // int/qreal

    attrWidth,              // qreal
    attrHeight,             // qreal
    // ...
    attrGenIgnore,          // bool

    // ...
    attrElementsCount,      // int

    // TODO: ввалить. перевести на бъёмную метрику
    attrHeightList          // QList<int> (количество точек должно совпадать с количество точек в нулевой метрике)

    // ...
};
typedef QList<int> Attributes;
typedef QList<int> HeightList;
typedef QVector<HeightList> MultiHeightList;

/**
 * @brief attributeHumanName Человекоориентированное имя атрибута
 * @param attr атрибут
 * @return Человекоориентированное имя атрибута
 */
QString attributeHumanName(int attr);

/**
 * @brief attributeHumanName Человекоориентированный список атрибута
 * @param attr атрибуты
 * @return Человекоориентированный список атрибута
 */
QStringList attributeHumanName(Attributes const &attr);

/**
 * @brief attributeHumanName Человекоориентированный список атрибутов со значениями
 * @param values значения атрибутов
 * @param delimiter разделитель
 * @param sort сортировка атрибутов в списке
 * @return Человекоориентированный список атрибутов со значениями
 */
QStringList attributeHumanName(
        QHash<int, QVariant> const &values,
        QString const &delimiter = ": ",
        Attributes const &sort = Attributes()
        );

/**
 * @brief attributeHumanName Человекоориентированный список атрибутов со значениями в одной строке
 * @param values значения атрибутов
 * @param delimiter разделитель
 * @param spacer межстрочечный разделитель
 * @param sort сортировка атрибутов в списке
 * @return Человекоориентированный список атрибутов со значениями в одной строке
 */
QString attributeHumanName(
        QHash<int, QVariant> const &values,
        QString const &delimiter = ": ",
        QString const &spacer = ", ",
        Attributes const &sort = Attributes()
        );


// -------------------------------------------------------
enum MapOption {
    optNone               = 0x00,
    optIgnoreGen          = 0x01,
    optDissolveGen        = 0x02,
    optAntialiasing       = 0x04,
    optSubstrateSmoothing = 0x08
};
Q_DECLARE_FLAGS(MapOptions, MapOption)

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, MapOption e)
{
    QDebug &d = dbg.nospace();
    switch (e) {
    case optNone:               d << "optNone"; break;
    case optIgnoreGen:          d << "optIgnoreGen"; break;
    case optDissolveGen:        d << "optDissolveGen"; break;
    case optAntialiasing:       d << "optAntialiasing"; break;
    case optSubstrateSmoothing: d << "optSubstrateSmoothing"; break;
    default: d << "Unknown MapOption!";
    }
    return dbg.space();
}
#endif

// -------------------------------------------------------

//! режимы хелпера
enum HelperState {
    stMove      = 0x01,
    stRotate    = 0x02,
    stScale     = 0x04,
    stSelect    = 0x08,
    stCreate    = 0x10,
    stEdit      = 0x20,

    stNoop      = 0x00,
    stAll       = stNoop | stMove | stRotate | stScale | stSelect | stCreate | stEdit
};
Q_DECLARE_FLAGS(States, HelperState)

QDataStream &operator<<(QDataStream &out, HelperState const &state);
QDataStream &operator<<(QDataStream &out, States const &state);

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, HelperState e)
{
    QDebug &d = dbg.nospace();
    switch (e) {
    case stMove:   d << "stMove"; break;
    case stRotate: d << "stRotate"; break;
    case stScale:  d << "stScale"; break;
    case stSelect: d << "stSelect"; break;
    case stCreate: d << "stCreate"; break;
    case stEdit:   d << "stEdit"; break;
    case stNoop:   d << "stNoop"; break;
    case stAll:    d << "stAll"; break;
    default: d << "Unknown HelperState!";
    }
    return dbg.space();
}
#endif

// -------------------------------------------------------

//! подрежимы реактирования хелпера
enum EditType {
    editNoop         = 0x00,
    editAddPoint     = 0x01,
    editAddPen       = 0x02,
    editPoint        = 0x04,
    editMarkers      = 0x08,
    editMove         = 0x10,
    editSelect       = 0x20
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, EditType e)
{
    QDebug &d = dbg.nospace();
    switch (e) {
    case editNoop:     d << "editNoop"; break;
    case editAddPoint: d << "editAddPoint"; break;
    case editAddPen:   d << "editAddPen"; break;
    case editPoint:    d << "editPoint"; break;
    case editMarkers:  d << "editMarkers"; break;
    case editMove:     d << "editMove"; break;
    case editSelect:   d << "editSelect"; break;
    default: d << "Unknown EditType!";
    }
    return dbg.space();
}
#endif

// -------------------------------------------------------

//! настройки хелпреа
enum HelperOptions {
    hoNone        = 0x00, // ничего не делать
    hoPoint       = 0x01, // точки   (маркер объекта)
    hoLine        = 0x02, // линии   (маркер объекта)
    hoSquare      = 0x04, // площадь (маркер объекта)
    hoHelpers     = 0x08, // вспомогательные линии (маркер объекта)
    hoNoCutMax    = 0x10, // не обрезать максимальное количество точек, вернуть все
    hoDisableEdit = 0x20, // отключить режим редактирования (пока только для режима нанесения)
    hoMoveObject  = 0x40, // передвинуть объект а не добавлять в него точки (тольк для режима нанесения)
    hoMarkPoint   = 0x80, // пометить точку
};
Q_DECLARE_FLAGS(HeOptions, HelperOptions)

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, HelperOptions e)
{
    QDebug &d = dbg.nospace();
    switch (e) {
    case hoNone:     d << "hoNone"; break;
    case hoPoint:    d << "hoPoint"; break;
    case hoLine:     d << "hoLine"; break;
    case hoSquare:   d << "hoSquare"; break;
    case hoHelpers:  d << "hoHelpers"; break;
    case hoNoCutMax: d << "hoNoCutMax"; break;
    default: d << "Unknown HelperOptions!";
    }
    return dbg.space();
}
#endif

// -------------------------------------------------------

//! Типы
enum CoordType {
    ct_WGS84 = 1, //!< WGS-84
    ct_SK42,      //!< СК-42
    ct_SK45,      //!< СК-45
    ct_PZ90,      //!< ПЗ-90
    ct_PZ90_02,   //!< ПЗ-90.02
    ct_PZ95       //!< ПЗ-95
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, CoordType e)
{
    QDebug &d = dbg.nospace();
    switch (e) {
    case ct_WGS84:   d << "ct_WGS84"; break;
    case ct_SK42:    d << "ct_SK42"; break;
    case ct_SK45:    d << "ct_SK45"; break;
    case ct_PZ90:    d << "ct_PZ90"; break;
    case ct_PZ90_02: d << "ct_PZ90_02"; break;
    case ct_PZ95:    d << "ct_PZ95"; break;
    default: d << "Unknown CoordType!";
    }
    return dbg.space();
}
#endif

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#ifndef QT_STATIC
#    if defined(QT_BUILD_POSITIONING_LIB)
#      define MAP_CLASS_EXPORT         Q_DECL_EXPORT
#    else
#      define MAP_CLASS_EXPORT         Q_DECL_IMPORT
#    endif
#else
#    define Q_POSITIONING_EXPORT
#endif

Q_DECLARE_METATYPE(minigis::HeightList)
Q_DECLARE_METATYPE(minigis::MultiHeightList)
Q_DECLARE_METATYPE(minigis::MapOptions)

// -------------------------------------------------------

#endif // MAPDEFS_H
