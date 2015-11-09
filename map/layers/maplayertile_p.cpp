
#include <QPainter>
#include <QScreen>
#include <QPropertyAnimation>
#include <QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimerEvent>
#include <QBuffer>
#include <QDateTime>

#include <common/Runtime.h>
#include <db/databasecontroller.h>

#include "frame/mapframe.h"
#include "core/mapdefs.h"
#include "core/mapmath.h"
#include "coord/mapcoords.h"
#include "layers/maplayertile.h"
#include "frame/mapsettings.h"

// вывод нумерации тайла
// #define TILEINDEX

#include "maplayertile_p.h"


// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

MapLayerTilePrivate::MapLayerTilePrivate(QObject *parent)
    : MapLayerPrivate(parent), func(ConvertColor::emptyColor), zoom(0), base(1),
      cameraScale(0.), scaleChanged(true), cameraAngle(0.), angleChanged(true), levelUp(0), smoothing(true)
{
    dc = new dc::DatabaseController;
    QString error;

//    Settings& set = Settings::inst("map");
    QVariantMap connData;

    //connData["name"] = set.getDefault("db/dbname",   "map");
//    connData["name"] = set.getDefault("db/dbname",   Runtime::getSharePath() + "tiles.sqlite");
//    connData["user"] = set.getDefault("db/user",     "center");
//    connData["pass"] = set.getDefault("db/password", "center");
//    connData["host"] = set.getDefault("db/host",     "127.0.0.1");
//    connData["port"] = set.getDefault("db/port",      5432);

    QString path = Runtime::getSharePath();
    connData["name"] = path + "tiles.sqlite";

    if (!dc->init("QSQLITE", connData, &error))
        return;

    tileDB = new TilesDB;
    tileDB->setDc(dc);
    dc->postRequest("create", QVariant(), dc::RealTimePriority);
    queueTimer.start(FlushInterval, this);
    cacheTimer.start(errorClearTime, this);

    queueTiles = new dc::QueryResult;
}

MapLayerTilePrivate::~MapLayerTilePrivate()
{
    flushTiles(false);

    QMutexLocker lockerKey(&keyMutex);
    QList<MapTileLoader *> tmp(loaders.values());
    loaders.clear();
    foreach (MapTileLoader *loader, tmp) {
        loader->done();
        loader->setParent(NULL);
    }
    delete queueTiles;
    delete dc;
    delete tileDB;
}

QList<int> MapLayerTilePrivate::missedTypes(const TileKey &key)
{
    QList<int> tmpKeys = types;

    quint64 hash = key.hash();
    Tile *t = tiles.value(hash);
    if (t && !t->source.isEmpty()) {
        foreach (int sourceKey, t->source.keys())
            tmpKeys.removeOne(sourceKey);

        // обновляем кэш тайлов
        int ind = tileHistory.indexOf(hash);
        if (ind != -1)
            tileHistory.move(ind, 0);
        else
            tileHistory.prepend(hash);

        // чистим кэш тайлов
        while (tileHistory.size() > MaxCacheTile)
            delete tiles.take(tileHistory.takeLast());
    }
    return tmpKeys;
}

Tile *MapLayerTilePrivate::generateTile(const TileKey &key)
{
    quint64 hash = key.hash();
    Tile *t = tiles.value(hash);
    // создаем новый тайл
    if (!t) {
        t = new Tile(key);
        tiles[hash] = t;
    }

//    if (!t->origin) {
//        if (!createUploadTile(t)) {
//            t->clear(Tile::Original);
//            createTmpTile(t);
//        }
//        else
//            t->opacity = 1.;

//        if (t->origin)
//            t->clear(Tile::Colorized);
//        else { // удаляем тайл, так как нету оригинала
//            tiles.remove(t->key.hash());
//            delete t;
//            t = NULL;
//        }
//    }
    return t;
}

TileKey MapLayerTilePrivate::createImages(Tile *t)
{
    TileKey key = t->key;
    if (!t->origin) {
        QImage img = createUploadTileThread(key);
        if (img.isNull())
            img = createTmpTileThread(key);

        if (!img.isNull()) {
            t->origin = new QImage(img);
            t->opacity = 1;
//            t->clear(Tile::Colorized);
        }
        else {
//            {
//                QMutexLocker locker(&keyMutex);
//                tiles.remove(key.hash());
//            }
//            delete t;
//            t = NULL;

            return key;
        }
    }

    if (!t->colorized)
        t->colorized = new QImage(createColorizedTileThread(t));
    if (!t->scaled)
        t->scaled = new QImage(createScaledTileThread(t->colorized));
    if (!t->rotated)
        t->rotated = new QImage(createRotatedTileThread(key, t->scaled));

    {
        QMutexLocker locker(&keyMutex);
        tiles.insert(key.hash(), t);
    }

    return key;
}

QImage MapLayerTilePrivate::createUploadTileThread(const TileKey &key)
{
    QImage img = QImage(QSize(TileSize, TileSize), QImage::Format_ARGB32_Premultiplied);

    img.fill(Qt::transparent);
    QPainter painter(&img);

    int num = 0;
    int upBound = 0;
    QList<int> active;
    {
        QMutexLocker locker(&keyMutex);
        active = types;
        upBound = levelUp;
    }

    foreach (int activeType, active) {
        int shift = 0;
        while (shift <= upBound) {
            TileKey prevKey(key.x >> shift, key.y >> shift, key.z - shift);

            QImage *origin = NULL;
            {
                QMutexLocker locker(&keyMutex);
                Tile *prevTile = tiles.value(prevKey.hash());
                origin = prevTile ? prevTile->source.value(activeType) : NULL;
            }
            if (origin) {
                ++num;
                int rectSize = TileSize >> shift;
                QRect source(QPoint(key.x - (prevKey.x << shift), key.y - (prevKey.y << shift)) * rectSize, QSize(rectSize, rectSize));

                painter.drawImage(img.rect(), *origin, source);
                break;
            }
            ++shift;
        }
    }
    painter.end();

    return num == 0 ? QImage() : img;
}

QImage MapLayerTilePrivate::createTmpTileThread(const TileKey &key)
{
    int shift = 1;
    int pz = 0;
    int z = 0;
    bool sm = 0;
    {
        QMutexLocker locker(&keyMutex);
        pz = prevZoom;
        z = zoom;
        sm = smoothing;
    }

    QImage img;
    int count = -1;
    if (pz < z) {
        TileKey prevKey(key.x >> shift, key.y >> shift, key.z - shift);

        QImage *origin = NULL;
        {
            QMutexLocker locker(&keyMutex);
            Tile *prevTile = tiles.value(prevKey.hash());
            if (prevTile && prevTile->origin && !prevTile->source.isEmpty())
                origin = prevTile->origin;
        }

        if (origin) {
            int rectSize = TileSize >> shift;
            QRect source(QPoint(key.x - (prevKey.x << shift), key.y - (prevKey.y << shift)) * rectSize, QSize(rectSize, rectSize));
            img = QImage(origin->copy(source).scaled(TileSize, TileSize, Qt::KeepAspectRatio, sm ? Qt::SmoothTransformation : Qt::FastTransformation));
        }
    }
    else if (pz != z) {
        count = 0;
        TileKey prevKey(key.x << shift, key.y << shift, key.z + shift);

        img = QImage(TileSize, TileSize, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter paint(&img);

        if (sm)
            paint.setRenderHints(QPainter::SmoothPixmapTransform);

        int rectSize = TileSize >> shift;
        int side = 1 << shift;
        int max = side * side;

        QRect dest(QPoint(), QSize(rectSize, rectSize));

        for (int i = 0; i < max; ++i) {
            TileKey tmpKey(prevKey.x + i % side, prevKey.y + int(i / side), prevKey.z);
            dest.moveTopLeft(rectSize * QPoint(i % side, i / side));

            QImage *origin = NULL;
            {
                QMutexLocker locker(&keyMutex);
                Tile *prevTile = tiles.value(tmpKey.hash());
                if (prevTile && prevTile->origin && !prevTile->source.isEmpty())
                    origin = prevTile->origin;
            }

            if (origin) {
                paint.drawImage(dest, *origin);
                ++count;
            }
        }

        paint.end();
    }
    return count == 0 ? QImage() : img;
}

QImage MapLayerTilePrivate::createColorizedTileThread(Tile *t)
{
    QVariantMap opt;
    bool night = 0;
    {
        QMutexLocker locker(&keyMutex);
        opt = graphOpt;
        night = map->settings()->isNightModeEnabled();
    }

    QImage colorized;
    if (night) {
        if (t->source.isEmpty()) {
            colorized = *t->origin;
            ConvertColor::invertedColor(&colorized, QVariantMap());
            func(&colorized, opt);
        }
        else {
            colorized = QImage(QSize(TileSize, TileSize), QImage::Format_ARGB32_Premultiplied);
            colorized.fill(Qt::transparent);

            QPainter painter(&colorized);
            for (QHashIterator<int, QImage*>it(t->source); it.hasNext(); ) {
                it.next();
                QImage *img = it.value();
                if (!img)
                    continue;

                bool loaderNight = false;
                {
                    QMutexLocker locker(&keyMutex);
                    MapTileLoader *loader = loaders.value(it.key());
                    loaderNight = (loader && loader->nightModeAvalible());
                }

                if (loaderNight) {
                    QImage invers = img->convertToFormat(QImage::Format_ARGB32_Premultiplied);
                    ConvertColor::invertedColor(&invers, QVariantMap());
                    painter.drawImage(QPointF(), invers);
                }
                else
                    painter.drawImage(QPointF(), *img);
            }
            func(&colorized, opt);
        }
    }
    else {
        colorized = *t->origin;
        func(&colorized, opt);
    }
    return colorized;
}

QImage MapLayerTilePrivate::createScaledTileThread(QImage *colorized)
{
    if (!colorized)
        return QImage();
    int size = 0;
    bool sm = 0;
    {
        QMutexLocker lock(&keyMutex);
        size = tilePixSize;
        sm = smoothing;
    }
    return colorized->scaled(size, size, Qt::KeepAspectRatio, sm ? Qt::SmoothTransformation : Qt::FastTransformation);
}

QImage MapLayerTilePrivate::createRotatedTileThread(const TileKey &key, QImage *scaled)
{
    if (!scaled)
        return QImage();

    QPointF pos;
    QTransform tr;
    QTransform backTr;
    bool sm = 0;
    {
        QMutexLocker lock(&keyMutex);
        tr = tileTr;
        backTr = tileBackTr;

        QRectF tileRect = TileSystem::tileBounds(QPoint(base - key.y - 1, key.x), zoom);
        pos = map->camera()->toScreen(tileRect.topRight());

        sm = smoothing;
    }

    return tr.isIdentity() ? *scaled : generateTransformImage(*scaled, pos, tr, backTr, NULL, 1, sm);
}

QList<int> MapLayerTilePrivate::getCacheImage(QPainter *painter, const TileKey &key)
{
    QList<int> tmpKeys = types;
    // работа с локальным кэшем
    {
        QMutexLocker locker(&keyMutex);

        // ищем нужный тайл
        quint64 hash = key.hash();
        Tile *t = tiles.value(hash);
        // создаем новый тайл
        if (!t)
            tiles[hash] = t = new Tile(key);

        // если нету оригинала
        if (!t->origin) {
            if (!createUploadTile(t)) {
                t->clear(Tile::Original);
                createTmpTile(t);
            }
            else
                t->opacity = 1.;

            if (t->origin)
                t->clear(Tile::Colorized);
            else { // удаляем тайл, так как нету оригинала
                tiles.remove(t->key.hash()/*tiles.key(t)*/);
                delete t;
                t = NULL;
            }
        }

        // Работа с тайлами
        if (t) {
            // создаем недостающие картинки и рисуем тайл
            if (t->origin) {
                if (!t->colorized)
                    createColorizedTile(t);
                if (!t->scaled)
                    createScaledTileDirect(t);
                if (!t->rotated)
                    createTransformTileDirect(t);

                if (t->rotated)
                    localDraw(painter, t);
            }

            if (!t->source.isEmpty()) {
                // оставляем только нужные ключи
                foreach (int sourceKey, t->source.keys())
                    tmpKeys.removeOne(sourceKey);

                // обновляем кэш тайлов
                int ind = tileHistory.indexOf(hash);
                if (ind != -1)
                    tileHistory.move(ind, 0);
                else
                    tileHistory.prepend(hash);

                // чистим кэш тайлов
                while (tileHistory.size() > MaxCacheTile)
                    delete tiles.take(tileHistory.takeLast());
            }
        }
    }
    return tmpKeys;
}

void MapLayerTilePrivate::getdbImage(const QList<int> types, const TileKey &key)
{
    // создаем список типов для запросов
    QStringList needTypes;
    QList<TileKey> loaderList;
    foreach (int activeType, types) {
        TileKey keyTmp(key.x, key.y, key.z, activeType);
        quint64 hash = keyTmp.hash();
        // добавляем в кэш запросов в бд
        if (!dbEmptyCache.contains(hash)) {
            if (!dbCache.contains(hash)) {
                dbCache.insert(hash);
                if (dbCache.size() > MaxUIntCacheSize)
                    dbCache.clear();
                needTypes.append(QString::number(activeType));
            }
        }
        else
            loaderList.append(keyTmp);
    }

    // запрос в бд
    if (!needTypes.isEmpty()) {
        QVariantMap v;
        v["x"] = key.x;
        v["y"] = key.y;
        v["z"] = key.z;
        v["type"] = needTypes;
        v["level"] = levelUp;
        dc->postRequest("selTiles", v, dc::AboveNormalPriority, this, "onDb_TileIncome");
    }

    // запрос на сервер
    foreach (TileKey key, loaderList) {
        MapTileLoader *loader = loaders.value(key.type);
        if (!loader)
            continue;
        // если лоадер выключен то игнориреум его
        bool ignore = !loader->isEnabled();
        MapTileLoaderHttp *httploader = qobject_cast<MapTileLoaderHttp*>(loader);
        if (httploader && httploader->isProxyEnabled())
            ignore = false;
        if (ignore)
            continue;
        // добавляем в кэш лоадеров
        quint64 keyHash = key.hash();
        if (!askCache.contains(keyHash) && !loadersErrorsCache.contains(keyHash)) {
            askCache.insert(keyHash);
            if (askCache.size() >= MaxUIntCacheSize)
                askCache.clear();

            // заправшиваем тайл
            loader->getTile(key.x, key.y, key.z);
        }
    }
}

void MapLayerTilePrivate::createColorizedTile(Tile *t)
{
    if (map->settings()->isNightModeEnabled()) {
        if (t->source.isEmpty()) {
            t->colorized = new QImage(*t->origin);
            ConvertColor::invertedColor(t->colorized, QVariantMap());
            func(t->colorized, graphOpt);
        }
        else {
            t->colorized = new QImage(QSize(TileSize, TileSize), QImage::Format_ARGB32_Premultiplied);
            t->colorized->fill(Qt::transparent);

            QPainter painter(t->colorized);
            for (QHashIterator<int, QImage*>it(t->source); it.hasNext(); ) {
                it.next();
                QImage *img = it.value();
                if (!img)
                    continue;
                MapTileLoader *loader = loaders.value(it.key());
                if (loader && loader->nightModeAvalible()) {
                    QImage invers = img->convertToFormat(QImage::Format_ARGB32_Premultiplied);
                    ConvertColor::invertedColor(&invers, QVariantMap());
                    painter.drawImage(QPointF(), invers);
                }
                else
                    painter.drawImage(QPointF(), *img);
            }
            func(t->colorized, graphOpt);
        }
    }
    else {
        t->colorized = new QImage(*t->origin);
        func(t->colorized, graphOpt);
    }
}

void MapLayerTilePrivate::createScaledTileDirect(Tile *t)
{
    if (t->scaled)
        t->clear(Tile::Scaled);
    t->scaled = new QImage(t->colorized->scaled(tilePixSize, tilePixSize, Qt::KeepAspectRatio, smoothing ? Qt::SmoothTransformation : Qt::FastTransformation));
}

void MapLayerTilePrivate::createTransformTileDirect(Tile *t)
{
    if (!t->scaled)
        return;

    TileKey key = t->key;
    QPoint tilePos(base - key.y - 1, key.x);
#ifndef YANDEXMAP
    QRectF tileRect = TileSystem::tileBounds(tilePos, zoom);
#else
    QRectF tileRect = MyUtils::tileBounds(tilePos, zoom);
#endif
    QPointF pos = camera->toScreen(tileRect.topRight());

    t->rotated = new QImage(
                tileTr.isIdentity() ?
                    *t->scaled :
                    generateTransformImage(*t->scaled, pos, tileTr, tileBackTr, NULL, 1., smoothing)
                    );
}

void MapLayerTilePrivate::localUpdate(int x, int y, int z)
{
    if (z != zoom)
        return;
    if (!visionTileRect.contains(QPoint(x, y)))
        return;

#ifndef YANDEXMAP
    QRectF rect = TileSystem::tileBounds(QPoint(x, y), z);
#else
    QRectF rect = MyUtils::tileBounds(QPoint(x, y), z);
#endif
    rect = QRectF(QPointF(-rect.top(), rect.left()), QPointF(-rect.bottom(), rect.right()));
    rect = camera->toScreen().mapRect(rect);
    QRect tileRect(rect.topLeft().toPoint(), QSize(qCeil(rect.width()), qCeil(rect.height())));

//    emit needRender(tileRect.intersected(layerScene.rect()));
    emit needRender(tileRect);
}

void MapLayerTilePrivate::loaderError(int x, int y, int z, int type)
{
    TileKey key(x, y, z, type);
    emit tileIncome(TileKey(x, y, z, type), true);
//    qDebug() << "----------------------------" << x << y << z << type << loadersErrorsCache.contains(key.hash());
    loadersErrorsCache.insert(key.hash());
}

void MapLayerTilePrivate::localDraw(QPainter *painter, Tile *t)
{
    TileKey const &key = t->key;
#ifndef YANDEXMAP
    QRectF rectT = TileSystem::tileBounds(QPoint(base - 1 - key.y, key.x), key.z);
#else
    QRectF rect = MyUtils::tileBounds(QPoint(base - 1 - key.y, key.x), key.z);
#endif
    QRectF rect = camera->toScreen().mapRect(rectT);
    painter->save();

#ifdef TILEANIMATION
    QRectF tileRect(rect.topLeft(), QSize(qCeil(rect.width()), qCeil(rect.height())));
    if (t->prevTmp) {
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter->setOpacity(1 - t->opacity * 0.75);
        painter->drawImage(tileRect, *t->prevTmp);
        painter->restore();
    }
    painter->setOpacity(t->opacity);
#endif

    painter->drawImage(rect.topLeft(), *t->rotated);
    // отрисовка стык в стык
//    painter->drawImage(rect, *t->rotated);
    painter->restore();

#ifdef TILEINDEX
    painter->save();
    painter->setPen(Qt::black);
    painter->drawRect(rect);
    painter->setBrush(QColor(255, 255, 255, 200));
    QFontMetricsF fm(painter->font());
    QString x = QString("X:%1 %2 %3").arg(key.x).arg((int)rectT.left()).arg((int)rect.left());
    QString y = QString("Y:%1 %2 %3").arg(key.y).arg((int)rectT.top()).arg((int)rect.left());
    QString z = QString("Z:%1").arg(key.z);
    qreal w = qMax(fm.width(x), qMax(fm.width(y), fm.width(z)));
    painter->drawRect(rect.adjusted(10, 20, -rect.width() + w + 30, -rect.height() + 130));
    painter->drawText(rect.topLeft() + QPoint(20, 40),  x);
    painter->drawText(rect.topLeft() + QPoint(20, 80),  y);
    painter->drawText(rect.topLeft() + QPoint(20, 120), z);
    painter->restore();
#endif
}

bool MapLayerTilePrivate::createUploadTile(Tile *t)
{
    if (!t->origin)
        t->origin = new QImage(QSize(TileSize, TileSize), QImage::Format_ARGB32_Premultiplied);

    TileKey key = t->key;

    t->origin->fill(Qt::transparent);
    QPainter painter(t->origin);
    int num = 0;
    foreach (int activeType, types) {
        int shift = 0;
        while (shift <= levelUp) {
            TileKey prevKey(key.x >> shift, key.y >> shift, key.z - shift);
            Tile *prevTile = tiles.value(prevKey.hash());

            QImage *origin = prevTile ? prevTile->source.value(activeType) : NULL;
            if (origin) {
                ++num;
                int rectSize = TileSize >> shift;
                QRect source(QPoint(key.x - (prevKey.x << shift), key.y - (prevKey.y << shift)) * rectSize, QSize(rectSize, rectSize));

                painter.drawImage(t->origin->rect(), *origin, source);
                break;
            }
            ++shift;
        }
    }
    painter.end();

    return num != 0;
}

void MapLayerTilePrivate::createTmpTile(Tile *t)
{
    TileKey key = t->key;
    int shift = 1;
    if (prevZoom < zoom) {
        TileKey prevKey(key.x >> shift, key.y >> shift, key.z - shift);
        Tile *prevTile = tiles.value(prevKey.hash());
        if (prevTile && prevTile->origin && !prevTile->source.isEmpty()) {
            QImage *origin = prevTile->origin;

            int rectSize = TileSize >> shift;
            QRect source(QPoint(key.x - (prevKey.x << shift), key.y - (prevKey.y << shift)) * rectSize, QSize(rectSize, rectSize));

            if (t->origin)
                delete t->origin;
            t->origin = new QImage(origin->copy(source).scaled(TileSize, TileSize, Qt::KeepAspectRatio, smoothing ? Qt::SmoothTransformation : Qt::FastTransformation));
        }
    }
    else if (prevZoom != zoom) {
        TileKey prevKey(key.x << shift, key.y << shift, key.z + shift);
        if (t->origin)
            delete t->origin;
        t->origin = new QImage(TileSize, TileSize, QImage::Format_ARGB32_Premultiplied);

        t->origin->fill(Qt::transparent);
        QPainter paint(t->origin);

        if (smoothing)
            paint.setRenderHints(QPainter::SmoothPixmapTransform);

        int rectSize = TileSize >> shift;
        int side = 1 << shift;
        int max = side * side;

        QRect dest(QPoint(), QSize(rectSize, rectSize));

        int count = 0;
        for (int i = 0; i < max; ++i) {
            TileKey tmpKey(prevKey.x + i % side, prevKey.y + int(i / side), prevKey.z);
            dest.moveTopLeft(rectSize * QPoint(i % side, i / side));
            Tile *prevTile = tiles.value(tmpKey.hash());
            if (prevTile && prevTile->origin && !prevTile->source.isEmpty()) {
                paint.drawImage(dest, *prevTile->origin);
                ++count;
            }
        }
        paint.end();
        if (count == 0)
            t->clear(Tile::Original);
    }

    if (t->origin)
        t->opacity = 1;
}

void MapLayerTilePrivate::saveTileInCache(const TileKey &key, int type, const QImage &img, bool flag)
{
    {
        QMutexLocker locker(&keyMutex);

        if (flag && (key.z != zoom || !visionTileRect.contains(QPoint(key.x, key.y))))
            return;

        quint64 hash = key.hash();
        Tile *t = tiles[hash];
        if (!t) {
            t = new Tile(key);
            tiles[hash] = t;
        }

        if (t->source.isEmpty()) {
            t->opacity = flag ? 0 : 1;
            if (t->rotated)
                t->prevTmp = new QImage(*t->rotated);
        }

        if (!t->source.contains(type))
            t->source[type] = new QImage(img);
        else
            *t->source[type] = img;
        {
            createUploadTile(t);
//            updateLowerTiles(t);

            t->clear(Tile::Colorized);
        }

        int ind = tileHistory.indexOf(hash);
        if (ind != -1)
            tileHistory.move(ind, 0);
        else
            tileHistory.prepend(hash);

        while (tileHistory.size() > MaxCacheTile)
            delete tiles.take(tileHistory.takeLast());

        // создаем анимацию появления тайла
#ifdef TILEANIMATION
        if (qFuzzyIsNull(t->opacity)) {
            if (!t->ani) {
                t->ani = new QPropertyAnimation(t, "opacity", this);
                connect(t, SIGNAL(needUpdate(int,int,int)), SLOT(localUpdate(int,int,int)));
                connect(t->ani.data(), SIGNAL(finished()), t, SLOT(removeTmp()));
            }

            if (t->ani->state() != QPropertyAnimation::Running) {
                t->ani->setStartValue(0);
                t->ani->setEndValue(1);
                t->ani->setDuration(TileAnimationTime);
                t->ani->start(QPropertyAnimation::DeleteWhenStopped);
            }
        }
#else
        t->opacity = 1.;
#endif
    }

    if (flag)
        localUpdate(key.x, key.y, key.z);
}

void MapLayerTilePrivate::flushTiles(bool flag)
{
    if (queueTiles->isEmpty()) {
        if (!flag)
            onDb_EndTile(0, QVariant(), QVariant());
        return;
    }
    dc->postRequest("insTiles", QVariant::fromValue<dc::QueryResult>(*queueTiles), dc::LowestPriority, this, flag ? "onDb_TileSave" : "onDb_EndTile");
    queueTiles->clear();
}

void MapLayerTilePrivate::changeZoom(qreal scale)
{
    int tmp = TileSystem::zoomForPixelSize(1. / scale);
    if (zoom != tmp) {
        prevZoom = zoom;
        zoom = tmp;
        base = 1 << zoom;
    }
}

QList<TileKey> sortedKeys(int startX, int endX, int startY, int endY, int zoom)
{
    QList<TileKey> keys;
    int sumX = startX + endX;
    int sumY = startY + endY;
    int midX = sumX % 2 == 0 ? sumX / 2 : qCeil(sumX * 0.5);
    int midY = sumY % 2 == 0 ? sumY / 2 : qCeil(sumY * 0.5);

    int xPlus = endX - midX;
    int xMinus = midX - startX;
    int yPlus = endY - midY;
    int yMinus = midY - startY;

    keys.append(TileKey(midX, midY, zoom));

    int posY = 0;
    int posX = 0;
    int size = 0;
    int xStart = 0;
    int yStart = 0;

    int k = 1;
    bool next = true;
    while (next) {
        next = false;
        size = 2 * k;
        if (k - 1 < xPlus) {
            next = true;
            posX = midX + k;
            yStart = midY - k + 1;
            for (int i = 0; i < size; ++i) {
                posY = yStart + i;
                if (posY >= startY && posY <= endY)
                    keys.append(TileKey(posX, posY, zoom));
            }
        }
        if (k - 1 < yPlus) {
            next = true;
            posY = midY + k;
            xStart = midX + k - 1;
            for (int i = 0; i < size; ++i) {
                posX = xStart - i;
                if (posX >= startX && posX <= endX)
                    keys.append(TileKey(posX, posY, zoom));
            }
        }
        if (k - 1 < xMinus) {
            next = true;
            posX = midX - k;
            yStart = midY + k - 1;
            for (int i = 0; i < size; ++i) {
                posY = yStart - i;
                if (posY >= startY && posY <= endY)
                    keys.append(TileKey(posX, posY, zoom));
            }
        }
        if (k - 1 < yMinus) {
            next = true;
            posY = midY - k;
            xStart = midX - k + 1;
            for (int i = 0; i < size; ++i) {
                posX = xStart + i;
                if (posX >= startX && posX <= endX)
                    keys.append(TileKey(posX, posY, zoom));
            }
        }
        ++k;
    }
    return keys;
}

void MapLayerTilePrivate::calcVisualRect()
{
    QRectF worldRect = camera->toWorld().mapRect(QRectF(QPoint(0, 0), camera->screenSize())); // экран в мировых координатах

    QPolygon tilePoints; // список индесков подложки QPoint(column, row)
#ifndef YANDEXMAP
    tilePoints << TileSystem::metersToTile(worldRect.topLeft(), zoom)
               << TileSystem::metersToTile(worldRect.bottomRight(), zoom);
#else
    tilePoints << MyUtils::metersToEllipticTile(worldRect.topLeft(), zoom)
               << MyUtils::metersToEllipticTile(worldRect.bottomRight(), zoom);
#endif

#ifndef YANDEXMAP
    int startX = qMax(tilePoints.at(0).y(), 0);
    int endX   = qMin(tilePoints.at(1).y(), base - 1);
#else
    int startX = base - 1 - qMin(tilePoints.at(0).y(), base - 1);
    int endX   = base - 1 - qMax(tilePoints.at(1).y(), 0);
#endif

    int startY = base - 1 - qMin(tilePoints.at(1).x(), base - 1);
    int endY   = base - 1 - qMax(tilePoints.at(0).x(), 0);

    QMutexLocker locker(&keyMutex);
    visionTileRect = QRect(QPoint(startX, startY),
                           QPoint(endX  , endY));
}

void MapLayerTilePrivate::updateLowerTiles(Tile *t)
{
    TileKey key = t->key;
    if (key.z >= zoom)
        return;

    int shift = zoom - key.z;
    TileKey prevKey(key.x << shift, key.y << shift, key.z + shift);

    int side = 1 << shift;
    int max = side * side;

    for (int i = 0; i < max; ++i) {
        TileKey tmpKey(prevKey.x + i % side, prevKey.y + int(i / side), prevKey.z);
        Tile *tile = tiles.value(tmpKey.hash());
        if (tile)
            tile->clear(Tile::Original);
        if (visionTileRect.contains(tmpKey.x, tmpKey.y))
            localUpdate(tmpKey.x, tmpKey.y, tmpKey.z);
    }
}

//! =======================================================================
void MapLayerTilePrivate::render(QPainter *painter, const QRectF &rgn, const MapCamera *camera, MapOptions options)
{
    Q_ASSERT(painter);
    QRectF worldRect = camera->toWorld().mapRect(rgn); // экран в мировых координатах

    bool newSmoothing = options.testFlag(optSubstrateSmoothing);
    if (!qFuzzyCompare(cameraScale, camera->scale()) || newSmoothing != smoothing) { // изменился scale
        smoothing = newSmoothing;

        changeZoom(camera->scale());
        cameraScale = camera->scale();
        scaleChanged = true;

        QMutexLocker locker(&keyMutex);
        std::for_each(tiles.begin(), tiles.end(), std::bind2nd(std::mem_fun(&Tile::clear), Tile::Scaled));
    }
    else if (!qFuzzyCompare(cameraAngle, camera->angle())) { // изменился угол поворота
        cameraAngle = camera->angle();
        angleChanged = true;

        QMutexLocker locker(&keyMutex);
        std::for_each(tiles.begin(), tiles.end(), std::bind2nd(std::mem_fun(&Tile::clear), Tile::Rotated));
    }

    // список индесков подложки QPoint(column, row)
    QPolygon tilePoints;
#ifdef YANDEXMAP
    tilePoints << MyUtils::metersToEllipticTile(worldRect.topLeft(), zoom)
               << MyUtils::metersToEllipticTile(worldRect.bottomRight(), zoom);
#else
    tilePoints << TileSystem::metersToTile(worldRect.topLeft(), zoom)
               << TileSystem::metersToTile(worldRect.bottomRight(), zoom);
#endif

    //     tileIndexes
    int startX = qMax(tilePoints.at(0).y(), 0);
    int endX   = qMin(tilePoints.at(1).y(), base - 1);

    int startY = base - 1 - qMin(tilePoints.at(1).x(), base - 1);
    int endY   = base - 1 - qMax(tilePoints.at(0).x(), 0);

    calcVisualRect();

    // tileKeys
    QList<TileKey> keyList;
    for (int x = startX ; x <= endX; ++x)
        for (int y = startY; y <= endY; ++y)
            keyList.append(TileKey(x, y, zoom));

    //    keyList = sortedKeys(startX, endX, startY, endY, zoom);

    // размер подложки в мировых координатах
#ifdef YANDEXMAP
    QRectF tileRect = MyUtils::tileBounds(tilePoints.first(), zoom);
#else
    QRectF tileRect = TileSystem::tileBounds(tilePoints.first(), zoom);
#endif

    // начальная точка
    QPointF pointStart = camera->toScreen(tileRect.topRight());
    // смещение начальный точки
    qreal step = qAbs((tileRect.topLeft() - tileRect.topRight()).x());

    // размер тайла в экранных координатах
    tilePixSize = qCeil(lengthR2(camera->toScreen(pointStart + QPointF(step, 0)) - camera->toScreen(pointStart)));

    tileTr.reset();
    tileTr.translate(pointStart.x(), pointStart.y()).rotate(camera->angle()).translate(-pointStart.x(), -pointStart.y());
    tileBackTr = tileTr.inverted();

#if 0
    foreach (const TileKey &key, keyList) {
        QList<int> tmp = getCacheImage(painter, key);
        getdbImage(tmp, key);
    }
#else
    QList<QFuture<TileKey> > futuerList;

//    futureCount = 0;
    foreach (const TileKey &key, keyList) {
        QList<int> tmp = missedTypes(key);
        Tile *t = generateTile(key);

        if (t) {
            if (!t->rotated) {
//                QFutureWatcher<TileKey> *watcher = new QFutureWatcher<TileKey>(this);
//                watcher->setFuture(QtConcurrent::run(this, &MapLayerTilePrivate::createImages, t));
//                connect(watcher, SIGNAL(finished()), SLOT(incomeFuture()));

//                {
//                    QMutexLocker locker(&keyMutex);
//                    ++futureCount;
//                }

                futuerList.append(QtConcurrent::run(this, &MapLayerTilePrivate::createImages, t));
            }
            else {
                localDraw(painter, t);
            }
        }
        getdbImage(tmp, key);
    }

//    if (futureCount > 0)
//        wait.wait(&mutex);

    while (!futuerList.isEmpty()) {
//        for (QMutableListIterator<QFuture<TileKey> > it(futuerList); it.hasNext(); ) {
//            it.next();
//            if (!it.value().isFinished())
//                continue;
            QFuture<TileKey> fut = futuerList.takeFirst();
            if (!fut.isFinished()) {
                futuerList.append(fut);
                continue;
            }

            TileKey res = fut.result();
            quint64 hash = res.hash();
            Tile *t = tiles.value(hash);
            if (t) {
                if (t->rotated) {
                    localDraw(painter, t);
                }
                else if (!t->origin && t->source.isEmpty()) {
                    QMutexLocker locker(&keyMutex);
                    tiles.remove(hash);
                    delete t;
                    t = NULL;
                }
            }
//        }
    }
#endif

    scaleChanged = false;
    angleChanged = false;
}

//! =======================================================================

void MapLayerTilePrivate::onDb_TileIncome(uint /*query*/, QVariant result, QVariant /*error*/)
{
    if (result.isNull() || !result.canConvert<dc::QueryResult>())
        return;

    int time = QDateTime::currentDateTime().toTime_t();
    QStringList expiresTiles;

    dc::QueryResult query = result.value<dc::QueryResult>();

    QVariantMap f = query.first();
    TileKey basicTile(f.value("x").toInt(), f.value("y").toInt(), f.value("z").toInt());
    foreach (QVariantMap vm, query) {
        TileKey key(
                    vm.value("x").toInt(),
                    vm.value("y").toInt(),
                    vm.value("z").toInt()
                    );

        int type = vm.value("type").toInt();

        quint64 keyHash = TileKey(key.x, key.y, key.z, type).hash();

        QByteArray imgData = vm.value("tile").toByteArray();
        QImage img;
        img.loadFromData(imgData);

        int expires = vm.value("expires").toInt();
        bool tileExpired = (expires != 0) && (expires < time);

        if (!img.isNull() && !tileExpired)
            emit tileIncome(TileKey(key.x, key.y, key.z, type), false, true);

        // если img не пуст то сохраняем его в кэш
        dbCache.remove(keyHash);
        if (!img.isNull())
            saveTileInCache(key, type, img, key == basicTile);
        else
            dbEmptyCache.insert(keyHash);

        // если нету img в базе или он устарел запрашиваем новый тайл в инете
        if (img.isNull() || tileExpired) {
            MapTileLoader *loader = loaders.value(type);
            if (!loader)
                continue;

            // если лоадер выключен то игнориреум его
            bool ignore = !loader->isEnabled();
            MapTileLoaderHttp *httploader = qobject_cast<MapTileLoaderHttp*>(loader);
            if (httploader && httploader->isProxyEnabled())
                ignore = false;
            if (ignore)
                continue;

            if (tileExpired && loader->isTemporaryTiles())
                expiresTiles.append(vm.value("id").toString());

            // добавляем в кэш лоадеров
            if (!askCache.contains(keyHash) && !loadersErrorsCache.contains(keyHash)) {
                askCache.insert(keyHash);
                if (askCache.size() >= MaxUIntCacheSize)
                    askCache.clear();

                // заправшиваем тайл
                loader->getTile(key.x, key.y, key.z);
            }
        }
    }

    // удаляем из базы устаревшие тайлы
    if (!expiresTiles.isEmpty())
        dc->postRequest("remTiles", QVariant::fromValue<QStringList>(expiresTiles), dc::NormalPriority);
}

void MapLayerTilePrivate::onDb_TileSave(uint /*query*/, QVariant result, QVariant /*error*/)
{
    if (result.isNull() || !result.canConvert<dc::QueryResult>())
        return;
    foreach (QVariantMap vm, result.value<dc::QueryResult>()) {
        TileKey key(
                    vm.value("x").toInt(),
                    vm.value("y").toInt(),
                    vm.value("z").toInt(),
                    vm.value("type").toInt()
                    );
        dbEmptyCache.remove(key.hash());
    }
}

void MapLayerTilePrivate::onDb_EndTile(uint /*query*/, QVariant /*result*/, QVariant /*error*/)
{
    dc->done();
}

void MapLayerTilePrivate::incomeFuture()
{
    QFutureWatcher<TileKey> *w = dynamic_cast<QFutureWatcher<TileKey> *>(sender());
    if (!w)
        return;
    TileKey key = w->future().result();

    quint64 hash = key.hash();
    Tile *t = NULL;
    {
        QMutexLocker locker(&keyMutex);
        t = tiles.value(hash);
    }
    if (t) {
        if (t->rotated) {
//            localDraw(painter, t);
        }
        else if (!t->origin && t->source.isEmpty()) {
            QMutexLocker locker(&keyMutex);
            tiles.remove(hash);
            delete t;
            t = NULL;
        }
    }

    delete w;
    QMutexLocker locker(&keyMutex);
    --futureCount;
    if (futureCount == 0)
        wait.wakeAll();
}

void MapLayerTilePrivate::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == queueTimer.timerId())
        flushTiles();
    else if (e->timerId() == cacheTimer.timerId())
        loadersErrorsCache.clear();
}

// -----------------------------------------------------------------------------

} // namespace minigis

// -----------------------------------------------------------------------------

