#ifndef MAPHELPERSUBHELPERS_H
#define MAPHELPERSUBHELPERS_H

#include <QObject>
#include <QPointF>
#include <QMouseEvent>
#include <QBasicTimer>
#include <QTimerEvent>
#include <QTransform>

#include "frame/mapframe.h"
#include "object/mapobject.h"

#include "interact/maphelpercommon.h"

#include "interact/maphelper.h"

// --------------------------------------------

#if defined ( QT_4 )
#undef signals
#undef Q_SIGNALS
#define signals public
#define Q_SIGNALS public
#endif

// --------------------------------------------

// TODO: correct drawer->localBound(...) - исправить на кривых безье для более быстрой отрисовки
namespace minigis {

// --------------------------------------------

class MapFrame;
class MapCamera;
class MapObject;
class MapLayerSystem;
class MapLayerObjects;

// --------------------------------------------
class MapSubHelperParent : public QObject
{
    Q_OBJECT

public:
    explicit MapSubHelperParent(QObject *parent = NULL)
        : QObject(parent) { }
    virtual ~MapSubHelperParent() { }

    virtual bool start(QPointF pos) = 0;
    virtual bool finish(QPointF pos) = 0;
    virtual bool proccessing(QPointF pos) = 0;

    virtual void reject(bool flag = true) { Q_UNUSED(flag); }
};

// --------------------------------------------

class MapSubHelper : public MapSubHelperParent
{
    Q_OBJECT

public:
    explicit MapSubHelper(MapFrame *map_, QObject *parent = 0);
    virtual ~MapSubHelper();

    enum { Type = stNoop };
    virtual quint8 type() const = 0;

signals:
    //! пользовательская обработка
    void handler(minigis::MHEvent*);
    //! просигналзировать об изменении мира
    void worldChanged(QTransform);
    //! самоотключение
    void autoReject();

public slots:
    void correctWorld(QTransform tr)
    {
        correctWorld(sender(), tr);
    }

    //! подправить что нужно при изменении мира
    void correctWorld(QObject *obj, QTransform tr)
    {
        if (obj == this)
            return;

        correctWorldImpl(tr);
    }

private:
    virtual void correctWorldImpl(QTransform) { }

protected:    
    bool isMouseDown;       // мышь нажата или нет
    QPointF lastScreenPos;  // последняя координата в экранах
    QPointF pressPos;       // координата вжатия

    MapFrame *map;          // карта
};

// --------------------------------------------

class MapSubHelperMove : public MapSubHelper
{
    Q_OBJECT

public:
    explicit MapSubHelperMove(MapFrame *map_, QObject *parent = 0);
    virtual ~MapSubHelperMove();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    //! init move bound rec (rgn must be valid and normolized)
    void initData(const QRectF &rgn);

    enum { Type = stMove };
    virtual quint8 type() const { return Type; }

protected:
    //! timer for inertial move
    virtual void timerEvent(QTimerEvent *event);

private:
    QPointF moveValidate(const QPointF &p) const;
    void sendWorldChanged(const QPointF &dt);

private:
    bool isTap;                 // возможен ли ТАП
    QRectF  moveRgn;            // ограниченная область перемещения

    // ----------------------- drag screen parameters

    QPoint speed;
    QPoint dragPos;
    enum ScrollState {
        NoScroll,
        ManualScroll,
        AutoScroll
    };
    ScrollState scrollState;
    QBasicTimer ticker;
};

// --------------------------------------------

class MapSubHelperScale : public MapSubHelper
{
    Q_OBJECT

public:
    explicit MapSubHelperScale(MapFrame *map_, QObject *parent = 0);
    virtual ~MapSubHelperScale();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    enum { Type = stScale };
    virtual quint8 type() const { return Type; }

    //! изменить масшатаб напрямую
    static QTransform scale(MapCamera *cam, const QPointF &mid, int delta);

private:
    void calculate(const QPointF &screenPos);
    void sendWorldChanged(const QPointF &mid, qreal scale);

private:
    qreal prevScale;    // предыдущий скель
};

// --------------------------------------------

class MapSubHelperRotate : public MapSubHelper
{
    Q_OBJECT

public:
    explicit MapSubHelperRotate(MapFrame *map_, QObject *parent = 0);
    virtual ~MapSubHelperRotate();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    enum { Type = stRotate };
    virtual quint8 type() const { return Type; }

private:
    void sendWorldChanged(const QPointF &mid, qreal angle);
};

// --------------------------------------------

class MapSubHelperSelect : public MapSubHelper
{
    Q_OBJECT

public:
    explicit MapSubHelperSelect(MapFrame *map_, bool continuedSelection_ = false, QObject *parent = 0);
    virtual ~MapSubHelperSelect();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    virtual void reject(bool flag = true);

    enum { Type = stSelect };
    virtual quint8 type() const { return Type; }

    //! проинциализировать режим начальным выделением
    void initData(MapObjectList *baseSelection);

    void deleteObject(QObject *obj);
    void deleteObjects(const QList<MapObject*> &objects);

    void appendCustomObjects(const QList<MapObject*> &baseSelection);
    QList<MapObjectPtr> getSelectedObjects() const;

private:
    QRectF rubberBand;              // область для выделения объектов
    MapObject *visualRubber;        // отрисовка области выделения
    MapLayerSystem *systemLayer;    // системный слой

    struct CanSelect
    {
        bool canSelect;     // можно ли выделить
        bool beSelected;    // оригинальное состояния
        bool flag;
    };
    QHash<MapObject*, CanSelect> canSelect; // временное выделение <объект, можно ли выделить, выделение до, метка>

    QSet<MapObjectPtr> selectedObjectsFull;    // полный список выделения
    QHash<MapObject*, bool> ignorGenList;      // хеш объект + игнорирование генерализации

    bool continuedSelection;
};

// --------------------------------------------
// --------------------------------------------
// --------------------------------------------

class SubHelper;
class MapSubMultiHelper : public MapSubHelper
{
    Q_OBJECT
public:
    explicit MapSubMultiHelper(MapFrame *map_, QObject *parent = 0);
    virtual ~MapSubMultiHelper();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);
    virtual void reject(bool flag = true);

    enum { Type = stNoop };
    virtual quint8 type() const { return Type; }

    virtual void addObjects(MapObjectPtr mo, bool isLowPriority = true) = 0;
    virtual void addObjects(const MapObjectList &objects, bool isLowPriority = true) = 0;
    virtual QList<MapObject*> objects() const = 0;
    virtual MapObject* object(MapObject *mark) = 0;
    virtual QList<MapObject*> markers() const = 0;
    virtual MapObject* marker(MapObject *obj) = 0;    

    QVariant flag(const QString &key, QVariant def = QVariant()) const
    {
        return dialogFlag.value(key, def);
    }
    virtual void setFlag(QString key, QVariant val)
    {
        dialogFlag.insert(key, val);
    }

    MapFrame*        getMap() const { return map; }
    MapCamera*       getCamera() const { return map->camera(); }
    MapLayerSystem*  getSystemLayer() const { return systemLayer; }
    MapLayerSystem*  getMarkerLayer() const { return markerLayer; }
    MapLayerObjects* getStandartLayer() const { return standartLayer; }
    const QPointF&   getLastScreenPos() const { return lastScreenPos; }
    const QPointF&   getPressPos() const { return pressPos; }

public slots:
    void changeObjects(QTransform tr, QRectF upd, bool flag)
    {
        changeObjectsImpl(tr, upd, flag);
    }

    void objectHasChanged(MapObject *obj)
    {
        objectHasChangedImpl(obj);
    }

signals:
    void objectsCountChanged();
    void correctSubWorld(QTransform);
    void correctSubWorld2(QTransform);
    void dataInited(HeOptions);

private:
    virtual void changeObjectsImpl(QTransform tr, QRectF upd, bool flag) = 0;
    virtual void objectHasChangedImpl(MapObject *obj) = 0;

protected:
    MapLayerSystem *systemLayer;    // системный слой
    MapLayerSystem *markerLayer;    // слой маркеров
    MapLayerObjects *standartLayer; // стандартный слой

    QVariantMap dialogFlag;

    QVector<SubHelper*> allSubhelpers; // список помощников
    SubHelper *subhelper;              // помощник
};

// --------------------------------------------
// --------------------------------------------
// --------------------------------------------

class MapSubHelperEdit : public MapSubMultiHelper
{
    Q_OBJECT
public:
    explicit MapSubHelperEdit(MapFrame *map, QObject *parent = 0);
    virtual ~MapSubHelperEdit();

    virtual void reject(bool flag = true);

    enum { Type = stEdit };
    virtual quint8 type() const { return Type; }

    virtual void addObjects(MapObjectPtr mo, bool isLowPriority = true);
    virtual void addObjects(const MapObjectList &objects, bool isLowPriority = true);

    virtual QList<MapObject*> objects() const;
    virtual MapObject* object(MapObject *mark);
    virtual QList<MapObject*> markers() const;
    virtual MapObject* marker(MapObject *obj);

    void initData(const MapObjectList &objects, HeOptions options);

public slots:
    // Реагируем на изменение объекта
    void recalcObject();
    void recalcObject(MapObject *obj);
    void deleteObject(QObject *obj);
    void changeClassCode(QString cls);
    void changeAttribute(int, QVariant attr);

private:
    virtual void correctWorldImpl(QTransform tr);
    virtual void changeObjectsImpl(QTransform tr, QRectF upd, bool flag);
    virtual void objectHasChangedImpl(MapObject *obj);

private:
    void clearIgnorGenList();

private:
    EditedObjects objectsMark;      // список объектов-маркеров
    QHash<MapObject*, bool> ignorGenList; // хеш объект + игнорирование генерализации

    HeOptions opt;
};

// --------------------------------------------

class MapSubHelperCreate : public MapSubMultiHelper
{
    Q_OBJECT
public:
    explicit MapSubHelperCreate(MapFrame *map, HeOptions opts = hoNone, QObject *parent = 0);
    virtual ~MapSubHelperCreate();

    virtual void reject(bool flag = true);

    enum { Type = stCreate };
    virtual quint8 type() const { return Type; }

    virtual void addObjects(MapObjectPtr mo, bool isLowPriority = true);
    virtual void addObjects(const MapObjectList &objects, bool isLowPriority = true);

    virtual QList<MapObject*> objects() const;
    virtual MapObject* object(MapObject *mark);
    virtual QList<MapObject*> markers() const;
    virtual MapObject* marker(MapObject *obj);

    void addPoint(QPointF const &p);
    void initData(MapObject *mo, int maxPoints = 0, HeOptions options = QFlag(hoPoint | hoHelpers));

public slots:
    // Реагируем на изменение объекта
    void recalcObject();
    void recalcObject(MapObject *obj);
    void deleteObject(QObject *obj);
    void changeClassCode(QString cls);
    void changeAttribute(int key, QVariant attr);

private:
    virtual void correctWorldImpl(QTransform tr);
    virtual void changeObjectsImpl(QTransform tr, QRectF upd, bool flag);
    virtual void objectHasChangedImpl(MapObject *obj);

    void init(MapObject *mo, int maxPoints, HeOptions options, bool askQuestions);

private:
    MapObject *currentObject;
    MapObject *visualObject;
    bool ignoreGen;

    bool tmpObj;

    HeOptions opt;
};

// --------------------------------------------
// --------------------------------------------
// --------------------------------------------

class SubHelper : public MapSubHelperParent
{
    Q_OBJECT

public:
    explicit SubHelper(MapSubMultiHelper *subhelper, QObject *parent = NULL)
        : MapSubHelperParent(parent), q_ptr(subhelper) { }
    virtual ~SubHelper() { }

    enum { Type = 0 };
    virtual quint8 type() const = 0;

protected:
    MapSubMultiHelper *q_ptr;
};

// --------------------------------------------

class SubHelperMark : public SubHelper
{
    Q_OBJECT

public:
    explicit SubHelperMark(MapSubMultiHelper *subhelper, QObject *parent = NULL);
    virtual ~SubHelperMark();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    virtual void reject(bool flag = true);

    enum { Type = 1 };
    virtual quint8 type() const { return Type; }

public slots:
    void clearMarkers();
    void checkMarkers();
    void correctMarkers(QTransform tr);

    void upMarkers(HeOptions);

private:
    void changeMarkers(QPointF pos, bool flag);
    void moveMarkers(QPointF delta);

    void initMarkesByObject();
    void initMarkersByPoint(QPointF mid);

protected Q_SLOTS:
    void updateMarkRect(QRectF &updateRect);

signals:
    void changeObjects(QTransform, QRectF, bool);

private:
    MarkerAnimation *midAni;
    MarkerAnimation *posAni;
    MarkerAnimation *rotateAni;
    MarkerAnimation *scaleAni;
    MarkerAnimationGroup markersGroup;

    int isMarkMode;                 // режим перемещения маркеров
    MapObject *midMark;             // метка центра
    MapObject *posMark;             // метка позиции
    MapObject *rotateMark;          // метка поворота
    MapObject *scaleMark;           // метка размера
};

// --------------------------------------------

class SubHelperEdit : public SubHelper
{
    Q_OBJECT

public:
    explicit SubHelperEdit(MapSubMultiHelper *subhelper, QObject *parent = NULL);
    virtual ~SubHelperEdit();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    virtual void reject(bool flag = true);

    enum { Type = 2 };
    virtual quint8 type() const { return Type; }

    // для использования совместно с customAction
    void initCustomObject(MapObject *mo, int mNumber, int pNumber);

public slots:
    void initData(HeOptions opt);
    void upDeleteMarkers();
    void correct(QTransform);
    void correct2(QTransform);

signals:
    void objectHasChanged(MapObject *);

private:
    bool changeControlPoint(const QPointF &pos);   // двигаем контрольную точку
    bool changeInterPoint(const QPointF &pos);     // добавлем промежуточную точку и двигаем её

    void changeHelperMetric(const QPolygonF &tmp); // меняем метрику visualObjec
    void changeCurrentObject();                    // меняем метрику захваченного объекта

private:
    int changinIndex;          // индекс изменяемой точкой
    int metricNumber;          // номер метрики
    MapObject *deletePoints;   // удаляемые точки
    bool markPoint;            // подсветить активную точку
    MapObject *activePoint;    // активная точка

    MapObject *currentObject; // текущий объект
    MapObject *visualObject;  // визуальный объект вся линия

    static const int updateDelta = 12;
};

// --------------------------------------------

class SubHelperAdd : public SubHelper
{
    Q_OBJECT

public:
    explicit SubHelperAdd(MapSubMultiHelper *subhelper, QObject *parent = NULL);
    virtual ~SubHelperAdd();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    virtual void reject(bool flag = true);

    enum { Type = 4 };
    virtual quint8 type() const { return Type; }

    void initData(MapObject *mo = NULL, int maxPoints_ = 0, HeOptions opt = hoNoCutMax);

signals:
    void autoReject();
    void objectHasChanged(MapObject*);

private:
    MapObject *currentObject;
    MapObject *visualObject;
    MapObject *visualTmp;
    int pointsCount;
    int maxPoints;
    bool noCutMax;
    bool moveObject;
    int addType;
    QPolygonF allClickedPoints;

    static const int updateDelta = 12;
    static const int moveTime    = 300;
};

// --------------------------------------------

class SubHelperCustom : public SubHelper
{
    Q_OBJECT

public:
    explicit SubHelperCustom(MapSubMultiHelper *subhelper, QObject *parent = NULL);
    virtual ~SubHelperCustom();

    virtual bool start(QPointF pos);
    virtual bool finish(QPointF pos);
    virtual bool proccessing(QPointF pos);

    virtual void reject(bool flag = true);

    enum { Type = 8 };
    virtual quint8 type() const { return Type; }

private:
    MapSubHelperParent *customAction;
    quint8 subtype;
};

// --------------------------------------------

} // namespace minigis

// --------------------------------------------

#if defined ( QT_4 )
#undef signals
#undef Q_SIGNALS
#define signals protected
#define Q_SIGNALS protected
#endif

// --------------------------------------------

#endif // MAPHELPERSUBHELPERS_H
