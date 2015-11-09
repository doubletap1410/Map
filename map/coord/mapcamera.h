#ifndef MAPCAMERA_H
#define MAPCAMERA_H

// -------------------------------------------------------

#include <QObject>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QTransform>
#include <QTimeLine>

// -------------------------------------------------------

#include "map/core/mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

/**
 * @brief The MapCamera class ориентация камеры в пространстве
 */
class MapCameraPrivate;
class MapFrame;
class MapCamera : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QPointF pos READ position WRITE moveTo)
    Q_PROPERTY(QPointF latLng READ latLng WRITE setLatLng)
    Q_PROPERTY(QString latLngStr READ latLngStr WRITE setLatLngStr)

    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal measure READ measure WRITE setMeasure)
    Q_PROPERTY(int zoomLevel READ zoomLevel WRITE setZoomLevel)

    Q_PROPERTY(qreal angle READ angle WRITE rotateTo)


public:
    explicit MapCamera(QObject *parent = 0);
    virtual ~MapCamera();

public:
    /**
     * @brief init установить карту
     * @param map
     */
    void init(MapFrame *map);

    /**
     * @brief moveTo переместиться в точку
     * @param pos точка
     * @param mid точка (в экранных координатах), координата которой == pos
     */
    Q_INVOKABLE void moveTo(QPointF pos, int duration = 0, QPointF mid = QPointF(), QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief moveBy сместиться на точку
     * @param delta точка
     */
    Q_INVOKABLE void moveBy(QPointF delta, int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief ensureVisible перейти в точку, если она находится за пределами viewport'а
     * @param pos точка (в системе Меркатора)
     * @return true если точка уже на экране, в противном случае false
     */
    Q_INVOKABLE bool ensureVisible(QPointF pos, int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief position получит позицию камеры
     * @return точка
     */
    Q_INVOKABLE QPointF position() const;

    /**
     * @brief setBounds задать ограничивающий прямоугольник
     */
    void setBounds(QRectF);

    /**
     * @brief bounds получить ограничевающий прямоугольник
     * @return
     */
    QRectF bounds() const;

    /**
     * @brief latLng получит позицию камеры (широта/долгота)
     * @return
     */
    QPointF latLng() const;

    /**
     * @brief setLatLng переместиться в точку
     * @param ll точка (широта/долгота)
     */
    void setLatLng(QPointF ll);

    /**
     * @brief latLngStr получит позицию камеры (широта/долгота)
     * @return
     */
    QString latLngStr() const;

    /**
     * @brief setLatLngStr переместиться в точку
     * @param ll точка (широта/долгота) через пробел
     */
    void setLatLngStr(QString ll);

    //------------------------------------------

    /**
     * @brief rotateTo повернуться до угола
     * @param angle угол в градусах
     * @param mid точка вращения (в экарнных координатах)
     */
    Q_INVOKABLE void rotateTo(qreal angle, QPointF mid = QPointF(), int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief rotateTo повернуться до вектора
     * @param angle вектор поворота
     * @param mid точка вращения (в экарнных координатах)
     */
    Q_INVOKABLE void rotateTo(QPointF angle, QPointF mid = QPointF(), int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief rotateBy повернуться на угол
     * @param delta угол в градуса
     * @param mid точка вращения (в экарнных координатах)
     */
    Q_INVOKABLE void rotateBy(qreal delta, QPointF mid = QPointF(), int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief angle угол поворота
     * @return угол в градусах
     */
    Q_INVOKABLE qreal angle() const;

    /**
     * @brief direction получить вектора направления камеры
     * @return вектор направления
     */
    QPointF direction() const;

    /**
     * @brief measurementError погрешность определения координат (метров в пикселе * 2)
     * @param pos точка (в меркаторах)
     * @return погрешность
     */
    qreal measurementError(QPointF pos) const;

    //------------------------------------------

    /**
     * @brief metricToScale перевод мастшаба в scale
     * @param metric масштаб
     * @return scale
     */
    static qreal metricToScale(qreal metric);

    /**
     * @brief scaleToMetric перевод scale в масштаб
     * @param scale
     * @return масштаб
     */
    static qreal scaleToMetric(qreal scale);

    /**
     * @brief setScale установить масштаб
     * @param sc масштаб
     * @param duration время анимации масштабирования
     * @param curve кривая анимации
     * @param mid точка масштабирования (в экранных координатах)
     */
    Q_INVOKABLE void setScale(qreal sc, QPointF mid = QPointF(), int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief scaleIn увеличить мастшаб (10% за 1 шаг)
     * @param stepCount количество шагов
     * @param duration время анимации масштабирования
     * @param curve кривая анимации
     * @param mid точка масштабирования (в экранных координатах)
     */
    Q_INVOKABLE void scaleIn(int stepCount = 1, QPointF mid = QPointF(), int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief scaleIn уменьшить мастшаб (10% за 1 шаг)
     * @param stepCount количество шагов
     * @param duration время анимации масштабирования
     * @param curve кривая анимации
     * @param mid точка масштабирования (в экранных координатах)
     */
    Q_INVOKABLE void scaleOut(int stepCount = 1, QPointF mid = QPointF(), int duration = 0, QEasingCurve curve = QEasingCurve::OutCubic);

    /**
     * @brief scale получить scale камеры
     * @return
     */
    Q_INVOKABLE qreal scale() const;

    /**
     * @brief zoom получить масштаб камеры
     * @return
     */
    Q_INVOKABLE qreal zoom() const;

    /**
     * @brief measure масштаб в метрах
     * @return
     */
    Q_INVOKABLE qreal measure() const;

    /**
     * @brief setMeasure задать масштаб в метрах
     */
    Q_INVOKABLE void setMeasure(qreal);

    /**
     * @brief zoomLevel вернуть номер уровня подложки с тайловой системе
     * @return
     */
    Q_INVOKABLE int zoomLevel() const;

    /**
     * @brief setZoomLevel задать уровень подложки в тайловой системе
     */
    Q_INVOKABLE void setZoomLevel(int);

    /**
     * @brief scaleForZoomLevel задать уровень подложки в тайловой системе
     */
    Q_INVOKABLE qreal scaleForZoomLevel(int);

    //------------------------------------------

    /**
     * @brief setScreenSize установить размер экрана
     * @param screen размер
     */
    void setScreenSize(QSize screen);

    /**
     * @brief screenSize получить размер экрана
     * @return размер
     */
    QSize screenSize() const;

    /**
     * @brief worldRect получить видимиый прямоугольник в мировых кооринатах
     * @return полигон (TopLeft, TopRight, BottomRight, BottomLeft)
     */
    QPolygonF worldRect() const;

    /**
     * @brief setMidPosition установить центр экрана
     * @param width позиция по широте (от 0 до 1 в процентах)
     * @param heigth позиция по высоте (от 0 до 1 в процентах)
     */
    void setMidPosition(qreal width, qreal heigth);
    void setMidPosition(QPointF mid);

    //------------------------------------------

    /**
     * @brief toWorld перевод точки из локальных в мировые координаты
     * @param pos точка в локальных координатах
     * @return точка в мировых координатах
     */
    QPointF toWorld(QPointF pos) const;

    /**
     * @brief toScreen перевод точки из мировых в локальные координаты
     * @param pos точка в мировых координатах
     * @return точка в локальных координатах
     */
    QPointF toScreen(QPointF pos) const;

    /**
     * @brief toScreen матрица трансформации в экранные координаты
     * @return матрица
     */
    const QTransform &toScreen() const;

    /**
     * @brief toWorld матрица трансформации в мировые координаты
     * @return матрица
     */
    const QTransform &toWorld() const;

Q_SIGNALS:
    void zoomChanged();
    void posChanged();
    void angleChanged();

private:
    Q_DECLARE_PRIVATE(MapCamera)
    Q_DISABLE_COPY(MapCamera)

    MapCameraPrivate *d_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPCAMERA_H

