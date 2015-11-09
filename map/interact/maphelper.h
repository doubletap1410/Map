#ifndef MAPHELPER_H
#define MAPHELPER_H

#include <QObject>
#include <QPointer>

#include "core/mapdefs.h"
#include "frame/mapframe.h"
#include "object/mapobject.h"
#include "interact/maphelperevent.h"

// -------------------------------------------------------

#if defined ( QT_4 )
#undef signals
#undef Q_SIGNALS
#define signals public
#define Q_SIGNALS public
#endif

// -------------------------------------------------------

class QGestureEvent;
class QGraphicsRectItem;
class QTouchEvent;

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

// общий класс обработчика пользователя на карте
class MapHelperPrivate;
class MapHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit MapHelper(QObject *parent = 0);
    virtual ~MapHelper();

    // инициализация
    virtual void init(MapFrame *map = 0);
    // запустить режим работы с картой
    virtual bool start();
    // активность хэлпера
    bool isEnabled() const;

public:
    // тип режима
    virtual int type() const;
    // тихий режим. можно беспардонно его прерывать.
    virtual bool isStill() const;
    // запущен ли режим
    virtual bool isStarted() const;
    // попытаться завершить текущий режим
    bool clear();
    // владелец режима карты
    QObject *owner() const;
    // название режима карты
    virtual QString humanName() const;

public Q_SLOTS:
    // задать активность хэлпера
    void setEnabled(bool);

    // обработка клавиатуры
    virtual bool keyPress(QKeyEvent *event);
    virtual bool keyRelease(QKeyEvent *event);
    // обработка мыши
    virtual bool mouseDown(QMouseEvent *event);
    virtual bool mouseUp(QMouseEvent *event);
    virtual bool mouseMove(QMouseEvent *event);

    virtual bool wheelEvent(QWheelEvent *event);
    // жесты
    virtual bool gesture(QGestureEvent *event);
    virtual bool touch(QTouchEvent *event);

    virtual void resize(QSizeF, QSizeF);

Q_SIGNALS:
    void enabledChanged(bool);

protected slots:
    // завершить работу текущего режима
    virtual void finish();

protected:
    MapHelper(MapHelperPrivate &dd, QObject *parent = 0);
    QScopedPointer<MapHelperPrivate> d_ptr;

private:
    Q_DISABLE_COPY(MapHelper)
    Q_DECLARE_PRIVATE(MapHelper)
    
};

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

class MapHelperUniversalPrivate;
class MapHelperUniversal : public MapHelper
{
    Q_OBJECT

public:
    MapHelperUniversal(QObject *parent = 0);
    ~MapHelperUniversal();

    //! название режима карты
    virtual QString humanName() const;
    //! тип режима
    virtual int type() const;
    //! тихий режим. можно беспардонно его прерывать.
    virtual bool isStill() const;
    //! запустить хелпер
    virtual bool start();
    //! установить значения запускаемого режима
    void setRunData(QObject *recv, const char *abortSignal, const char *handlerSlot = 0);

    //! текущее состояние хэлпера
    minigis::States state() const;

    //! приёмник сообщений
    QObject *receiver() const;

    //! проверить включен ли режим
    bool hasMode(HelperState state) const;

public slots:
    //! перейти в режим перемещения в заданной области
    void move(const QRectF &rgn = QRectF());
    //! перейти в режим поворота
    void rotate();
    //! перейти в режим масштабирования
    void scale();
    //! перейти в режим выделения
    void select(MapObjectList *baseSelection = NULL);
    //! перейти в режим редактирования объектов
    void edit(const MapObjectList &objects = MapObjectList(), HeOptions options = QFlag(hoPoint | hoHelpers));
    //! перейти в режим создания объекта
    void create(MapObject *mo = NULL, int maxPoints = 0, HeOptions options = QFlag(hoPoint | hoHelpers));

    //! прекратить режим перемещения
    void moveStop() { reject(stMove); }
    //! прекратить режим поворота
    void rotateStop() { reject(stRotate); }
    //! прекратить режим масштабирования
    void scaleStop() { reject(stScale); }
    //! прекратить режим выделения
    void selectStop() { reject(stSelect); }
    //! прекратить режим редактирования объектов
    void editStop() { reject(stEdit); }
    //! прекратить режим создания объекта
    void createStop() { reject(stCreate); }

    //! снять выделение с объектов
    void deselectObjects(MapObject *object);
    void deselectObjects(const QList<MapObject *> objects);
    //! выделить объекты
    void selectObjects(MapObject *object);
    void selectObjects(const QList<MapObject *> objects);

    //! добавить точку
    void addPoint(const QPointF &pt);
    void addPoint(qreal x, qreal y);

    //! добавить объекты в edit
//    void addObject(const MapObjectPtr &object, bool isLowPriority = true);
//    void addObjects(const MapObjectList &objects, bool isLowPriority = true);

public slots:
    virtual bool keyPress(QKeyEvent *event);
    //! обработка мыши
    virtual bool mouseDown(QMouseEvent *event);
    virtual bool mouseUp(QMouseEvent *event);
    virtual bool mouseMove(QMouseEvent *event);
    //! колесо мыши
    virtual bool wheelEvent(QWheelEvent *event);
    //! мультитач
    virtual bool touch(QTouchEvent *event);
    //! ресайз ивент
    virtual void resize(QSizeF, QSizeF);

    //! сбросит режим
    void reject(States st = stAll);

    //! пересчитать маркеры
//    void feedback(const QList<minigis::MapObject *> &list);

    //! пересчитать маркеры объекта
//    void recalcObject();
//    void recalcObject(MapObject*);

    //! пересчитать все маркеры
//    void recalc();

signals:
    //! изменился режим хелпера
    void stateChanged(States);

protected slots:
    //! завершить работу хелпера
    virtual void finish();

protected:
    MapHelperUniversal(MapHelperUniversalPrivate &dd, QObject *parent = 0);

private:
    Q_DISABLE_COPY(MapHelperUniversal)
    Q_DECLARE_PRIVATE(MapHelperUniversal)
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#if defined ( QT_4 )
#undef signals
#undef Q_SIGNALS
#define signals protected
#define Q_SIGNALS protected
#endif

// -------------------------------------------------------

Q_DECLARE_OPERATORS_FOR_FLAGS(minigis::States)
Q_DECLARE_OPERATORS_FOR_FLAGS(minigis::HeOptions)

#endif // MAPHELPER_H
