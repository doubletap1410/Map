#ifndef MAPCAMERA_P_H
#define MAPCAMERA_P_H

#include <QObject>
#include <QPointF>
#include <QTransform>
#include <QMutex>
#include <QTimeLine>

// -------------------------------------------------------
namespace minigis {
// -------------------------------------------------------

class MapCamera;
class MapFrame;
class MapCameraPrivate : public QObject
{
    Q_OBJECT
public:
    explicit MapCameraPrivate(QObject *parent = 0);
    virtual ~MapCameraPrivate();

public:
    /**
     * @brief calcZoom пересчитать масштаб
     * @return
     */
    qreal calcZoom();
    /**
     * @brief changeMatrix пересчитать матрицу транформации
     */
    void changeMatrix();

    /**
     * @brief midScreenPos центр экрана
     * @return
     */
    QPointF midScreenPos();

    // функции для запуска анимации
    void moveAnimation(QPointF finish, int duration, QEasingCurve curve = QEasingCurve::OutCubic);
    void rotateAnimation(qreal finish, int duration, QEasingCurve curve = QEasingCurve::OutCubic);
    void scaleAnimation(qreal finish, int duration, QEasingCurve curve = QEasingCurve::OutCubic);

public slots:
    void updatePos(qreal);
    void updateAngle(qreal);
    void updateScale(qreal);

signals:
    void zoomChanged();
    void posChanged();
    void angleChanged();

public:
    QPointF location; //! позиция
    QPointF forward;  //! направление
    qreal   scale;    //! сжатие
    qreal   zoom;     //! масштаб карты (в 1 см)

    QSize screenSize; //! размеры карты
    QPointF midPos;   //! положение центра экрана

    QTransform transformMatrix;    //! матрица перевода из мировых в экранные координаты
    QTransform inversMatrix;       //! обратная матрица
    mutable bool beCacheMatrix;    //! флаг изменения матрицы
    mutable bool beCacheInvers;    //! флаг изменения обратной матрицы
    mutable bool beCacheZoom;      //! флаг изменения мастшаба

    QMutex mutex;

    QTimeLine lMove;
    QTimeLine lScale;
    QTimeLine lRotate;

    QPointF startPos;
    QPointF finishPos;

    QPointF startVect;
    qreal startAngle;
    qreal finishAngle;

    qreal startScale;
    qreal finishScale;

    static const int updateAnimationTime = 25;

    MapFrame *map;

    QRectF boundRect;

public:
    Q_DISABLE_COPY(MapCameraPrivate)
};

// -------------------------------------------------------
} //minigis
// -------------------------------------------------------
#endif // MAPCAMERA_P_H
