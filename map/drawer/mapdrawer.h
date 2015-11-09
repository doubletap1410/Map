#ifndef MAPDRAWER_H
#define MAPDRAWER_H

// -------------------------------------------------------

#include <QObject>
#include <QRect>
#include <QPolygonF>
#include <QImage>
#include <QPainter>

#include "object/mapobject.h"
#include "core/mapdefs.h"
#include "core/mapmetric.h"

// -------------------------------------------------------

class QPainter;
class QSvgRenderer;

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

#define DRAWERANGLELENGTH 150

double GeneralizationExpCoefficient(double x);

#define CHECKGENERALIZATIONPAINT(object, zoom, gmin, gmax, checkGen)                    \
    MapObject const *o = object;                                                        \
    while (o->parentObject())                                                           \
        o = o->parentObject();                                                          \
    checkGen = !o->attribute(attrGenIgnore, !checkGen).toBool();                        \
    if (checkGen && !options.testFlag(optIgnoreGen)) {                                  \
        gmin = o == object ? o->attribute(attrGenMinObject, set->generalizMin).toReal() \
                           : o->attributeDraw(attrGenMinObject).toReal();               \
        gmax = o == object ? o->attribute(attrGenMaxObject, set->generalizMax).toReal() \
                           : o->attributeDraw(attrGenMaxObject).toReal();               \
        if (options.testFlag(optDissolveGen)) {                                         \
            qreal gz = (gmax - gmin) * GenerlizationDissolveRange;                      \
            qreal m = gmin - gz * exp(GeneralizationExpCoefficient(gmin - gz));         \
            qreal M = gmax + gz * exp(GeneralizationExpCoefficient(gmax + gz));         \
            if (zoom < m || zoom > M)                                                   \
                return;                                                                 \
        }                                                                               \
        else {                                                                          \
            if (zoom < gmin || zoom > gmax)                                             \
                return;                                                                 \
        }                                                                               \
    }

#define CHECKGENERALIZATIONHIT(object, zoom)                                                    \
    MapObject const *o = object;                                                                      \
    while (o->parentObject())                                                                   \
        o = o->parentObject();                                                                  \
    if (!o->attribute(attrGenIgnore, false).toBool() && !options.testFlag(optIgnoreGen)) {       \
        qreal gmin = o == object ? o->attribute(attrGenMinObject, set->generalizMin).toReal()   \
                           : o->attributeDraw(attrGenMinObject).toReal();                       \
        qreal gmax = o == object ? o->attribute(attrGenMaxObject, set->generalizMax).toReal()   \
                           : o->attributeDraw(attrGenMaxObject).toReal();                       \
        if (options.testFlag(optDissolveGen)) {                                                 \
            qreal gz = (gmax - gmin) * GenerlizationDissolveRange;                              \
            qreal m = gmin - gz * exp(GeneralizationExpCoefficient(gmin - gz));                 \
            qreal M = gmax + gz * exp(GeneralizationExpCoefficient(gmax + gz));                 \
            if (zoom < m || zoom > M)                                                           \
                return false;                                                                   \
        }                                                                                       \
        else if (zoom < gmin || zoom > gmax)                                                    \
            return false;                                                                       \
    }

#define PAINTERGENERALIZATION(painter, zoom, chekGen, gmin, gmax)                         \
    if (chekGen && !options.testFlag(optIgnoreGen) && options.testFlag(optDissolveGen)) { \
        qreal gz = (gmax - gmin) * GenerlizationDissolveRange;                            \
        qreal mint = gz * exp(GeneralizationExpCoefficient(gmin - gz));                   \
        qreal Mint = gz * exp(GeneralizationExpCoefficient(gmax + gz));                   \
        qreal m = gmin - mint;                                                            \
        qreal M = gmax + Mint;                                                            \
        if (m <= zoom && zoom <= gmin) {                                                  \
            qreal opacity = 1 - (gmin - zoom) / mint;                                     \
            painter->setOpacity(opacity);                                                 \
        }                                                                                 \
        else if (gmax <= zoom && zoom <= M)  {                                            \
            qreal opacity = (M - zoom) / Mint;                                            \
            painter->setOpacity(opacity);                                                 \
        }                                                                                 \
    }

// --------------

#define CHECKSELECTCOLOR(COLOR)                                  \
    QColor COLOR = Qt::transparent;                              \
    if (object->selected())                                      \
        COLOR = SelectedColor;                                   \
    else if (object->highlighted() == MapObject::HighligtedHigh) \
        COLOR = HighlightedHigh;                                 \
    else if (object->highlighted() == MapObject::HighligtedLow)  \
        COLOR = HighlightedLow;

// --------------

#define SAVEALPHACHANNEL(img, alpha) \
    QImage alpha;                    \
    if (img.hasAlphaChannel())       \
        alpha = img.alphaChannel();

// --------------

#define RESTOREALPHACHANNEL(img, alpha) \
    if (!alpha.isNull())                \
        img.setAlphaChannel(alpha);

// --------------

#define DRAWSELECTEDIMAGE(painter, image, color)                      \
    {                                                                 \
        QPainter painter(&image);                                     \
        painter.setCompositionMode(QPainter::CompositionMode_Screen); \
        painter.fillRect(image.rect(), color);                        \
        painter.end();                                                \
    }

// --------------

#define FONTDRAWER(font)                                                        \
    font.setFamily(object->attribute(attrFontFamily, set->family).toString());  \
    font.setPixelSize(object->attribute(attrFontPixelSize, set->size).toInt()); \
    font.setWeight(object->attribute(attrFontWeight, set->weight).toInt());     \
    font.setItalic(object->attribute(attrFontItalic, set->italic).toBool());

#define PENDRAWER(selectColor)                                                  \
    QColor c = object->attribute(attrPenColor, set->lineColor).value<QColor>(); \
    if (!opacity.isNull())                                                      \
        c.setAlpha(opacity.toInt());                                            \
    if (selectColor != Qt::transparent)                                         \
        c = selectColor;                                                        \
    QPen pen;                                                                   \
    QVariant _penw = object->attribute(attrPenWidth);                           \
    QVariant _pens = object->attribute(attrPenStyle);                           \
    QVariant _penc = object->attribute(attrPenCapStyle);                        \
    QVariant _penj = object->attribute(attrPenJoinStyle);                       \
    painter->setPen(QPen(                                                       \
        QBrush(c),                                                              \
        _penw.isNull() ? set->lineWidth : _penw.toInt(),                        \
        _pens.isNull() ? set->lineStyle : Qt::PenStyle(_pens.toInt()),          \
        _penc.isNull() ? Qt::FlatCap    : Qt::PenCapStyle(_penc.toInt()),       \
        _penj.isNull() ? Qt::BevelJoin  : Qt::PenJoinStyle(_penj.toInt())       \
                         ));

#define BRUSHDRAWER(selectColor)                                                                                            \
    QColor brushColor(object->attribute(attrBrushColor, set->brushColor).value<QColor>());                                  \
    if (!opacity.isNull())                                                                                                  \
        brushColor.setAlpha(opacity.toInt());                                                                               \
    if (selectColor != Qt::transparent)                                                                                     \
        brushColor = selectColor;                                                                                           \
    painter->setBrush(QBrush(brushColor, Qt::BrushStyle(object->attribute(attrBrushStyle, (int)set->brushStyle).toInt())));

// -------------------------------------------------------

class PainterSaver
{
public:
    PainterSaver(QPainter *p) {painter = p; painter->save();}
    ~PainterSaver() {painter->restore();}

private:
    QPainter *painter;
};

// -------------------------------------------------------

class MapCamera;

// -------------------------------------------------------

class MapDrawerPrivate;
/**
 * @brief Рисовальщик объектов
 */
class MapDrawer : public QObject
{
    Q_OBJECT

public:
    explicit MapDrawer(QObject *parent = 0);
    virtual ~MapDrawer();
    
public:
    /**
     * @brief exportDrivers экспорт списка классификаторов которые могут быть отрисованы
     * @return список поддерживаемых классификаторов
     */
    virtual QStringList exportedClassCodes() const = 0;

    /**
     * @brief init инициализация рисовальщика
     * @return успешность инициализации
     */
    virtual bool init() = 0;

    /**
     * @brief finish завершение работы
     * @return успешность выполнения операции
     */
    virtual bool finish() = 0;

    /**
     * @brief paint отрисовать объект
     * @param object объект
     * @param painter паинтер
     * @param rgn регион отрисовки
     * @param camera камера
     */
    virtual void paint(MapObject const *object, QPainter *painter, QRectF const &rgn, MapCamera const *camera, MapOptions options = optNone) = 0;

    /**
     * @brief hit определить попадает ли данный объект в заданную область
     * @param object объект
     * @param rgn регион
     * @return истина - если объект попадает в область
     */
    virtual bool hit(MapObject const *object, QRectF const &rgn, MapCamera const *camera, MapOptions options = optNone) = 0;

    /**
     * @brief boundRect описывающий прямоугольник (только для объектов без родителя)
     * @param object объект
     * @return возвращает описанный прямоугольник для объекта (в мировых координатах)
     */
    virtual QRectF boundRect(MapObject const *object) = 0;

    /**
     * @brief localBound описывающий прямоугольник
     * @param object объект
     * @return возвращает описанный прямоугольник для объекта (в локальных координатах)
     */
    virtual QRectF localBound(MapObject const *object, MapCamera const *camera) = 0;

    /**
     * @brief isValid проверка объекта на валидность
     * @return
     */
    virtual bool isValid(MapObject const *object) = 0;

    /**
     * @brief isEmpty пустой ли объект
     * @return
     */
    virtual bool isEmpty(MapObject const *object) = 0;

    /**
     * @brief attributes атрибуты художника
     * @param cls класскод
     * @return атрибуты художника в том числе статические
     */
    virtual Attributes attributes(QString const &cls = QString()) const = 0;

    /**
     * @brief attribute значение атрибута
     * @param uid класскод
     * @param attr атрибут
     * @return значение атрибута
     */
    virtual QVariant attribute(QString const &uid, int attr) const = 0;

    /**
     * @brief attributes список значений атрибутов
     * @param uid класскод
     * @param attr список стрибутов
     * @return список значений атрибутов
     */
    virtual QHash<int, QVariant> attributes(QString const &uid, Attributes const &attr) const;

    /**
     * @brief pos
     * @param object
     * @return возвращает позицию объекта
     */
    virtual QPointF pos(MapObject const *object) = 0;

    /**
     * @brief angle
     * @param object
     * @return возвразает угол поворота объекта
     */
    virtual qreal angle(MapObject const *object, MapCamera const *camera) = 0;

    /**
     * @brief controlPoints спросить контрольные точки
     * @param object объект
     * @return контрольные точки
     */
    virtual QVector<QPolygonF> controlPoints(MapObject const *object, MapCamera const *camera) = 0;

    /**
     * @brief canAddMetric можно ли добавить контрольную точку
     * @param object объект
     * @return можно ли
     */
    virtual bool canAddControlPoints(MapObject const *object) = 0;

    /**
     * @brief canAddPen можно ли добавлять точки "пером"
     * @param object объект
     * @return можно ли
     */
    virtual bool canAddPen(MapObject const *object) = 0;

    /**
     * @brief correctMetric корректирует метрику объекта
     * @param object объект
     * @param number номер метрки
     * @param metric метрика
     */
    virtual QVector<QPolygonF> correctMetric(MapObject const *object, QPolygonF metric, int number) = 0;

    /**
     * @brief fillMetric заполнить недостающую метрику
     * @param object объект
     * @param metric метрика
     * @return новая метрика
     */
    virtual QVector<QPolygonF> fillMetric(MapObject const *object, MapCamera const *camera) = 0;

    /**
     * @brief whereAddPoint куда добавить новую точку
     * @param object объект
     * @return индекс метрики ("-1" - в любое место)
     */
    virtual int whereAddPoint(MapObject const *object) = 0;

    /**
     * @brief canRemovePoint можно ли удалить точку
     * @param object объект
     * @param metricNumber номер метрики с удаляемой точкой
     * @return можно ли
     */
    virtual bool canRemovePoint(MapObject const *object, int metricNumber = 0) = 0;

    /**
     * @brief canUseMarkMode можно ли использовать режим меток
     * @return
     */
    virtual bool canUseMarkMode() = 0;

    /**
     * @brief markerType какие маркеры использовать (1 - точки; 2 - линия; 4 - площадь)
     * @return
     */
    virtual int markerType() = 0;

    /**
     * @brief isClosed замкнутый
     * @param cls классификатор
     * @return
     */
    virtual bool isClosed(const QString &cls);

    /**
     * @brief renderImg нарисовать объект
     * @param painter паинтер
     * @param cls класскод объекта
     * @param rgn облать отрисовки
     * @param type тип объекта (1 - выделенный, 2 - подсвеченный, 3 - притушенный)
     * @param flag флаг отрисовки точки привязки
     * @param metric предпочитаемая метрика
     */
    virtual void renderImg(QPainter &painter, const QString &cls, QRectF rgn, int type = 0, bool flag = false, QPolygonF metric = QPolygonF()) = 0;

    /**
     * @brief grayScaled отрисовка картинки в оттенках серого
     * @param sourceImage исходное изображение
     * @return исходная картинка в оттенках серого
     */
    static QImage grayScaled(const QImage &sourceImage);

    /**
     * @brief lines построение вспомогательных линий
     * @param metric метрика
     * @return
     */
    virtual QVector<QLineF> lines(MapObject const *object);

    /**
     * @brief принудительно скрыть объект
     * @param object объект
     * @return
     */
    virtual void hide(MapObject const *object);

    /**
     * @brief нужно ли дополнительно обрабатывать сигнал слоя о скрытии
     * @return нужно лм
     */
    virtual bool needLayerVisibilityChangedCatch();

protected:
    /**
     * @brief rendererForElement получить рендерер для отрисовки временного SVG
     * @param elementId классификатор объекта
     * @param color цвет для наложения
     * @param bounds границы объекта
     * @param sourceRenderer рендерер для отрисовки объекта
     * @return рендерер для отрисовки временного SVG
     */
    QSvgRenderer* rendererForElement(const QString &elementId, const QColor &color, const QRectF &bounds, QSvgRenderer &sourceRenderer);

protected:
    explicit MapDrawer(MapDrawerPrivate &dd, QObject *parent = 0);
    MapDrawerPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(MapDrawer)
    Q_DISABLE_COPY(MapDrawer)
};


// -------------------------------------------------------

//#undef CHECKGENERALIZATIONPAINT
//#undef CHECKGENERALIZATIONHIT
//#undef PAINTERGENERALIZATION
//#undef FONTDRAWER
//#undef PENDRAWER
//#undef BRUSHDRAWER
//#undef IMAGEDRAWER

// -------------------------------------------------------

} // namespace minigis

Q_DECLARE_OPERATORS_FOR_FLAGS(minigis::MapOptions)

// -------------------------------------------------------

#endif // MAPDRAWER_H


