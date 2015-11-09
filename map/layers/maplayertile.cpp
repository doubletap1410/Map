
#include <QPainter>
#include <QScreen>
#include <QPropertyAnimation>
#include <QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimerEvent>
#include <QBuffer>

//#include <common/Runtime.h>
//#include <common/Settings.h>
#include <db/databasecontroller.h>

#include "frame/mapframe.h"
#include "core/mapdefs.h"
#include "core/mapmath.h"
#include "coord/mapcoords.h"
#include "layers/maplayertile.h"

#include "maplayertile_p.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

MapLayerTile::MapLayerTile(QObject *parent)
    : MapLayer(*new MapLayerTilePrivate, parent)
{
    setObjectName("MapLayerTile");
    d_ptr->q_ptr = this;

    Q_D(MapLayerTile);
    qRegisterMetaType<TileKey>("TileKey");
    connect(d, SIGNAL(tileIncome(TileKey,bool,bool)), SIGNAL(tileIncome(TileKey,bool,bool)));

    setName("MapLayerTile");
    setZOrder(-1);
}

MapLayerTile::MapLayerTile(MapLayerPrivate &dd, QObject *parent)
    : MapLayer(dd, parent)
{
    setObjectName("MapLayerTile");
    d_ptr->q_ptr = this;

    Q_D(MapLayerTile);
    qRegisterMetaType<TileKey>("TileKey");
    connect(d, SIGNAL(tileIncome(TileKey,bool,bool)), SIGNAL(tileIncome(TileKey,bool,bool)));

    setName("MapLayerTile");
    setZOrder(-1);
}

MapLayerTile::~MapLayerTile()
{
    delete d_ptr;
}

void MapLayerTile::clearLayer()
{
}

bool MapLayerTile::registerLoader(MapTileLoader *loader)
{
    if (loader->thread() != thread())
        loader->moveToThread(thread());
    if (thread() != QThread::currentThread()) {
        bool res = false;
        QMetaObject::invokeMethod(this, "registerLoader", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, res), Q_ARG(MapTileLoader*, loader));
        return res;
    }

    Q_D(MapLayerTile);
    QMutexLocker locker(&d->mutex);

    if (!loader)
        return false;
    if (d->loaders.contains(loader->type())) {
        qWarning() << "Loader " << loader->description() << " is already exist!";
        return false;
    }

    loader->setParent(this);
    loader->init(this);

    d->loaders.insert(loader->type(), loader);
    connect(loader, SIGNAL(imageReady(QImage,int,int,int,int,int)), this, SLOT(addNewImage(QImage,int,int,int,int,int)));
    connect(loader, SIGNAL(errorKey(int,int,int,int)), d, SLOT(loaderError(int,int,int,int)));
    connect(loader, SIGNAL(destroyed(QObject*)), SLOT(loaderDestroid(QObject*)));
    return true;    
}

void MapLayerTile::unregisterLoader(MapTileLoader *loader)
{
    Q_D(MapLayerTile);
    QMutexLocker locker(&d->mutex);

    d->loaders.take(d->loaders.key(loader));
    loader->done();
    loader->setParent(NULL);
}

MapTileLoader *MapLayerTile::loader(int type) const
{
    Q_D(const MapLayerTile);
    return d->loaders.value(type);
}

QList<MapTileLoader *> MapLayerTile::loaders() const
{
    Q_D(const MapLayerTile);
    return d->loaders.values();
}

QList<int> MapLayerTile::types() const
{
    Q_D(const MapLayerTile);
    return d->types;
}

dc::DatabaseController *MapLayerTile::dc() const
{
    Q_D(const MapLayerTile);
    return d->dc;
}

void MapLayerTile::clearTileTypes(int type)
{
    Q_D(MapLayerTile);
    foreach (Tile *t, d->tiles.values())
        t->source.remove(type);
    emit imageReady();
}

void MapLayerTile::changeColorizedTiles(ConvertColor::ColorFilterFunc f)
{
    Q_D(MapLayerTile);

    d->func = f;
    clearCache();
}

void MapLayerTile::clearCache()
{
    Q_D(MapLayerTile);
    for (QMutableHashIterator<quint64, Tile*> it(d->tiles); it.hasNext();) {
        it.next();
        Tile *t = it.value();
        if (t)
            t->clear(Tile::Colorized);
    }
    d->map->update();
}

void MapLayerTile::setGraphOptions(QVariantMap options)
{
    Q_D(MapLayerTile);

    d->graphOpt = options;
    clearCache();
}

void MapLayerTile::changeTileTypesL(QList<int> types)
{
    Q_D(MapLayerTile);
    if (d->types == types)
        return;
    d->types = types;

    // очистка кэша    
    d->dbCache.clear();
    d->askCache.clear();

    foreach (Tile *t, d->tiles.values()) {
        delete t->prevTmp;
        t->prevTmp = NULL;

        t->setSource(types);
    }

    emit imageReady();
}

void MapLayerTile::changeTileTypesI(int type)
{
    changeTileTypesL(QList<int>() << type);
}

void MapLayerTile::changeWatchType(int)
{
    Q_D(MapLayerTile);
    if (!d->types.contains(MapTileLoaderCheckFillDataBase::Type))
        return;

    foreach (Tile *t, d->tiles.values())
        delete t->source.take(MapTileLoaderCheckFillDataBase::Type);
}

void MapLayerTile::addNewImage(QImage img, int x, int y, int z, int type, int expires)
{
    Q_D(MapLayerTile);

    TileKey tmpKey(x, y, z, type);
    emit tileIncome(tmpKey, img.isNull());

    quint64 keyHash = tmpKey.hash();
    d->askCache.remove(keyHash);

    if (img.isNull())
        return;

    TileKey key(x, y, z);
    // img в очередь на сохранение в бд
    MapTileLoader *loader = d->loaders.value(type);
    if (loader) {
        bool needSave = true;
        if (loader->isTemporaryTiles() && !expires)
            needSave = false;

        if (needSave) {
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);

            img.save(&buffer, loader->fileformat().toUtf8());

            QVariantMap v;
            v["x"] = key.x;
            v["y"] = key.y;
            v["z"] = key.z;
            v["type"] = type;
            v["tile"] = ba;
            v["expires"] = expires;

            d->queueTiles->append(v);
            // если очередь слишком большая то сохраняем в бд
            if (d->queueTiles->size() >= d->maxQueueSize) {
                d->dc->postRequest("insTiles", QVariant::fromValue<dc::QueryResult>(*d->queueTiles), dc::LowestPriority, d, "onDb_TileSave");
                d->queueTiles->clear();
            }
        }
    }
    d->saveTileInCache(key, type, img, d->zoom == z);
}

void MapLayerTile::getdbImage(const TileKey &key, bool ignoreDb)
{
    Q_D(MapLayerTile);
    quint64 hash = key.hash();
    if (d->loadersErrorsCache.contains(hash))
        emit tileIncome(key, true);
    else {
        quint64 hash = key.hash();
        if (!d->dbEmptyCache.contains(hash) && !ignoreDb) {
            if (!d->dbCache.contains(hash)) {
                d->dbCache.insert(hash);
                if (d->dbCache.size() > d->MaxUIntCacheSize)
                    d->dbCache.clear();

                QVariantMap v;
                QStringList types;
                types.append(QString::number(key.type));
                v["x"] = key.x;
                v["y"] = key.y;
                v["z"] = key.z;
                v["type"] = types;
                v["level"] = d->levelUp;
                d->dc->postRequest("selTiles", v, dc::AboveNormalPriority, d, "onDb_TileIncome");
            }
        }
        else {
            MapTileLoader *loader = d->loaders.value(key.type);
            if (!loader)
                return;
            bool ignore = !loader->isEnabled();
            MapTileLoaderHttp *httploader = qobject_cast<MapTileLoaderHttp*>(loader);
            if (httploader && httploader->isProxyEnabled())
                ignore = false;
            if (ignore)
                return;
            if (!d->askCache.contains(hash)) {
                d->askCache.insert(hash);
                if (d->askCache.size() >= d->MaxUIntCacheSize)
                    d->askCache.clear();
                loader->getTile(key.x, key.y, key.z);
            }
        }
    }
}

void MapLayerTile::flushTiles()
{
    Q_D(MapLayerTile);
    d->flushTiles();
}

int MapLayerTile::tileUploadLevel() const
{
    Q_D(const MapLayerTile);
    return d->levelUp;
}

void MapLayerTile::setUploadLevel(int level)
{
    Q_D(MapLayerTile);
    level = qBound(0, level, TILEUPLOADLEVEL);
    if (d->levelUp == level)
        return;

    d->levelUp = level;
    d->map->update();
}

void MapLayerTile::loaderDestroid(QObject *loader)
{
    Q_D(MapLayerTile);
    d->loaders.take(d->loaders.key(static_cast<MapTileLoader*>(loader)));
}

// -----------------------------------------------------------------------------

} // namespace minigis

// -----------------------------------------------------------------------------
