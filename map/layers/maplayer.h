#ifndef MAPLAYER_H
#define MAPLAYER_H

#include <QObject>
#include <QPainter>

#include "core/mapdefs.h"

namespace minigis {

// -------------------------------------------------------

class MapFrame;
class MapObject;
class MapCamera;

// -------------------------------------------------------

class MapLayerPrivate;
class MapLayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int zOrder READ zOrder WRITE setZOrder NOTIFY zOrderChanged)
    Q_PROPERTY(QString uid READ uid WRITE setUid NOTIFY uidChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(minigis::MapCamera* camera READ camera WRITE setCamera)

public:
    explicit MapLayer(QObject *parent = 0);
    virtual ~MapLayer();
    
    MapFrame *map() const;
    int zOrder() const;
    QString name() const;
    QString uid() const;
    bool visible() const;
    MapCamera *camera() const;

    virtual void clearLayer() { }

Q_SIGNALS:
    void mapChanged(MapFrame *);      // mapChanged изменился владелец слоя
    void zOrderChanged(int);          // zOrderChanged изменился порядковый номер
    void nameChanged(QString);        // nameChanged изменено имя
    void uidChanged(QString);         // uidChanged изменён ид
    void visibleChanged(bool);        // visibleChanged изменена видимость
    void imageReady(QRect = QRect()); // imageReady картинка готова

public Q_SLOTS:
    // setMap установить владельца слоя
    void setMap(MapFrame *map);
    // setZOrder Задать порядковый номер в стопке слоёв
    void setZOrder(int z);
    // setName Задать логическое имя слоя
    void setName(QString name);
    // setUid Задать логическое имя слоя
    void setUid(QString uid);
    // setVisible Задать видимость слоя
    void setVisible(bool visible);
    // setCamera установить камеру
    void setCamera(MapCamera *camera);

    void show();
    void hide();

    // update перерисока слоя
    void update(QPainter *painter, const QRect &rgn = QRect(), MapOptions options = optNone);

protected:
    // MapLayer конструктор для использования в унаследованных классах
    explicit MapLayer(MapLayerPrivate &dd, QObject *parent = 0);

protected:
    Q_DECLARE_PRIVATE(MapLayer)
    Q_DISABLE_COPY(MapLayer)

    MapLayerPrivate *d_ptr;
};

// -------------------------------------------------------

class MapLayerWithObjectsPrivate;
class MapLayerWithObjects: public MapLayer
{
    Q_OBJECT

public:
    MapLayerWithObjects(QObject *parent = 0);
    virtual ~MapLayerWithObjects();

    virtual void addObject(MapObject *mo);
    virtual void remObject(MapObject *mo);
    virtual MapObject *findObject(QString const &uid) const;

    virtual void clearLayer();
    virtual QList<MapObject*> const objects() const;

protected:
    explicit MapLayerWithObjects(MapLayerWithObjectsPrivate &dd, QObject *parent = 0);

protected:
    Q_DECLARE_PRIVATE(MapLayerWithObjects)
    Q_DISABLE_COPY(MapLayerWithObjects)
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------


#endif // MAPLAYER_H
