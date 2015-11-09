#ifndef MAPHELPEREVENT_H
#define MAPHELPEREVENT_H

#include <QObject>
#include <QPointer>

#include "core/mapdefs.h"   // State + EditType
#include "frame/mapframe.h" // MapObject + MapObjectPtr

//----------------------------------------------------------

namespace minigis {

//----------------------------------------------------------

//! Тип сообщения графического редактора
enum MHEventType {
    etNoop,             //!< ничего

    etMovePress,        //!< начало движения
    etMoveRelease,      //!< конец движения

    etTap,              //!< данные по объектам по ТАП-у

    etScope,            //!< запросить область действия

    etCanObject,        //!< можно ли сделать что либо с объектом
    etCanSelect,        //!< можно ли выделить объект

    etObjectList,       //!< список объектов за последнее выделение
    etSelectedObjects,  //!< список выделенных объектов

    etEditedObjects,    //!< список измененных объектов

    etClickedPoints,    //!< список нанесенных точек
    etAllClickedPoints, //!< список всех нанесенных точек

    etChangedPoint,     //!< измененная точка
    etPointRemoved,     //!< точка удалена

    etPointInserted,    //!< точка добавлена
    etPointDrag,        //!< точка захвачена
    etPointDrop,        //!< точка отпущена

    etMarkMode,         //!< можно ли ли включить режим меток

    etCanAddPoint,      //!< можно ли добавлять точки
    etCanRemovePoint,   //!< можно ли удалить точку
    etCanChangePoint,   //!< можно ли изменять точки
    etCachedMode,       //!< спросить один раз или спрашивать постоянно

    etCustomAction,     //!< кастомный режим редактирования

    etModeRejected,     //!< режим отключился
    etFinished          //!< оключились от хелпера
};

//----------------------------------------------------------

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, MHEventType e)
{
    QDebug &d = dbg.nospace();
    switch (e) {
    case etNoop:             d << "etNoop"; break;
    case etMovePress:        d << "etMovePress"; break;
    case etMoveRelease:      d << "etMoveRelease"; break;
    case etTap:              d << "etTap"; break;
    case etScope:            d << "etScope"; break;
    case etCanObject:        d << "etCanObject"; break;
    case etCanSelect:        d << "etCanSelect"; break;
    case etObjectList:       d << "etObjectList"; break;
    case etSelectedObjects:  d << "etSelectObject"; break;
    case etEditedObjects:    d << "etEditedObjects"; break;
    case etClickedPoints:    d << "etClickedPoints"; break;
    case etAllClickedPoints: d << "etAllClickedPoints"; break;
    case etChangedPoint:     d << "etChangedPoint"; break;
    case etPointRemoved:     d << "etPointRemoved"; break;
    case etPointInserted:    d << "etPointInserted"; break;
    case etPointDrag:        d << "etPointDrag"; break;
    case etPointDrop:        d << "etPointDrop"; break;
    case etMarkMode:         d << "etMarkMode"; break;
    case etCanAddPoint:      d << "etCanAddPoint"; break;
    case etCanRemovePoint:   d << "etCanRemovePoint"; break;
    case etCanChangePoint:   d << "etCanChangePoint"; break;
    case etCachedMode:       d << "etIsCached"; break;
    case etCustomAction:     d << "etCustomAction"; break;
    case etModeRejected:     d << "etModeRejected"; break;
    case etFinished:         d << "etFinishHelper"; break;
    default:                 d << "Unknown MHEventType!";
    }
    return dbg.space();
}
#endif

//----------------------------------------------------------

//! базовое событие графического редактора
class MHEvent
{
public:
    MHEvent() { _type = etNoop; }
    virtual ~MHEvent() {}
    //! тип события
    inline virtual MHEventType type() const { return _type; }
    //! привести событие к заданному типу. в случае неудачи - крах приложения
    template<typename T> T *toType() { T *r = dynamic_cast<T*>(this); Q_ASSERT(r); return r; }
    //! попытаться привести событие к заданному типу. в случае неудачи - пустой указатель
    template<typename T> T *toTypeE() { return dynamic_cast<T*>(this); }

protected:
    MHEventType _type;
};

//----------------------------------------------------------
class MHEMoveMouse : public MHEvent
{
public:
    MHEMoveMouse(QPointF const &pos) : MHEvent(), _pos(pos) { }
    inline const QPointF position() const { return _pos; }

protected:
    QPointF _pos;
};

//----------------------------------------------------------
class MHEMovePress : public MHEMoveMouse
{
public:
    MHEMovePress(QPointF const &pos) : MHEMoveMouse(pos) { _type = etMovePress; }
};

//----------------------------------------------------------
class MHEMoveRelease : public MHEMoveMouse
{
public:
    MHEMoveRelease(QPointF const &pos) : MHEMoveMouse(pos) { _type = etMoveRelease; }
};

//----------------------------------------------------------
#ifndef HELPERNOMAPOBJECT
class MHETap : public MHEvent
{
public:
    MHETap(QList<MapObject *> const &objects, QPointF const &pos = QPointF())
        : MHEvent(), _objects(objects), _pos(pos) { _type = etTap; }
    inline QList<MapObject *> objects() const {return _objects; }
    inline const QPointF position() const { return _pos; }
protected:
    QList<MapObject *> _objects;
    QPointF _pos;
};
#endif
//----------------------------------------------------------

// TODO: unused
//! запрос! возможной области перемещения
class MHEScope : public MHEvent
{
public:
    MHEScope() : MHEvent() { _type = etScope; }

    inline const QRectF &rect() const { return _rect; }
    inline void setRect(const QRectF &r) { _rect = r; }

private:
    QRectF _rect;
};

//----------------------------------------------------------
#ifndef HELPERNOMAPOBJECT
// TODO: unused
//! запрос! можно ли сделать что либо с объектом
class MHECanObject : public MHEvent
{
public:
    MHECanObject(MapObjectPtr &mo, bool res = false) : MHEvent(), _mo(mo), _result(res) { _type = etCanObject; }
    inline MapObjectPtr mapObject() const { return _mo; }

    inline bool result() const { return _result; }
    inline void setResult(bool r) { _result = r; }

protected:
    MapObjectPtr _mo;
    bool _result;
};

//----------------------------------------------------------

//! запрос! можно ли выделить объект
class MHECanSelect : public MHECanObject
{
public:
    MHECanSelect(MapObjectPtr &mo, bool res = false) : MHECanObject(mo, res) {  _type = etCanSelect; }
};

//----------------------------------------------------------

//! вернуть! список объектов попавших в текщущую область
class MHEObjectList : public MHEvent
{
public:
    MHEObjectList(QList<MapObject*> mol) : MHEvent(), _mol(mol) { _type = etObjectList; }
    inline QList<MapObject*> mapObjects() const { return _mol; }

protected:
    QList<MapObject*> _mol; // список объектов
};

//----------------------------------------------------------

//! вернуть! список выделенных объектов
class MHESelectedObjects : public MHEObjectList
{
public:
    MHESelectedObjects(QList<MapObject*> mol) : MHEObjectList(mol) { _type = etSelectedObjects; }
};

//----------------------------------------------------------

//! вернуть! список измененных объектов
class MHEEditedObjects : public MHEObjectList
{
public:
    MHEEditedObjects(QList<MapObject*> mol) : MHEObjectList(mol) { _type = etEditedObjects; }
};
#endif
//----------------------------------------------------------

//! вернуть! список накликанных точек
class MHEClickedPoints : public MHEvent
{
public:
    MHEClickedPoints(QPolygonF poly) : MHEvent(), _poly(poly) { _type = etClickedPoints; }
    inline QPolygonF result() const { return _poly; }
private:
    QPolygonF _poly; // список точек
};

//----------------------------------------------------------

//! вернуть! список всех нанесенных точек
class MHEAllClickedPoints : public MHEClickedPoints
{
public:
    MHEAllClickedPoints(QPolygonF poly) : MHEClickedPoints(poly) { _type = etAllClickedPoints; }
};

//----------------------------------------------------------
#ifndef HELPERNOMAPOBJECT
//! вернуть! изменяемую точку
class MHEChangedPoint : public MHEvent
{
public:
    MHEChangedPoint(MapObjectPtr _mo, QPointF _prev, QPointF _current, int _metric, int _pos) :
        MHEvent(), mo(_mo), prevPoint(_prev), currentPoint(_current), metricNumber(_metric), position(_pos) { _type = etChangedPoint; }
public:
    MapObjectPtr mo;        // объект
    QPointF prevPoint;      // предыдущяя позиция точки
    QPointF currentPoint;   // новая позиция точки
    int metricNumber;       // метрика в которой храниться точка
    int position;           // позиция точки в метрике
};

//----------------------------------------------------------

//! вернуть! удаленную точку
class MHEPointRemoved : public MHEvent
{
public:
    MHEPointRemoved(MapObjectPtr _mo, QPointF _deletedPoint, int _metric, int _pos) :
        MHEvent(), mo(_mo), point(_deletedPoint), metricNumber(_metric), position(_pos) { _type = etPointRemoved; }
public:
    MapObjectPtr mo;        // объект
    QPointF point;          // удаленная точка
    int metricNumber;       // метрика в которой хранилась точка
    int position;           // позиция точки в метрике
};

//----------------------------------------------------------

//! вернуть! вставленную точку
class MHEPointInsert : public MHEvent
{
public:
    MHEPointInsert(MapObjectPtr _mo, QPointF _insertedPoint, int _metric, int _pos) :
        MHEvent(), mo(_mo), point(_insertedPoint), metricNumber(_metric), position(_pos) { _type = etPointInserted; }
public:
    MapObjectPtr mo;        // объект
    QPointF point;          // вставленная точка
    int metricNumber;       // метрика в которую вставилась точка
    int position;           // позиция точки в метрике
};

//----------------------------------------------------------

//! вернуть! индекс взятой точки
class MHEPointDrag : public MHEvent
{
public:
    MHEPointDrag(MapObjectPtr _mo, int _metric, int _pos) :
        MHEvent(), mo(_mo), metricNumber(_metric), position(_pos) { _type = etPointDrag; }
public:
    MapObjectPtr mo;        // объект
    int metricNumber;       // метрика в которой хранилась точка
    int position;           // позиция точки в метрике
};

//----------------------------------------------------------

//! вернуть! индекс отпущенной точки
class MHEPointDrop : public MHEvent
{
public:
    MHEPointDrop(MapObjectPtr _mo, int _metric, int _pos) :
        MHEvent(), mo(_mo), metricNumber(_metric), position(_pos) { _type = etPointDrop; }
public:
    MapObjectPtr mo;        // объект
    int metricNumber;       // метрика в которой хранилась точка
    int position;           // позиция точки в метрике
};

//----------------------------------------------------------

//! запрос! можно ли использвать режим меток
class MHEMarkMode : public MHEvent
{
public:
    MHEMarkMode(bool res = false) : MHEvent(), _result(res) { _type = etMarkMode; }
    inline bool result() const { return _result; }
    inline void setResult(bool r) { _result = r; }

protected:
    bool _result;
};

//----------------------------------------------------------

//! запрос! можно ли добавлять точки
class MHECanAddPoint : public MHEMarkMode
{
public:
    MHECanAddPoint(bool res = true, bool _flag = true, MapObject* _mo = NULL) :
        MHEMarkMode(res), flag(_flag), mo(_mo) { _type = etCanAddPoint; }

public:
    bool flag;         // точкой или пером
    MapObject* mo;     // объект
};

//----------------------------------------------------------

//! запрос! можно ли удалить точку
class MHECanRemovePoint : public MHEMarkMode
{
public:
    MHECanRemovePoint(bool res = true, MapObject* _mo = NULL, int _metricNumber = 0) :
        MHEMarkMode(res), mo(_mo), metricNumber(_metricNumber) { _type = etCanRemovePoint; }

public:
    MapObject* mo;     // объект
    int metricNumber;  // номер метрики
};

//----------------------------------------------------------

//! запрос! можно ли изменять точки
class MHECanChangePoint : public MHEvent
{
public:
    MHECanChangePoint(int type = 2, MapObject* _mo = NULL) :
        _res(type), mo(_mo) { _type = etCanChangePoint; }
    inline int result() const { return _res; }
    inline void setResult(int r) { _res = r; }

public:
    int _res; // 0 - нельзя, 1 - только опорные точки, 2 - все;
    MapObject* mo;     // объект
};
//----------------------------------------------------------

//! запрос! спросить один раз или спрашивать каждый раз
class MHECachedMode: public MHEMarkMode
{
public:
    MHECachedMode(bool res = true) : MHEMarkMode(res) { _type = etCachedMode; }
};
#endif

//----------------------------------------------------------

//! запрос! режим хелпера отключен
class MHEModeRejected: public MHEvent
{
public:
    MHEModeRejected(States state) : MHEvent(), rs(state) { _type = etModeRejected; }
    States rejectedStates() {return rs;}

private:
    States rs;
};

//----------------------------------------------------------

//! запрос! хелпер завершил работу и отключился
class MHEFinished: public MHEvent
{
public:
    MHEFinished(QPointer<QObject> reciver = NULL) : MHEvent(), recv(reciver) { _type = etFinished; }
    QPointer<QObject> reciver() const {return recv;}

private:
    QPointer<QObject> recv;
};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
#ifndef HELPERNOMAPOBJECT
class HelperAction
{
public:
    enum { Type = 0 };
    explicit HelperAction(EditType et = editNoop) : _subState(et), _type(Type) { }
    virtual ~HelperAction() { }

    EditType state() { return _subState; }
    quint8 type() const { return _type; }

protected:
    EditType _subState;
    int _type;
};

class HelperActionMove : public HelperAction
{
public:
    enum { Type = 1 };
    explicit HelperActionMove() : HelperAction(editMove) { _type = Type; }
};

class HelperActionSelect : public HelperAction
{
public:
    enum { Type = 2 };
    explicit HelperActionSelect() : HelperAction(editSelect) { _type = Type; }
};

class HelperActionCreate : public HelperAction
{
public:
    enum { Type = 3 };
    explicit HelperActionCreate(MapObject *object = NULL) : HelperAction(editAddPen), mo(object) { _type = Type; }
    virtual ~HelperActionCreate() { }

    MapObject *object() { return mo; }

private:
    MapObject *mo;
};

class HelperActionEdit : public HelperAction
{
public:
    enum { Type = 4 };
    explicit HelperActionEdit(MapObject *object, int metricNumber = 0, int pointNumber = 0)
        : HelperAction(editPoint), mo(object), metric(metricNumber), number(pointNumber) { _type = Type; }
    virtual ~HelperActionEdit() { }

    int metricNumber() { return metric; }
    int pointNumber()  { return number; }
    MapObject *object() { return mo; }

private:
    MapObject *mo;
    int metric;
    int number;
};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

//! диалог! кастомный режим эдитора
class MHECustomAction : public MHEvent
{
public:
    MHECustomAction(QPointF point) : MHEvent(), _point(point), _action(NULL) { _type = etCustomAction; }
    ~MHECustomAction() { }

    void setAction(QSharedPointer<HelperAction> action) { _action = action; }
    HelperAction *action() { return _action.data(); }

    QPointF point() { return _point; }

private:
    QPointF _point;
    QSharedPointer<HelperAction> _action;
};
#endif
//----------------------------------------------------------

} //namespace minigis

//----------------------------------------------------------

#endif // MAPHELPEREVENT_H
