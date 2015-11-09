#ifndef MAPDRAWERCOMMON_H
#define MAPDRAWERCOMMON_H

#include <QPainter>
#include <QtSvg>
#include <QTransform>

#include "object/mapobject.h"
#include "coord/mapcamera.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

//! ------------------------------------------------------
/**
 * @brief svgGenerator генерируем svg на Image и трансформируем его
 * @param render svgRender
 * @param p исходная точка приложения
 * @param ang угол поворота
 * @param cls классификатор svg
 * @param svgSize размер svg
 * @param anchorX якорь привязки по оси Х
 * @param anchorY якорь привязки по оси Y
 * @param start новая точка приложения
 * @return Image с svg
 */
QImage svgGenerator(QSvgRenderer &render, const QPointF &p, const qreal &ang, const QString &cls,
                    const qreal &svgSize, const quint8 &anchorX, const quint8 &anchorY, QPointF *start = NULL);
/**
 * @brief svgGenerator генерируем svg на Image и трансформируем его
 * @param render svgRender
 * @param cls классификатор svg
 * @param rgn описывающий прямоугольник для svg
 * @param tr матрица преобразования
 * @param invertedTr обратная матрица
 * @param start новая точка приложения
 * @return Image с svg
 */
QImage svgGenerator(QSvgRenderer &render, const QString &cls, const QRectF &rgn, const QTransform &tr = QTransform(),
                    const QTransform &invertedTr = QTransform(), QPointF *start = NULL);

/**
 * @brief svgRect
 * @param render svgRender
 * @param p точка приложения
 * @param cls классификатор svg
 * @param svgSize размер svg
 * @return описывающие прямоугольник для svg
 */
QRectF svgRect(QSvgRenderer &render, const QString &cls, const QPointF &p = QPointF(), const qreal &svgSize = 1);

//! ------------------------------------------------------
/**
 * @brief getParentPos возвращает позиция родителя и его угол поворота
 * @param object объект
 * @param camera камера
 * @param mid позиция родителя
 * @param angle угол поворота родителя
 */
void getParentPos(MapObject const *object, MapCamera const *camera, QPointF &mid, qreal &angle);
// TODO: return bool

//! ------------------------------------------------------
/**
 * @brief shiftPolygonSimple "простое" смещение полигона
 * @param origin оригинальные полигон
 * @param delta смещение
 * @param closed замкнутый полигон
 * @return новый полигон
 */
QPolygonF shiftPolygonSimple(const QPolygonF &origin, qreal delta, bool closed);
/**
 * @brief shiftPolygonSimple "сложное" смещение полигона (с обрезанием острых углов и с вырождением)
 * @param origin оригинальные полигон
 * @param delta смещение
 * @param closed замкнутый полигон
 * @return новый полигон
 */
QPolygonF shiftPolygonDifficult(const QPolygonF &origin, qreal delta, bool closed);

/**
 * @brief shiftPolygon смещение полигона
 * @param origin оригиная
 * @param delta дельта смешения
 * @param direction направление смещения
 * @param doubleDots индесксы сдвоенных точек
 * @return
 */
QPolygonF shiftPolygon(const QPolygonF &origin, qreal delta, bool direction, QList<int> *doubleDots = NULL);

//! функции hit ------------------------------------------
/**
 * @brief rectIntersectPoly пересечение прямоугольника с полым полигоном
 * @param rect прямоугольник
 * @param poly полигон
 * @param closed замкнутый полигон
 * @return
 */
bool rectIntersectPolyHollow(const QRectF &rect, const QPolygonF &poly, bool closed);

/**
 * @brief rectIntersectPolyFull пересечение прямоугольника с полным полигоном
 * @param rect прямоугольник
 * @param poly полигон
 * @return
 */
bool rectIntersectPolyFull(const QRectF &rect, const QPolygonF &poly);

/**
 * @brief rectIntersectLine пересечение прямоугольника с линией
 * @param rect прямоугольник
 * @param line линия
 * @return
 */
bool rectIntersectLine(const QRectF &rect, const QLineF &line);

/**
 * @brief rectIntersectEllipse пересечение прямоугольника с эллипсом
 * @param rect прямоугольник
 * @param mid центр эллипса
 * @param rx радиус по OX
 * @param ry радиус по OY
 * @param rotate угол наклона эллипса
 * @param isHollow полый эллипс
 * @return
 */
bool rectIntersectEllipse(const QRectF &rect, const QPointF &mid, const qreal &rx, const qreal &ry, const qreal &rotate, bool isHollow = true);

/**
 * @brief rectIntersectCircle пересечение прямоугольника с окружностью
 * @param rect прямоугольник
 * @param mid центр окружности
 * @param rx радиус окружности
 * @param isHollow закрашенная окружность
 * @return
 */
bool rectIntersectCircle(const QRectF &rect, const QPointF &mid, const qreal &rx, bool isHollow = true);

/**
 * @brief pixAnalyse пересечение прямоугольника с изображением - попиксельный анализ (пиксель пуст если alpha == 0)
 * @param img изображение
 * @param rect прямоугольник
 * @return
 */
bool pixAnalyse(const QImage &img, const QRectF &rect);

/**
 * @brief rectIntersectCubic пересечение прямоугольника с кривой Безье
 * @param rect прямоугольник
 * @param P0
 * @param P1
 * @param P2
 * @param P3
 * @return
 */
bool rectIntersectCubic(const QRectF &rect, QPointF P0, QPointF P1, QPointF P2, QPointF P3);

/**
 * @brief rectIntersectCubic пересечение прямоугольника с путем Безье
 * @param rect прямоугольник
 * @param path путь Безье
 * @return
 */
bool rectIntersectCubic(const QRectF &rect, const QPolygonF &path);

//! функции cut ------------------------------------------

/**
 * @brief cutPolygon обрезаем полигон рамкой
 * @param poly полигон
 * @param rect рамка
 * @param список параметров
 * @return список линий
 */
QList<QPolygonF> cutPolygon(const QPolygonF &poly, const QRectF &rect, bool closed = false, QList<qreal> *params = NULL);

//! функции для работы с эллипсом ------------------------
/**
 * @brief ellipseCoeff высчитывает коэффициенты эллипса
 * @param rx радиус по Х
 * @param ry радиус по Y
 * @param phi угол наклона эллипса
 * @param a
 * @param b
 * @param c
 * @param d
 */
void ellipseCoeff(qreal rx, qreal ry, qreal phi, qreal &a, qreal &b, qreal &c, qreal &d);

/**
 * @brief ellipseHaveCoord ищем точки пересечения с прямой || одной из осей и проверяем попадают ли они в границы
 * @param mid центр эллипса
 * @param a
 * @param b
 * @param c
 * @param d
 * @param coord прямая || одной из осей
 * @param min нижняя граница
 * @param max верхняя граница
 * @param xory выбор прямой (|| OX или || OY)
 * @return
 */
bool ellipseHaveCoord(const QPointF &mid, const qreal &a, const qreal &b, const qreal &c, const qreal &d, const qreal &coord, const qreal &min, const qreal &max, bool xory);

// -------------------------------------------------------

class ArrowTypePrivate;
/**
 * @brief The ArrowType class загрузчик типов окончаний линий
 */
class ArrowType
{
public:
    ~ArrowType();
    static ArrowType &inst();

    static QByteArray templateSVG();
    static QHash<uint, QPair<QByteArray, QRectF> > metric();
    static QByteArray metric(uint key);
    static QRectF bound(uint key);
    static QList<uint> keys();

private:
    Q_DISABLE_COPY(ArrowType)
    ArrowType();

    ArrowTypePrivate *d_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPDRAWERCOMMON_H
