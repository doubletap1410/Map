#ifndef MAPFRAME_H
#define MAPFRAME_H

#include <QQuickPaintedItem>

#include <QPointer>
#include <QPolygonF>

#include <QEvent>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapHelper;
class MapCamera;
class MapHelperUniversal;
class MapLayer;
class MapLayerTile;
class MapSettings;
class MapController;
class MapObject;

// -------------------------------------------------------

/**
 * @brief Компонент отображения картографической схемы
 */
class MapFramePrivate;
class MapFrame : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(minigis::MapSettings * settings READ settings CONSTANT)
    Q_PROPERTY(minigis::MapCamera * camera READ camera CONSTANT)    
    Q_PROPERTY(minigis::MapController * controller READ controller CONSTANT)

public:
    explicit MapFrame(QQuickItem *parent = 0);
    virtual ~MapFrame();

    void initDrawersAndLayers();

    MapHelper *helper() const;
    Q_INVOKABLE MapCamera *camera();
    MapCamera const *camera() const;

    Q_INVOKABLE void registerLayer(MapLayer *layer);
    Q_INVOKABLE void unregisterLayer(MapLayer *layer);

    QList<minigis::MapLayer *> layers() const;
    Q_INVOKABLE minigis::MapLayer* layerByName(QString const &layerName) const;
    Q_INVOKABLE minigis::MapLayer* layerByUid(QString const &layerUid) const;
    minigis::MapLayerTile* tileLayer() const;
    minigis::MapLayerTile& tileLayerRef() const;
    Q_INVOKABLE minigis::MapSettings const * settings() const;
    Q_INVOKABLE minigis::MapSettings * settings();

    Q_INVOKABLE minigis::MapController const * controller() const;
    Q_INVOKABLE minigis::MapController * controller();

    MapHelperUniversal *actionUniversal(QObject *recv, const char *abortSignal, const char *handlerSlot = 0);

    QList<MapObject *> const selectObjects(QRectF const &rgn, MapCamera const *camera) const;
    QList<MapObject *> const selectObjects(QPointF const &pos, MapCamera const *camera) const;

public Q_SLOTS:
    void updateScene(QRect = QRect());

protected Q_SLOTS:
    void sizeSceneChange();

protected:
    virtual void paint(QPainter *painter);

private:
    Q_DECLARE_PRIVATE(MapFrame)
    Q_DISABLE_COPY(MapFrame)

    MapFramePrivate *d_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPFRAME_H
