
#include <QDebug>

#include <QHash>
#include <QPointer>
#include <QUuid>
#include <QPolygonF>
#include <QPropertyAnimation>

#include "frame/mapframe.h"
#include "drawer/mapdrawer.h"
#include "layers/maplayer.h"
#include "layers/maplayerobjects.h"
#include "core/mapmath.h"

#include "object/mapobject.h"

// -------------------------------------------------------

Q_DECLARE_METATYPE(QPolygonF)

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapObjectPrivate
{
public:
    MapObjectPrivate(MapObject *obj);
    ~MapObjectPrivate();

    //! графический объект
    MapObject *q_ptr;
    //! Ид объекта
    QString uid;
    //! Классификатор
    QString classCode;
    //! тип объекта
    int type;
    //! слой объекта
    QPointer<MapLayerWithObjects> layer;
    //! художник объекта
    QPointer<MapDrawer> drawer;
    //! метрика объекта
    Metric metric;
    //! необходимо слать сигнал изменения метрики
    bool useMetricNotify;
    //! тексты объекта
    QStringList text;
    //! атрибуты объекта
    QHash<int, QVariant> attribute;
    //! семантика объекта (10 - QImage, 20 - QWidget)
    QVariantMap semantic;
    //! выделен
    bool selected;
    //! подсвечен
    MapObject::Highlighting highlighted;
    //! родитель объекта
    MapObject *parentObject;
    //! дети объекта
    QList<MapObject*> children;

    //! аниматор
    QPropertyAnimation *ani;
    bool isAnimationRunning;
};

// -------------------------------------------------------

MapObjectPrivate::MapObjectPrivate(MapObject *obj) :
    q_ptr(obj),
    layer(NULL),
    drawer(NULL),
    useMetricNotify(false),
    selected(false),
    highlighted(MapObject::HighligtedNormal),
    parentObject(NULL),
    ani(NULL),
    isAnimationRunning(false)
{
}

// -------------------------------------------------------

MapObjectPrivate::~MapObjectPrivate()
{
}

// =======================================================

QPolygonF toEqualPoly(const QPolygonF &from, const QPolygonF &to)
{
//    if (from.isEmpty() || to.size() == from.size())
//        return from;
//    if (from.size() > to.size())
//        return toEqualPoly(to, from);

    qreal len;
    QVector<qreal> paramA = lineParams(from, len);
    QVector<qreal> paramB = lineParams(to, len);

    // поиск совпадющих точек
    QList<int> temp; // индексы точек из "to" совпадающих с точками из "from"
    temp.append(0);
    int i = 0; // текущий индекс точки из "to"
    int k = 0; // последней добавленный индекс точки - 1
    qreal t = 0.0;
    while (!paramA.isEmpty()) {
        t = paramA.takeFirst();
        qreal dt_prev = 0.0;      // предыдущая delta
        qreal dt = paramB[i] - t; // текущая delta
        bool endflag = false;
        while (dt < 0.0 && !qFuzzyIsNull(dt)) {
            ++i;
            dt_prev = dt;
            dt = paramB[i] - t;

            // если размеры полигонов совпадают и не последний индекс, то завершаем цикл
            if (paramB.size() - (i + 1) == paramA.size() && paramB.size() != i + 1) {
                endflag = true;
                break;
            }
        }

        // при завершении цикла, добавляем все индексы подряд
        if (endflag) {
            temp.append(i + 1);
            while (!paramA.isEmpty()) {
                ++i;
                paramA.removeFirst();
                temp.append(i + 1);
            }
            break;
        }

        int n = (qAbs(dt) < qAbs(dt_prev) || qFuzzyCompare(dt, dt_prev)) ? i : i - 1; // берем индекс ближайшей точки
        // если текущий индекс уже добавлен, добавляем следующий
        if (n <= k)
            n = k + 1;
        temp.append(n + 1); // paramPoly.size = poly.size - 1
        k = n;              // запоминаем последний добавленный индекс - 1
    }

    // расчет недостающих точек
    QPolygonF anew;
    anew.append(from.first());
    for (int i = 1; i < temp.size(); ++i) {
        if (temp[i] - temp[i-1] > 1) {
            QList<qreal> aTemp;
            qreal aTempLength = 0.;
            for (int j = temp[i-1]; j < temp[i]; ++j) {
                aTempLength += lengthR2(to[j+1] - to[j]);
                aTemp.append(aTempLength);
            }

            QLineF line(from[i-1], from[i]);

            aTemp.removeLast();
            for (int j = 0; j < aTemp.size(); ++j) {
                aTemp[j] /= aTempLength;
                anew.append(line.pointAt(aTemp[j]));
            }
        }
        anew.append(from[i]);
    }
    return anew;
}

QPolygonF movePoly(const QPolygonF &from, const QPolygonF &to, qreal t)
{
    if (from.size() != to.size())
        return from;

    QPolygonF exit;
    QEasingCurve ea(QEasingCurve::OutSine);
//    for (int i = 0; i < from.size(); ++i)
//        exit.append(from[i] + ea.valueForProgress(t) * (to[i] - from[i]));
    for (int i = 0; i < from.size(); ++i) {
        qreal dt = ea.valueForProgress(from.size() * t / (i + 1));
        if (dt > 1. || qFuzzyIsNull(dt - 1.))
            exit.append(to[i]);
        else
            exit.append(from[i] + dt * (to[i] - from[i]));
    }
    return exit;
}

QVariant animatePoly(const QPolygonF &from, const QPolygonF &to, qreal progress)
{
    QPolygonF res;

    if (qFuzzyIsNull(progress))
        res = from;
    else if (qFuzzyCompare(progress, 1))
        res = to;
    else if (from.size() == to.size())
        res = movePoly(from, to, progress);
    else if (from.size() > to.size())
        res = movePoly(from, toEqualPoly(to, from), progress);
    else
        res = movePoly(toEqualPoly(from, to), to, progress);

    return QVariant::fromValue(res);
}

// =======================================================

MapObject::MapObject(QObject *parent)
    : QObject(parent), d_ptr(new MapObjectPrivate(this))
{
    qRegisterAnimationInterpolator<QPolygonF>(animatePoly);
    setObjectName("MapObject");
    d_ptr->uid = QUuid::createUuid().toString();

    __data()[this] = d_ptr->uid;
}

// -------------------------------------------------------

MapObject::MapObject(MapObjectPrivate &dd, QObject *parent)
    : QObject(parent), d_ptr(&dd)
{
    setObjectName("MapObject");

    __data()[this] = d_ptr->uid;
}

// -------------------------------------------------------

MapObject::~MapObject()
{
    if (!__data().contains(this))
        qWarning() << "Double delete";
    __data().remove(this);

    if (!d_ptr)
        return;

    if (d_ptr->parentObject)
        d_ptr->parentObject->d_ptr->children.removeOne(this);
    foreach (MapObject *o, d_ptr->children)
        o->d_ptr->parentObject = NULL;
    if (d_ptr->layer)
        d_ptr->layer->remObject(this);

    MapObjectPrivate *tmp = d_ptr;
    d_ptr = 0;
    delete tmp;
}

QHash<MapObject *, QString> &MapObject::__data()
{
    static QHash<MapObject *, QString> _;
    return _;
}

// -------------------------------------------------------

const QString &MapObject::uid() const
{
    Q_D(const MapObject);
    return d->uid;
}

// -------------------------------------------------------

const QString &MapObject::classCode() const
{
    Q_D(const MapObject);
    return d->classCode;
}

// -------------------------------------------------------

Metric MapObject::metric() const
{
    Q_D(const MapObject);
    return d->metric;
}

// -------------------------------------------------------

Metric &MapObject::metric()
{
    Q_D(MapObject);
    return d->metric;
}

// -------------------------------------------------------

Metric *MapObject::metricPtr()
{
    Q_D(MapObject);
    return &d->metric;
}

// -------------------------------------------------------

bool MapObject::useMetricNotify() const
{
    Q_D(const MapObject);
    return d->useMetricNotify;
}

// -------------------------------------------------------

int MapObject::textCount() const
{
    Q_D(const MapObject);
    return d->text.size();
}

// -------------------------------------------------------

QStringList MapObject::texts() const
{
    Q_D(const MapObject);
    return d->text;
}

// -------------------------------------------------------

QString MapObject::text(int index) const
{
    Q_D(const MapObject);
    if (index < 0 || index >= d->text.size())
        return QString();
    return d->text.at(index);
}

// -------------------------------------------------------

QStringList MapObject::joinText() const
{
    Q_D(const MapObject);
    return d->text;
}

// -------------------------------------------------------

void MapObject::splitText(const QString &txt)
{
    Q_UNUSED(txt);
}

// -------------------------------------------------------

bool MapObject::hasAttribute(int attr) const
{
    Q_D(const MapObject);
    return d->drawer ? d->drawer->attributes(d->classCode).contains(attr) : false;
}

// -------------------------------------------------------

Attributes MapObject::hasAttributes() const
{
    Q_D(const MapObject);
    return d->drawer ? d->drawer->attributes(d->classCode) : Attributes();
}

// -------------------------------------------------------

QVariant MapObject::attribute(int key, QVariant const &defaultValue) const
{
    Q_D(const MapObject);
#warning Костыль! По ключу данные есть, но сами по себе они невалидные. Надо искать, где они создаются
    QVariant v = d->attribute.value(key, defaultValue);
    return v.isNull() ? defaultValue : v;
}

// -------------------------------------------------------

QVariant &MapObject::attribute(int key)
{
    Q_D(MapObject);
    return d->attribute[key];
}

// -------------------------------------------------------

QVariant MapObject::attributeDraw(int key, QVariant const &defaultValue) const
{
    Q_D(const MapObject);
    QVariant v = d->attribute.value(key);
    if (v.isNull() && d->drawer)
        v = d->drawer->attribute(d->classCode, key);
    return v.isNull() ? defaultValue : v;
}

// -------------------------------------------------------

QHash<int, QVariant> MapObject::attributes() const
{
    Q_D(const MapObject);
    return d->attribute;
}

// -------------------------------------------------------

QVariantMap const MapObject::semantic() const
{
    Q_D(const MapObject);
    QVariantMap res;
    for (QMapIterator<QVariantMap::key_type, QVariantMap::mapped_type> it(d->semantic); it.hasNext(); ) {
        it.next();
        res[it.key()] = it.value();
    }
    return res;
}

// -------------------------------------------------------

QVariant const MapObject::semantic(int key) const
{
    Q_D(const MapObject);
    return d->semantic.value(QString::number(key));
}

// -------------------------------------------------------

QVariant &MapObject::semantic(int key)
{
    Q_D(MapObject);
    return d->semantic[QString::number(key)];
}

// -------------------------------------------------------

QVariant const MapObject::semantic(QString const &key) const
{
    Q_D(const MapObject);
    return d->semantic.value(key);
}

// -------------------------------------------------------

QVariant &MapObject::semantic(QString const &key)
{
    Q_D(MapObject);
    return d->semantic[key];
}

// -------------------------------------------------------

MapLayerWithObjects *MapObject::layer() const
{
    Q_D(const MapObject);
    return d->layer;
}

// -------------------------------------------------------

MapDrawer *MapObject::drawer() const
{
    Q_D(const MapObject);
    return d->drawer;
}

// -------------------------------------------------------

bool MapObject::selected() const
{
    return d_ptr->selected;
}

// -------------------------------------------------------

MapObject::Highlighting MapObject::highlighted() const
{
    return d_ptr->highlighted;
}

// -------------------------------------------------------

void MapObject::setParentObject(MapObject *obj)
{
    Q_D(MapObject);
    if (d->parentObject == obj)
        return;
    if (d->parentObject)
        d->parentObject->d_ptr->children.removeOne(this);
    d->parentObject = obj;
    if (!obj)
        return;

    obj->d_ptr->children.append(this);
    setLayer(obj->layer());
}

// -------------------------------------------------------

MapObject *MapObject::parentObject() const
{
    return d_ptr->parentObject;
}

// -------------------------------------------------------

void MapObject::appendChild(MapObject *obj)
{
    Q_D(MapObject);
    if (!obj)
        return;
    else if (obj->d_ptr->parentObject == this)
        return;
    else if (obj->d_ptr->parentObject)
        obj->d_ptr->parentObject->d_ptr->children.removeOne(obj);

    d->children.append(obj);
    obj->d_ptr->parentObject = this;
    obj->setLayer(layer());
}

// -------------------------------------------------------

void MapObject::appendChild(QList<MapObject *> obj)
{
    foreach (MapObject *o, obj)
        appendChild(o);
}

// -------------------------------------------------------

QList<MapObject *> MapObject::childrenObjects() const
{
    return d_ptr->children;
}

// -------------------------------------------------------

void MapObject::setMetric(Metric const &arg, int ms)
{
    Q_D(MapObject);
    if (d->metric == arg)
        return;

    if (ms > 0 && arg.size() == 1 && d->metric.size()) {
        if (!d->ani) {
            d->ani = new QPropertyAnimation(this, "aniPos", this);
            connect(d->ani, SIGNAL(finished()), SLOT(stopAni()));
        }

        d->ani->setDuration(ms);
        d->ani->setStartValue(QVariant::fromValue(d->metric.toMercator(0)));
        d->ani->setEndValue(QVariant::fromValue(arg.toMercator(0)));

        if (d->isAnimationRunning)
            d->ani->stop();
        d->isAnimationRunning = true;
        d->ani->start();
    }
    else
        d->metric = arg;

    if (d->useMetricNotify)
        emit metricChanged();
}

// -------------------------------------------------------

void MapObject::setMetricPtr(const Metric *metr)
{
    return setMetric(*metr);
}

// -------------------------------------------------------

void MapObject::setMetric(int index, Metric const &arg, int ms)
{
    Q_D(MapObject);
    if (ms > 0 && arg.size() == 1 && d->metric.size()) {
        if (!d->ani) {
            d->ani = new QPropertyAnimation(this, "aniPos", this);
            connect(d->ani, SIGNAL(finished()), SLOT(stopAni()));
        }
        Metric m = d->metric;
        m.setMetric(arg, index);

        d->ani->setDuration(ms);
        d->ani->setStartValue(QVariant::fromValue(d->metric));
        d->ani->setEndValue(QVariant::fromValue(m));

        if (d->isAnimationRunning)
            d->ani->stop();
        d->isAnimationRunning = true;
        d->ani->start();
    }
    else
        d->metric.setMetric(arg, index);

    if (d->useMetricNotify)
        emit metricChanged();
}

// -------------------------------------------------------

void MapObject::setUseMetricNotify(bool isOn)
{
    Q_D(MapObject);
    if (d->useMetricNotify == isOn)
        return;
    d->useMetricNotify = isOn;
    emit useMetricNotifyChanged(isOn);
}

// -------------------------------------------------------

void MapObject::setTextCount(int count)
{
    Q_D(MapObject);
    if (d->text.size() == count)
        return;
    d->text.reserve(count);
    while (d->text.size() < count)
        d->text.append(QString());
    while (d->text.size() > count)
        d->text.removeLast();
}

// -------------------------------------------------------

void MapObject::setTexts(QStringList const &txt)
{
    Q_D(MapObject);
    if (d->text.size() < txt.size())
        setTextCount(txt.size());
    for (int i = 0; i < txt.size(); ++i) {
        if (d->text[i] == txt[i])
            continue;
        d->text[i] = txt[i];
        emit textChanged(txt[i], i);
    }
    emit textsChanged(txt);
}

// -------------------------------------------------------

void MapObject::setText(const QString &txt, int index)
{
    Q_D(MapObject);
    if (index == 0) {
        if (d->text.size() == 0)
            d->text.append(txt);
        else
            d->text[index] = txt;
        emit textChanged(txt, index);
        return;
    }
    if (d->text.size() - 1 < index || index < 0)
        return;
    if (d->text.at(index) == txt)
        return;

    d->text[index] = txt;
    emit textChanged(txt, index);
}

// -------------------------------------------------------

void MapObject::setAttribute(int key, QVariant const &attr)
{
    Q_D(MapObject);
    if (d->attribute.value(key) == attr)
        return;
    d->attribute[key] = attr;
    emit attributeChanged(key, attr);
}

// -------------------------------------------------------

void MapObject::setAttributes(QHash<int, QVariant> const &attr)
{
    Q_D(MapObject);
    for (QHashIterator<int, QVariant> it(attr); it.hasNext(); ) {
        it.next();
        if (d->attribute.value(it.key()) == it.value())
            continue;
        d->attribute[it.key()] = it.value();
        emit attributeChanged(it.key(), it.value());
    }
}

// -------------------------------------------------------

void MapObject::clearAttribute(int key)
{
    Q_D(MapObject);
    if (d->attribute.value(key).isNull())
        return;
    QVariant r = d->attribute.take(key);
    if (!r.isNull())
        emit attributeChanged(key, QVariant());
}

// -------------------------------------------------------

void MapObject::clearAttributes()
{
    Q_D(MapObject);
    QHash<int, QVariant> attr;
    attr.swap(d->attribute);
    for (QHashIterator<int, QVariant> it(attr); it.hasNext(); ) {
        it.next();
        if (!it.value().isNull())
            emit attributeChanged(it.key(), QVariant());
    }
}

// -------------------------------------------------------

void MapObject::setSemantic(QVariantMap const &sem)
{
    for (QMapIterator<QVariantMap::key_type, QVariantMap::mapped_type> it(sem); it.hasNext(); ) {
        it.next();
        setSemantic(it.key(), it.value());
    }
    emit semanticsChanged(sem);
}

// -------------------------------------------------------

void MapObject::setSemantic(int key, QVariant const &sem)
{
    setSemantic(QString::number(key), sem);
}

// -------------------------------------------------------

void MapObject::setSemantic(QString const &key, QVariant const &sem)
{
    Q_D(MapObject);
    if (d->semantic.value(key) == sem)
        return;

#if 0
    if (key == WidgetSemantic) {
        QWidget *wid = QWidget::find(semantic(key).value<WId>());
        if (d->drawer && d->drawer->needLayerVisibilityChangedCatch() && d->layer && wid)
            disconnect(d->layer.data(), SIGNAL(visibleChanged(bool)), wid, SLOT(setVisible(bool)));
    }
#endif

    d->semantic[key] = sem;

#if 0
    if (key == WidgetSemantic) {
        QWidget *wid = QWidget::find(sem.value<WId>());
        if (d->drawer && d->drawer->needLayerVisibilityChangedCatch() && d->layer && wid)
            connect(d->layer.data(), SIGNAL(visibleChanged(bool)), wid, SLOT(setVisible(bool)));
    }
#endif

    emit semanticChanged(key, sem);
}

// -------------------------------------------------------

void MapObject::setSemantic(QHash<int, QVariant> const &sem)
{
    for (QHashIterator<int, QVariant> it(sem); it.hasNext(); ) {
        it.next();
        setSemantic(it.key(), it.value());
    }
}

// -------------------------------------------------------

void MapObject::clearSemantic(int key)
{
    clearSemantic(QString::number(key));
}

// -------------------------------------------------------

void MapObject::clearSemantic(QString const &key)
{
    Q_D(MapObject);
    if (d->semantic.value(key).isNull())
        return;
    QVariant r = d->semantic.take(key);
    if (!r.isNull()) {
#if 0
        if (key == WidgetSemantic) {
            QWidget *wid = QWidget::find(r.value<WId>());
            if (d->drawer && d->drawer->needLayerVisibilityChangedCatch() && d->layer && wid)
                disconnect(d->layer.data(), SIGNAL(visibleChanged(bool)), wid, SLOT(setVisible(bool)));
        }
#endif
        emit semanticChanged(key, QVariant());
    }
}

// -------------------------------------------------------

void MapObject::clearSemantics()
{
    Q_D(MapObject);
    QVariantMap sem;
    sem.swap(d->semantic);
    for (QMapIterator<QVariantMap::key_type, QVariantMap::mapped_type> it(sem); it.hasNext(); ) {
        it.next();
        if (!it.value().isNull()) {
#if 0
            if (it.key() == WidgetSemantic) {
                QWidget *wid = QWidget::find(it.value().value<WId>());
                if (d->drawer && d->drawer->needLayerVisibilityChangedCatch() && d->layer && wid)
                    disconnect(d->layer.data(), SIGNAL(visibleChanged(bool)), wid, SLOT(setVisible(bool)));
            }
#endif

            emit semanticChanged(it.key(), QVariant());
        }
    }
}

// -------------------------------------------------------

void MapObject::setLayer(MapLayerWithObjects *layer)
{
    Q_D(MapObject);
    if (d->layer == layer)
        return;

    if (!layer && d->layer)
        d->layer->remObject(this);
    d->layer = layer;

    if (!d->drawer && layer && layer->map() && !d->classCode.isEmpty()) {
//        d->drawer = layer->map()->drawer(d->classCode);

        if (!d->drawer)
            qWarning() << "Drawer classcode not exists" << d->classCode;
    }

    if (layer)
        layer->addObject(this);

    emit layerChanged(layer);
}

// -------------------------------------------------------

void MapObject::setSelected(bool flag)
{
    d_ptr->selected = flag;
}

// -------------------------------------------------------

void MapObject::setHighlighted(Highlighting flag)
{
    d_ptr->highlighted = flag;
}

// -------------------------------------------------------

void MapObject::raise()
{
    Q_D(MapObject);
    MapLayerObjects* lobj = qobject_cast<MapLayerObjects* >(d->layer);
    if (lobj)
        lobj->raiseObject(this);
}

// -------------------------------------------------------

void MapObject::lower()
{
    Q_D(MapObject);
    MapLayerObjects* lobj = qobject_cast<MapLayerObjects* >(d->layer);
    if (lobj)
        lobj->lowerObject(this);
}

// -------------------------------------------------------

void MapObject::stackUnder(MapObject *parentObject)
{
    Q_D(MapObject);
    MapLayerObjects* lobj = qobject_cast<MapLayerObjects* >(d->layer);
    if (lobj)
        lobj->stackUnderObject(this, parentObject);
}

// -------------------------------------------------------

void MapObject::stopAni()
{
    Q_D(MapObject);
    d->isAnimationRunning = false;
    if (!d->ani)
        return;
    d->ani->stop();
}

// -------------------------------------------------------

void MapObject::setAniPos(QPolygonF poly)
{
    Q_D(MapObject);
    if (!drawer())
        return;
    QVector<QPolygonF> tmp = drawer()->correctMetric(this, poly, 0);
    d->metric.setMetric(tmp, Mercator);
    emit aniNotify(this);
}

// -------------------------------------------------------

QPolygonF MapObject::aniPos() const
{
    Q_D(const MapObject);
    return d->metric.size() == 0 ? QPolygonF() : d->metric.toMercator(0);
}

// -------------------------------------------------------

void MapObject::setUid(QString const &uid)
{
    Q_D(MapObject);
    if (d->uid == uid)
        return;
    __data()[this] = uid;
    QString old = d->uid;
    d->uid = uid;
    emit uidChanged(uid, old);
}

// -------------------------------------------------------

void MapObject::setClassCode(QString const &cls)
{
    Q_D(MapObject);
    if (d->classCode == cls)
        return;
    d->classCode = cls;
//    if (d->layer && d->layer->map())
//        d->drawer = d->layer->map()->drawer(cls);
    if (!d->drawer && d->layer)
        qWarning() << "Drawer classcode not exists" << cls;
    emit classCodeChanged(cls);
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
