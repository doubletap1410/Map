#ifndef MAPHELPERCOMMON_H
#define MAPHELPERCOMMON_H

#include <algorithm>
#include <QList>
#include <QHash>
#include <QStack>
#include <QTimeLine>

#include "core/mapmetric.h"
#include "object/mapobject.h"

// --------------------------------------------

namespace minigis {

// --------------------------------------------

class EditedObjects
{
public:
    typedef MapObject *ItemType;
    typedef QList<ItemType> ContainerType;
    typedef ContainerType::iterator iterator;
    typedef ContainerType::const_iterator const_iterator;
    typedef QHash<ItemType, ItemType> HashType;
    typedef HashType::iterator hIterator;
    typedef HashType::const_iterator const_hIterator;

    inline void clear() {
        _freeMark << _mark.toVector();
        std::for_each(_obj.begin(), _obj.end(), std::bind2nd(std::mem_fun(&MapObject::setSelected), false));
        _obj.clear();
        _mark.clear();
        _objMark.clear();
        _markObj.clear();
    }
    inline void deleteAll() {
        _mark << _freeMark.toList();
        std::sort(_mark.begin(), _mark.end());
        qDeleteAll(_mark.begin(), std::unique(_mark.begin(), _mark.end()));

        _obj.clear();
        _mark.clear();
        _objMark.clear();
        _markObj.clear();
    }
    inline ItemType remove(ItemType obj) {
        _obj.removeAll(obj);
        ItemType m = _objMark.take(obj);
        _mark.removeAll(m);
        _markObj.remove(m);
        _eObj.remove(obj);
        if (m) {
            m->setMetric(Metric(Mercator));
            freePush(m);
        }
        return m;
    }
    inline ItemType marker(ItemType obj) { return _objMark.value(obj); }
    inline ContainerType markers() const { return _objMark.values(); }
    inline ItemType object(ItemType obj) { return _markObj.value(obj); }
    inline ContainerType objects() const { return _obj; }
    inline bool contains(ItemType obj) const { return _objMark.contains(obj); }
    inline bool isEmpty() const { return _obj.isEmpty(); }
    inline int size() const { return _obj.size(); }
    inline void freePush(ItemType mark) { _freeMark.push(mark); }
    inline ItemType freePop() {
        if (_freeMark.isEmpty())
            return NULL;
        return _freeMark.pop();
    }

    inline ItemType first() { return _obj.first(); }
    inline ItemType last() { return _obj.last(); }

    inline iterator begin() { return _obj.begin(); }
    inline const_iterator begin() const { return _obj.begin(); }
    inline iterator end() { return _obj.end(); }
    inline const_iterator end() const { return _obj.end(); }

    inline iterator mBegin() { return _mark.begin(); }
    inline const_iterator mBegin() const { return _mark.begin(); }
    inline iterator mEnd() { return _mark.end(); }
    inline const_iterator mEnd() const { return _mark.end(); }

    inline hIterator hBegin() { return _objMark.begin(); }
    inline const_hIterator hBegin() const { return _objMark.begin(); }
    inline hIterator hEnd() { return _objMark.end(); }
    inline const_hIterator hEnd() const { return _objMark.end(); }

    inline void push_back(ItemType obj, ItemType mark) {
        _obj.push_back(obj);
        _mark.push_back(mark);
        _objMark[obj] = mark;
        _markObj[mark] = obj;
    }
    inline void push_front(ItemType obj, ItemType mark) {
        _obj.push_front(obj);
        _mark.push_front(mark);
        _objMark[obj] = mark;
        _markObj[mark] = obj;
    }

    inline ContainerType edited() {
        ContainerType res = _eObj.toList();
        _eObj.clear();
        return res;
    }
    inline void edited(ItemType obj) { _eObj.insert(obj); }
    inline void editedAll() { _eObj += _obj.toSet(); }

private:
    ContainerType _obj;     // список объектов с приоритетом обработки от высшего к низшему
    ContainerType _mark;    // список маркеров
    HashType _objMark;      // <global, local> список объектов-маркеров
    HashType _markObj;      // <local, global> список маркеров-объектов

    QStack<ItemType> _freeMark;   // стек созданных пустых объектов-меток
    QSet<ItemType> _eObj;         // список изменённых объектов
};

// --------------------------------------------
// --------------------------------------------
// --------------------------------------------

class MarkerAnimation : public QObject
{
    Q_OBJECT

public:
    MarkerAnimation(MapObject* obj, QObject *parent = NULL, int duration = 200, QEasingCurve curve = QEasingCurve::OutCubic);
    ~MarkerAnimation();

    QTimeLine::State state();
    void setEndPosition(QPointF endPos);
    void setEndOpacity(qreal opacity);
    MapObject *markerObj() const;

public slots:
    void start();
    void stop();

private slots:
    void updateMarker(qreal);
    void finishAni();

signals:
    void finished();
    void valueChanged(QRect);

private:
    MapObject *marker;
    QTimeLine *animator;
    QPointF startPos;
    QPointF endPos;

    qreal startOpacity;
    qreal endOpacity;

    static const int updateAnimationTime = 25;
};

// --------------------------------------------

class MarkerAnimationGroup : public QObject
{
    Q_OBJECT

public:
    MarkerAnimationGroup(QObject *parent = 0);
    ~MarkerAnimationGroup();

public slots:
    void addAnimation(MarkerAnimation *ani, int pauseAfter = 0);
    void start();
    void stop();
    bool isRunning();

private slots:
    void startNext();

private:
    QList<MarkerAnimation*> animations;
    QList<int> pauses;
    int currentAni;
};

// --------------------------------------------

} // namespace minigis

// --------------------------------------------


#endif // MAPHELPERCOMMON_H
