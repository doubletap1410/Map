#ifndef MAPDRAWERLINE_H
#define MAPDRAWERLINE_H

// -------------------------------------------------------

#include "mapdrawer.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapDrawerLinePrivate;
class MapDrawerLine : public MapDrawer
{

public:
    explicit MapDrawerLine(QObject *parent = 0);
    virtual ~MapDrawerLine();

    /**
     * @brief exportDrivers экспорт списка классификаторов которые могут быть отрисованы
     * @return список поддерживаемых классификаторов
     */
    virtual QStringList exportedClassCodes() const;

    /**
     * @brief init инициализация рисовальщика
     * @return успешность инициализации
     */
    virtual bool init();

    /**
     * @brief finish завершение работы
     * @return успешность выполнения операции
     */
    virtual bool finish();

    /**
     * @brief paint отрисовать объект
     * @param object объект
     * @param painter паинтер
     * @param rgn регион отрисовки
     * @param camera камера
     */
    virtual void paint(MapObject const *object, QPainter *painter, QRectF const &rgn, MapCamera const *camera, MapOptions options = optNone);

    /**
     * @brief hit определить попадает ли данный объект в заданную область
     * @param object объект
     * @param rgn регион
     * @return истина - если объект попадает в область
     */
    virtual bool hit(MapObject const *object, QRectF const &rgn, MapCamera const *camera, MapOptions options = optNone);

    /**
     * @brief boundRect описывающий прямоугольник
     * @param object объект
     * @return возвращает описнный прямоугольник для объекта
     */
    virtual QRectF boundRect(MapObject const *object);

    /**
     * @brief localBound описывающий прямоугольник
     * @param object объект
     * @return возвращает описанный прямоугольник для объекта (в локальных координатах)
     */
    virtual QRectF localBound(MapObject const *object, const MapCamera *camera);

    /**
     * @brief isValid проверка объекта на валидность
     * @return
     */
    virtual bool isValid(MapObject const *object);

    /**
     * @brief isEmpty пустой ли объект
     * @return
     */
    virtual bool isEmpty(MapObject const *object);

    /**
     * @brief attributes атрибуты художника
     * @return атрибуты художника в том числе статические
     */
    virtual Attributes attributes(QString const &cls = QString()) const;

    /**
     * @brief attribute значение атрибута
     * @param uid класскод
     * @param attr атрибут
     * @return значение атрибута
     */
    virtual QVariant attribute(QString const &uid, int attr) const;

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
    virtual QPointF pos(MapObject const *object);

    /**
     * @brief angle
     * @param object
     * @return возвразает угол поворота объекта
     */
    virtual qreal angle(MapObject const *object, MapCamera const *camera);

    /**
     * @brief controlPoints спросить контрольные точки
     * @param object объект
     * @return контрольные точки
     */
    virtual QVector<QPolygonF> controlPoints(MapObject const *object, MapCamera const *camera);

    /**
     * @brief canAddMetric можно ли добавить контрольную точку
     * @param object объект
     * @return можно ли
     */
    virtual bool canAddControlPoints(MapObject const *object);

    /**
     * @brief canAddPen можно ли добавлять точки "пером"
     * @param object объект
     * @return можно ли
     */
    virtual bool canAddPen(MapObject const *object);

    /**
     * @brief correctMetric корректирует метрику объекта
     * @param object объект
     * @param number номер метрки
     * @param metric метрика
     */
    virtual QVector<QPolygonF> correctMetric(MapObject const *object, QPolygonF metric, int number);

    /**
     * @brief fillMetric заполнить недостающую метрику
     * @param object объект
     * @param metric метрика
     * @return новая метрика
     */
    virtual QVector<QPolygonF> fillMetric(MapObject const *object, const MapCamera *camera);

    /**
     * @brief whereAddPoint куда добавить новую точку
     * @param object объект
     * @return индекс метрики
     */
    virtual int whereAddPoint(MapObject const *object);

    /**
     * @brief canRemovePoint можно ли удалить точку
     * @param object объект
     * @param metricNumber номер метрики с удаляемой точкой
     * @return можно ли
     */
    virtual bool canRemovePoint(MapObject const *object, int metricNumber = 0);

    /**
     * @brief canUseMarkMode можно ли использовать режим меток
     * @return
     */
    virtual bool canUseMarkMode();

    /**
     * @brief markerType какие маркеры использовать
     * @return
     */
    virtual int markerType();

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
    virtual void renderImg(QPainter &painter, const QString &cls, QRectF rgn, int type = 0, bool flag = false, QPolygonF metric = QPolygonF());

protected:
    explicit MapDrawerLine(MapDrawerPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(MapDrawerLine)
    Q_DISABLE_COPY(MapDrawerLine)
};

// -------------------------------------------------------

} // minigis

// -------------------------------------------------------

#endif // MAPDRAWERLINE_H
