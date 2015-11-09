#include "interact/mapuserinteraction.h"
#include "interact/maphelper.h"
#include "coord/mapcamera.h"
#include "layers/maplayer.h"
#include "layers/maplayertile.h"
#include "core/maptemplates.h"
#include "frame/mapsettings.h"
#include "controller/mapcontroller.h"

#include "mapframe.h"
#include "mapframe_p.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

MapFramePrivate::MapFramePrivate(QObject *parent)
    : QObject(parent), helper(NULL), camera(new MapCamera),
      ui(new MapUserInteraction), settings(new MapSettings),
      controller(new MapController)
{
    setObjectName("MapFramePrivate");
    connectSettingsToData();
}

// -------------------------------------------------------

MapFramePrivate::~MapFramePrivate()
{
    delete helper;
    helper = NULL;
}

// -------------------------------------------------------

void MapFramePrivate::connectSettingsToData()
{
    connect(settings.data(), SIGNAL(saturationChanged()), SLOT(hsvChanged()));
    connect(settings.data(), SIGNAL(valueChanged()), SLOT(hsvChanged()));
    connect(settings.data(), SIGNAL(proxyHostChanged()), SLOT(proxyChanged()));
    connect(settings.data(), SIGNAL(proxyPortChanged()), SLOT(proxyChanged()));
    connect(settings.data(), SIGNAL(proxyChanged()), SLOT(proxyEnable()));
    connect(settings.data(), SIGNAL(httpChanged()), SLOT(httpEnable()));
    connect(settings.data(), SIGNAL(currentSearchTypeChanged()), SLOT(setSearcher()));
    connect(settings.data(), SIGNAL(nightModeChanged()), SLOT(clearTileCache()));
    connect(settings.data(), SIGNAL(mapOptionsChanged()), SLOT(mapOptionsChanged()));
}

// -------------------------------------------------------

void MapFramePrivate::layerSort()
{
    qSort(layers.begin(), layers.end(), compare<std::less>(&MapLayer::zOrder));
}

// -------------------------------------------------------

void MapFramePrivate::layerDeleted(QObject *object)
{
    MapLayer *layer = static_cast<MapLayer *>(object);
    if (!layer)
        return;
    disconnect(layer, SIGNAL(destroyed(QObject*)), this, SLOT(layerDeleted(QObject*)));
    disconnect(layer, SIGNAL(zOrderChanged(int)), this, SLOT(layerZOrderChanged(int)));

    layers.removeOne(layer);
}

// -------------------------------------------------------

void MapFramePrivate::layerZOrderChanged(int)
{
    layerSort();
}

// -------------------------------------------------------

void MapFramePrivate::hsvChanged()
{
    QVariantMap opt;
    opt.insert("saturation", settings->saturation());
    opt.insert("value", settings->value());
    layerTile->setGraphOptions(opt);
}

// -------------------------------------------------------

void MapFramePrivate::proxyChanged()
{
#if 0
    foreach (MapSearchEngine *s, searches) {
        MapSearchEngineHttp *httpS = dynamic_cast<MapSearchEngineHttp*>(s);
        if (httpS)
            httpS->setProxy(settings->proxyHost(), settings->proxyPort());
    }
#endif

    if (layerTile.isNull())
        return;

    QList<MapTileLoader*> list = layerTile->loaders();
    foreach (MapTileLoader *l, list) {
        MapTileLoaderHttp *httpL = dynamic_cast<MapTileLoaderHttp*>(l);
        if (httpL)
            httpL->setProxy(settings->proxyHost(), settings->proxyPort());
    }
}

// -------------------------------------------------------

void MapFramePrivate::proxyEnable()
{
#if 0
    foreach (MapSearchEngine *s, searches) {
        MapSearchEngineHttp *httpS = dynamic_cast<MapSearchEngineHttp*>(s);
        if (httpS)
            httpS->setProxyEnabled(settings->isProxyEnabled());
    }
#endif

    if (layerTile.isNull())
        return;

    QList<MapTileLoader*> list = layerTile->loaders();
    foreach (MapTileLoader *l, list) {
        MapTileLoaderHttp *httpL = dynamic_cast<MapTileLoaderHttp*>(l);
        if (httpL)
            httpL->setProxyEnabled(settings->isProxyEnabled());
    }
}

// -------------------------------------------------------

void MapFramePrivate::httpEnable()
{
#if 0
    foreach (MapSearchEngine *s, searches) {
        MapSearchEngineHttp *httpS = dynamic_cast<MapSearchEngineHttp*>(s);
        if (httpS)
            httpS->setEnabled(settings->isHttpEnabled());
    }
#endif

    if (layerTile.isNull())
        return;

    QList<MapTileLoader*> list = layerTile->loaders();
    foreach (MapTileLoader *l, list) {
        MapTileLoaderHttp *httpL = dynamic_cast<MapTileLoaderHttp*>(l);
        if (httpL)
            httpL->setEnabled(settings->isHttpEnabled());
    }
}

// -------------------------------------------------------

void MapFramePrivate::setSearcher()
{
#if 0
    currentSearch = NULL;
    int t = settings->currentSearchType();
    foreach (MapSearchEngine *engine, searches) {
        if (engine && engine->type() == t) {
            currentSearch = engine;
            break;
        }
    }
#endif
}

// -------------------------------------------------------

void MapFramePrivate::clearTileCache()
{
    layerTile->clearCache();
}

// -------------------------------------------------------

void MapFramePrivate::mapOptionsChanged()
{
    q_ptr->update();
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
