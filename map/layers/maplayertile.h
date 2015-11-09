#ifndef MAPLAYERTILE_H
#define MAPLAYERTILE_H

#include <QObject>
#include <QMap>
#include <QWaitCondition>
#include <QPropertyAnimation>
#include <QBasicTimer>

#include "core/mapmath.h"
#include "loaders/maptileloader.h"

#include "layers/maplayer.h"
#include "coord/mapcamera.h"

#include "sql/mapsql.h"

namespace dc {
    class DatabaseController;
    typedef QList<QVariantMap> QueryResult;
}

//#define YANDEXMAP

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapLayerTilePrivate;
class MapLayerTile : public MapLayer
{
    Q_OBJECT

    Q_PROPERTY(QList<int> types READ types WRITE changeTileTypesL)

public:
    explicit MapLayerTile(QObject *parent = 0);
    virtual ~MapLayerTile();

    // clearLayer очистить слой
    virtual void clearLayer();

    MapTileLoader *loader(int type) const;
    QList<MapTileLoader*> loaders() const;
    QList<int> types() const;
    dc::DatabaseController * dc() const;

    // clearTileTypes очистить кэш исходников от типа type
    void clearTileTypes(int type);

public Q_SLOTS:
    bool registerLoader(MapTileLoader *loader);
    void unregisterLoader(MapTileLoader *loader);

    // addNewImage добавляем полученную картинку в кэш
    void addNewImage(QImage, int, int, int, int, int);

    // реагируем на изменение типа подложки
    void changeTileTypesL(QList<int>);
    void changeTileTypesI(int);

public:
    // changeColorizedTiles реагируем на изменение гаммы
    void changeColorizedTiles(ConvertColor::ColorFilterFunc);
    void clearCache();
    // установить настроки гаммы
    void setGraphOptions(QVariantMap options);

    // changeWatchType реагируем на изменение типа лоадера следящего за базой
    void changeWatchType(int);

    // getdbImage создать запрос недостающих тайлов
    void getdbImage(const TileKey &key, bool ignoreDb = false);

    // flushTiles принудительно сохранить в базу
    void flushTiles();

    // tileUploadLevel возвращает уровень подргузки тайлов
    int tileUploadLevel() const;
    // setUploadLevel установить уровень подгрузки тайлов
    void setUploadLevel(int level);

Q_SIGNALS:
    void tileIncome(const TileKey &key, bool empty, bool frombd = false);

protected Q_SLOTS:
    void loaderDestroid(QObject *);

protected:
    explicit MapLayerTile(MapLayerPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(MapLayerTile)
    Q_DISABLE_COPY(MapLayerTile)
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

// -------------------------------------------------------

#endif // MAPLAYERTILE_H
