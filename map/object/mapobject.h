#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QPolygonF>

#include "core/mapmetric.h"
#include "core/mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapFrame;
class MapLayerWithObjects;
class MapDrawer;

// -------------------------------------------------------

class MapObjectPrivate;
/**
 * @brief Картографический объект
 */
class MapObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString uid READ uid WRITE setUid NOTIFY uidChanged)
    Q_PROPERTY(QString classCode READ classCode WRITE setClassCode NOTIFY classCodeChanged)

    Q_PROPERTY(minigis::Metric metric READ metric WRITE setMetric NOTIFY metricChanged)
    Q_PROPERTY(minigis::Metric* metricPtr READ metricPtr WRITE setMetricPtr NOTIFY metricChanged)
    Q_PROPERTY(bool useMetricNotify READ useMetricNotify WRITE setUseMetricNotify NOTIFY useMetricNotifyChanged)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QStringList texts READ texts WRITE setTexts NOTIFY textsChanged)

    Q_PROPERTY(minigis::MapLayerWithObjects* layer READ layer WRITE setLayer NOTIFY layerChanged)
    Q_PROPERTY(QVariantMap semantic READ semantic WRITE setSemantic NOTIFY semanticsChanged)

    Q_PROPERTY(QPolygonF aniPos READ aniPos WRITE setAniPos NOTIFY aniNotify)

public:
    explicit MapObject(QObject *parent = 0);
    ~MapObject();

    enum Highlighting {
        HighligtedLow,
        HighligtedNormal,
        HighligtedHigh
    };
    
public:

    static QHash<MapObject *, QString> &__data();

    Q_INVOKABLE QString const &uid() const;
    Q_INVOKABLE QString const &classCode() const;

    Q_INVOKABLE Metric metric() const;
    Q_INVOKABLE Metric &metric();
    Q_INVOKABLE Metric *metricPtr();

    bool useMetricNotify() const;

    Q_INVOKABLE int textCount() const;
    Q_INVOKABLE virtual QStringList texts() const;
    Q_INVOKABLE virtual QString text(int index = 0) const;

    Q_INVOKABLE virtual QStringList joinText() const;
    Q_INVOKABLE virtual void splitText(QString const &txt);

    Q_INVOKABLE bool hasAttribute(int attr) const;
    Q_INVOKABLE Attributes hasAttributes() const;
    Q_INVOKABLE QVariant attribute(int key, QVariant const &defaultValue = QVariant()) const;
    Q_INVOKABLE QVariant &attribute(int key);
    Q_INVOKABLE QVariant attributeDraw(int key, QVariant const &defaultValue = QVariant()) const;
    Q_INVOKABLE QHash<int, QVariant> attributes() const;

    Q_INVOKABLE QVariantMap const semantic() const;
    Q_INVOKABLE QVariant const semantic(int key) const;
    Q_INVOKABLE QVariant &semantic(int key);
    Q_INVOKABLE QVariant const semantic(QString const &key) const;
    Q_INVOKABLE QVariant &semantic(QString const &key);

    Q_INVOKABLE MapLayerWithObjects *layer() const;
    MapDrawer *drawer() const;

    Q_INVOKABLE bool selected() const;
    Q_INVOKABLE Highlighting highlighted() const;

    Q_INVOKABLE void setParentObject(MapObject *obj);
    Q_INVOKABLE void appendChild(MapObject *obj);
    Q_INVOKABLE void appendChild(QList<MapObject *> obj);
    Q_INVOKABLE QList<MapObject *> childrenObjects() const;
    Q_INVOKABLE MapObject *parentObject() const;

Q_SIGNALS:
    void uidChanged(QString uid, QString oldUid = QString());
    void classCodeChanged(QString cls);

    void metricChanged();
    void useMetricNotifyChanged(bool isOn);

    void semanticChanged(QString key, QVariant sem);
    void semanticsChanged(QVariantMap);

    void textChanged(QString txt, int index);
    void textsChanged(QStringList);

    void layerChanged(MapLayerWithObjects *layer);

    void attributeChanged(int key, QVariant attr);

    void aniNotify(MapObject *);

public Q_SLOTS:
    Q_INVOKABLE void setUid(QString const &uid);
    Q_INVOKABLE void setClassCode(QString const &cls);

    Q_INVOKABLE void setMetric(Metric const &arg, int ms = 0);
    Q_INVOKABLE void setMetricPtr(const Metric *metr);
    void setMetric(int index, Metric const &arg, int ms = 0);

    Q_INVOKABLE void setUseMetricNotify(bool isOn);

    Q_INVOKABLE void setTextCount(int count);
    Q_INVOKABLE void setTexts(QStringList const &txt);
    Q_INVOKABLE void setText(QString const &txt, int index = 0);

    Q_INVOKABLE void setAttribute(int key, QVariant const &attr);
    Q_INVOKABLE void setAttributes(QHash<int, QVariant> const &attr);
    Q_INVOKABLE void clearAttribute(int key);
    Q_INVOKABLE void clearAttributes();

    Q_INVOKABLE void setSemantic(QVariantMap const &sem);
    Q_INVOKABLE void setSemantic(int key, QVariant const &sem);
    Q_INVOKABLE void setSemantic(QString const &key, QVariant const &sem);
    Q_INVOKABLE void setSemantic(QHash<int, QVariant> const &sem);
    Q_INVOKABLE void clearSemantic(int key);
    Q_INVOKABLE void clearSemantic(QString const &key);
    Q_INVOKABLE void clearSemantics();

    Q_INVOKABLE void setLayer(MapLayerWithObjects *layer);

    Q_INVOKABLE void setSelected(bool flag);
    Q_INVOKABLE void setHighlighted(Highlighting flag);

    Q_INVOKABLE void raise();
    Q_INVOKABLE void lower();
    Q_INVOKABLE void stackUnder(MapObject *parentObject);

protected Q_SLOTS:
    void stopAni();
    void setAniPos(QPolygonF poly);

public:
    QPolygonF aniPos() const;

protected:
    explicit MapObject(MapObjectPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(MapObject)
    Q_DISABLE_COPY(MapObject)

    MapObjectPrivate *d_ptr;
};

typedef QPointer<MapObject> MapObjectPtr;
typedef QList<MapObjectPtr> MapObjectList;

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

Q_DECLARE_METATYPE(minigis::MapObject*)
Q_DECLARE_METATYPE(minigis::MapObjectPtr)
Q_DECLARE_METATYPE(minigis::MapObjectList)

// -------------------------------------------------------

#endif // MAPOBJECT_H
