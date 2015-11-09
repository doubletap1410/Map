
#include <QResizeEvent>
#include <QPainter>
#include <QScopedPointer>
#include <QMouseEvent>
#include <QFileInfo>
#include <QBuffer>

// -------------------------------------------------------

#include <common/Runtime.h>

#include "map/interact/mapuserinteraction.h"
#include "map/interact/maphelper.h"
#include "map/coord/mapcamera.h"
#include "map/coord/mapcoords.h"
#include "map/layers/maplayer.h"
#include "map/layers/maplayerobjects.h"
#include "map/layers/maplayertile.h"
#include "map/frame/mapsettings.h"
#include "map/frame/mapframe_p.h"
#include "map/controller/mapcontroller.h"
#include "map/object/mapobject.h"

#include "mapframe.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

MapFrame::MapFrame(QQuickItem *parent)
    : QQuickPaintedItem(parent), d_ptr(new MapFramePrivate)
{
    Q_D(MapFrame);

    setObjectName("MapFrame");
    //    setAttribute(Qt::WA_NoSystemBackground);
    //    setAttribute(Qt::WA_AcceptTouchEvents, true);
    //    grabGesture(Qt::TapAndHoldGesture);
    //    grabGesture(Qt::PanGesture);
    //    grabGesture(Qt::PinchGesture);
    //    grabGesture(Qt::SwipeGesture);

    d->q_ptr = this;

    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);

    connect(this, SIGNAL(widthChanged()), SLOT(sizeSceneChange()));
    connect(this, SIGNAL(heightChanged()), SLOT(sizeSceneChange()));

    d->camera->init(this);
    d->camera->setBounds(
                QRectF(QPointF(-minigis::TileSystem::MaxX, -minigis::TileSystem::MaxY),
                       QPointF( minigis::TileSystem::MaxX,  minigis::TileSystem::MaxY))
                .normalized());

    d->helper = new MapHelperUniversal(this);
    d->helper->init(this);

    d->ui->setMap(this);
    installEventFilter(d->ui.data());

    initDrawersAndLayers();

    d->controller->init(this);

    ////////////////////////////////////////////////////////////////
    minigis::MapLayerTile *tLayer = tileLayer();
    Q_ASSERT(tLayer);

    settings()->enableProxy(false);
    settings()->enableHttp(true);
    settings()->enableNightMode(false);

//    settings()->setCurrentSearchType(minigis::MapSearchEngineYandex::Type);
    settings()->enableSearchHelper(false);

    settings()->setMapOptions(minigis::MapOptions(minigis::optDissolveGen | minigis::optSubstrateSmoothing | minigis::optAntialiasing));
    settings()->setSaturation(0);
    settings()->setValue(0);

    settings()->enableAutoNightMode(false);
    tLayer->setUploadLevel(1);
    tLayer->changeTileTypesI(61);

    MapCamera *cam = camera();
    Q_ASSERT(cam);
}

// -------------------------------------------------------

MapFrame::~MapFrame()
{

}

// -------------------------------------------------------

void MapFrame::initDrawersAndLayers()
{
//    MapShare::setSVGfileName(Runtime::getSharePath() + "rsc.svg");

    MapLayerTile *lTile = new MapLayerTile();
    lTile->setCamera(d_ptr->camera.data());
    lTile->changeTileTypesL(QList<int>());
    registerLayer(lTile);

    d_ptr->layerTile = lTile;
    lTile->changeColorizedTiles(ConvertColor::hsvColor);

    // FIXME: Сильно подозреваю что лоадеры нигде не удаляются (как и драверы). придумать механизм чистки при завершении ПО

    MapTileLoader *loader;

#define REGISTER_LOADER(T) \
    loader = new T(); \
    if (!lTile->registerLoader(loader)) \
        delete loader;

//    REGISTER_LOADER(MapTileLoaderYandexWeather);
    REGISTER_LOADER(MapTileLoaderCheckFillDataBase);

#undef REGISTER_LOADER

//#warning Runtime::getEtcPath - android res
    QList<MapTileLoaderHttp *> templLoaders = MapTileLoaderHttpTemplLoader::load(
                Runtime::getEtcPath() + "tileserver.xml"
                );

    for (QListIterator<MapTileLoaderHttp*> it(templLoaders); it.hasNext(); ) {
        MapTileLoaderHttp *l = it.next();
        if (!lTile->registerLoader(l))
            delete l;
    }
}

// -------------------------------------------------------

MapHelper *MapFrame::helper() const
{
    Q_D(const MapFrame);
    return d->helper;
}

// -------------------------------------------------------

MapCamera *MapFrame::camera()
{
    Q_D(MapFrame);
    return d->camera.data();
}

const MapCamera *MapFrame::camera() const
{
    Q_D(const MapFrame);
    return d->camera.data();
}

// -------------------------------------------------------

void MapFrame::registerLayer(MapLayer *layer)
{
    if (!layer)
        return;

    Q_D(MapFrame);

    // FIXME: ошибка при вызове layer->setMap(map) при layer->map == NULL и map != NULL
    // не достаточная проверка
    if (layer->map() == this)
//    if (d->layers.contains(layer))
        return;

    if (layers().contains(layer)) {
        qWarning() << "Warning. Layer exists";
        return;
    }

    connect(layer, SIGNAL(destroyed(QObject*)), d, SLOT(layerDeleted(QObject*)), Qt::QueuedConnection);
    connect(layer, SIGNAL(zOrderChanged(int)), d, SLOT(layerZOrderChanged(int)), Qt::QueuedConnection);
    connect(layer, SIGNAL(imageReady(QRect)), this, SLOT(updateScene(QRect)), Qt::QueuedConnection);

    layer->setMap(this);
    d->layers.append(layer);
    d->layerSort();
}

// -------------------------------------------------------

void MapFrame::unregisterLayer(MapLayer *layer)
{
    if (!layer)
        return;
    Q_D(MapFrame);

    d->layerDeleted(layer);
    layer->setMap(NULL);
}

// -------------------------------------------------------

QList<MapLayer *> MapFrame::layers() const
{
    Q_D(const MapFrame);
    return d->layers;
}

// -------------------------------------------------------

MapLayer *MapFrame::layerByName(const QString &layerName) const
{
    Q_D(const MapFrame);
    foreach (MapLayer * const l, d->layers)
        if (l->name() == layerName)
            return l;
    return NULL;
}

// -------------------------------------------------------

MapLayer *MapFrame::layerByUid(const QString &layerUid) const
{
    Q_D(const MapFrame);
    foreach (MapLayer * const l, d->layers)
        if (l->uid() == layerUid)
            return l;
    return NULL;
}

// -------------------------------------------------------

MapLayerTile *MapFrame::tileLayer() const
{
    Q_D(const MapFrame);
    return d->layerTile.data();
}

// -------------------------------------------------------

MapLayerTile &MapFrame::tileLayerRef() const
{
    Q_D(const MapFrame);
    return *d->layerTile;
}

// -------------------------------------------------------

const MapSettings *MapFrame::settings() const
{
    Q_D(const MapFrame);
    return d->settings.data();
}

// -------------------------------------------------------

MapSettings *MapFrame::settings()
{
    Q_D(MapFrame);
    return d->settings.data();
}

// -------------------------------------------------------

const MapController *MapFrame::controller() const
{
    Q_D(const MapFrame);
    return d->controller.data();
}

// -------------------------------------------------------

MapController *MapFrame::controller()
{
    Q_D(const MapFrame);
    return d->controller.data();
}

// -------------------------------------------------------

MapHelperUniversal *MapFrame::actionUniversal(QObject *recv, const char *abortSignal, const char *handlerSlot)
{
    MapHelperUniversal *mh = static_cast<MapHelperUniversal *>(d_ptr->helper);
    if (!mh)
        return NULL;
    mh->setRunData(recv, abortSignal, handlerSlot);
    if (!mh->start())
        return NULL;

    return mh;
}

// -------------------------------------------------------

const QList<MapObject *> MapFrame::selectObjects(const QRectF &rgn, const MapCamera *camera) const
{
    QList<MapObject *> list;
    foreach (MapLayer *l, layers()) {
        MapLayerObjects* lobj = qobject_cast<MapLayerObjects*>(l);
        if (lobj)
            list += lobj->selectObjects(rgn, camera);
    }
    return list;
}

// -------------------------------------------------------

const QList<MapObject *> MapFrame::selectObjects(const QPointF &pos, const MapCamera *camera) const
{
    return selectObjects(QRectF(pos, QSize()), camera);
}

// -------------------------------------------------------

void MapFrame::updateScene(QRect r)
{
    r.isEmpty() ? update() : update(r);
}

// -------------------------------------------------------

void MapFrame::sizeSceneChange()
{
    Q_D(MapFrame);
    QSize s = contentsBoundingRect().size().toSize();
    d->camera->setScreenSize(s);
    d->helper->resize(s, s);
    update();
}

// -------------------------------------------------------

void MapFrame::paint(QPainter *painter)
{
    painter->fillRect(contentsBoundingRect(), QBrush(Qt::transparent));
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

    foreach (MapLayer *l, layers())
        if (l->visible())
            l->update(painter, contentsBoundingRect().toRect());
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------




