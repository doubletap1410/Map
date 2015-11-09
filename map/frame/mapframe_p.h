#ifndef MAPFRAME_P_H
#define MAPFRAME_P_H

#include <QObject>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapCamera;
class MapUserInteraction;
class MapHelper;
class MapLayer;
class MapLayerTile;
class MapSettings;
class MapController;

// -------------------------------------------------------

class MapFrame;
class MapFramePrivate: public QObject
{
    Q_OBJECT
public:
    MapFramePrivate(QObject *parent = NULL);
    virtual ~MapFramePrivate();

    MapHelper *helper;
    QScopedPointer<MapCamera> camera;
    QScopedPointer<MapUserInteraction> ui;

    QList<MapLayer*> layers;
    QPointer<MapLayerTile> layerTile;

    //! настройки карты
    QScopedPointer<MapSettings> settings;

    //! QML коммуникатор
    QScopedPointer<MapController> controller;

public Q_SLOTS:
    void connectSettingsToData();

    void layerSort();                    //! сортировать слои
    void layerDeleted(QObject *object);  //! Удалён слой
    void layerZOrderChanged(int);        //! при изменении слоя по Z

private Q_SLOTS:
    void hsvChanged();
    void proxyChanged();
    void proxyEnable();
    void httpEnable();
    void setSearcher();
    void clearTileCache();
    void mapOptionsChanged();

public:
    Q_DECLARE_PUBLIC(MapFrame)
    Q_DISABLE_COPY(MapFramePrivate)

    MapFrame *q_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
#endif // MAPFRAME_P_H
