#ifndef MAPLAYERTILE_P_H
#define MAPLAYERTILE_P_H

#include <QObject>
#include <QMap>
#include <QWaitCondition>
#include <QPropertyAnimation>
#include <QBasicTimer>

#include "loaders/maptileloader.h"

#include "layers/maplayer.h"
#include "layers/maplayer_p.h"

#include "coord/mapcamera.h"
#include "sql/mapsql.h"
#include "layers/maplayertile.h"

namespace dc {
    class DatabaseController;
    typedef QList<QVariantMap> QueryResult;
}

//#define YANDEXMAP

// -------------------------------------------------------

namespace minigis {

class Tile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ getOpacity WRITE setOpacity NOTIFY needUpdate)

public:
    Tile(TileKey tKey, QObject *parent = NULL) : QObject(parent),
        origin(NULL), colorized(NULL), scaled(NULL), rotated(NULL), prevTmp(NULL),
        opacity(.0), key(tKey), ani(NULL) {}

    ~Tile() {
        clear();
        qDeleteAll(source.values());
    }

    void setSource(QList<int> const &types) {
        for (QMutableHashIterator<int, QImage*> it(source); it.hasNext();) {
            it.next();
            if (!types.contains(it.key())) {
                if (it.value())
                    delete it.value();
                it.remove();
            }
        }
        clear();
    }

    enum ClearBit{
        OriginBit    = 0x01,
        ColorizedBit = 0x02,
        ScaledBit    = 0x04,
        RotatedBit   = 0x08,

        Original     = OriginBit | ColorizedBit | ScaledBit | RotatedBit,
        Colorized    =             ColorizedBit | ScaledBit | RotatedBit,
        Scaled       =                            ScaledBit | RotatedBit,
        Rotated      =                                        RotatedBit,

        All          = Original
    };
    Q_DECLARE_FLAGS(ClearBits, ClearBit)

    void clear(ClearBits flag = All) {
        if (origin && flag.testFlag(OriginBit)) {
            delete origin;
            origin = NULL;
        }
        if (colorized && flag.testFlag(ColorizedBit)) {
            delete colorized;
            colorized = NULL;
        }
        if (scaled && flag.testFlag(ScaledBit)) {
            delete scaled;
            scaled = NULL;
        }
        if (rotated && flag.testFlag(RotatedBit)) {
            delete rotated;
            rotated = NULL;
        }
    }

    QHash<int, QImage*> source;
    QImage *origin;
    QImage *colorized;
    QImage *scaled;
    QImage *rotated;

    QImage *prevTmp;
    qreal opacity;
    // -----------
    TileKey key;
    QPointer<QPropertyAnimation> ani;

public slots:
    void setOpacity(const qreal o) {
        if (qFuzzyCompare(opacity, o))
            return;
        opacity = o;
        emit needUpdate(key.x, key.y, key.z);
    }
    void removeTmp() {
        if (prevTmp) {
            delete prevTmp;
            prevTmp = NULL;
        }
    }

signals:
    void needUpdate(int, int, int);
public:
    qreal getOpacity() const { return opacity; }
};

// -------------------------------------------------------

class MapLayerTilePrivate : public MapLayerPrivate
{
    Q_OBJECT

public:
    explicit MapLayerTilePrivate(QObject *parent = 0);
    virtual ~MapLayerTilePrivate();

    /**
     * @brief flushTiles сохранение полученных тайлов в бд
     * @flag сохранить и продолжить/завершить работу
     */
    void flushTiles(bool flag = true);

    /**
     * @brief saveTileInCache сохраняем полученный тайл в локальный кэш
     * @param hash ключ-хэш тайла
     * @param type тип тайла
     * @param img тайл
     */
    void saveTileInCache(const TileKey &key, int type, const QImage &img, bool flag = true);

private:

    QList<int> missedTypes(const TileKey &key);
    Tile *generateTile(const TileKey &key);

    TileKey createImages(Tile *t);

    QImage createUploadTileThread(const TileKey &key);
    QImage createTmpTileThread(const TileKey &key);

    QImage createColorizedTileThread(Tile *t);
    QImage createScaledTileThread(QImage *colorized);
    QImage createRotatedTileThread(const TileKey &key, QImage *scaled);

    /**
     * @brief getCacheImage попытаться нарисовать тайл из кэша
     * @param painter паинтер
     * @param key ключ
     * @return список недостающих типов для текущего ключа
     */
    QList<int> getCacheImage(QPainter *painter, const TileKey &key);

    /**
     * @brief getdbImage создать запрос недостающих тайлов
     * @param types типы тайлов
     * @param key ключ
     */
    void getdbImage(const QList<int> types, const TileKey &key);

    /**
     * @brief createColorizedTile создать перекрашенный тайл
     * @param t
     */
    void createColorizedTile(Tile *t);

    /**
     * @brief createScaledTileDirect создать отмасштабированный тайл
     * @param t тайл
     */
    void createScaledTileDirect(Tile *t);

    /**
     * @brief createTransformTileDirect создать повернутый тайл
     * @param t тайл
     */
    void createTransformTileDirect(Tile *t);

    /**
     * @brief createUploadTile создать/обновить тайл с использованием верхних масштабов
     * @param t
     */
    bool createUploadTile(Tile *t);

    /**
     * @brief createTmpTile создаем тайл с предыдущего мастшаба (перед вызовом надо удалить t->origin)
     * @param t текущий тайл
     */
    void createTmpTile(Tile *t);

    /**
     * @brief changeZoom реагируем на изменение масштаба
     */
    void changeZoom(qreal);

    /**
     * @brief calcVisualRect пересчитать необходимые ключи
     */
    void calcVisualRect();

    /**
     * @brief updateLowerTiles
     * @param t
     */
    void updateLowerTiles(Tile *t);

    /**
     * @brief localDraw нарисовать тайл
     * @param painter паинтер
     * @param t тайл
     */
    void localDraw(QPainter *painter, Tile *t);

public Q_SLOTS:
    /**
     * @brief render отрисовка подложки
     * @param painter паинтер
     * @param rgn область отрисовки
     * @param camera камера
     */
    virtual void render(QPainter *painter, const QRectF &rgn, const MapCamera *camera, MapOptions options = optNone);

    /**
     * @brief localUpdate посылает сигнал на перерисовку тайла
     * @param x
     * @param y
     * @param z
     */
    void localUpdate(int x, int y, int z);

    /**
     * @brief loaderError сервер не смог обработать запрос
     */
    void loaderError(int, int, int, int);

Q_SIGNALS:
    void tileIncome(const TileKey &key, bool empty, bool fromdb = false);

protected Q_SLOTS:
    /**
     * @brief onDb_TileIncome пришел ответ на получение тайлов из бд
     * @param query идентификатор
     * @param result результат
     * @param error ошибки
     */
    void onDb_TileIncome(uint query, QVariant result, QVariant error);
    /**
     * @brief onDb_TileSave пришел ответ на сохранение тайлов в бд
     * @param query идентификатор
     * @param result результат
     * @param error ошибки
     */
    void onDb_TileSave(uint query, QVariant result, QVariant error);

    /**
     * @brief onDb_EndTile отложенное закрытие базы
     * @param query
     * @param result
     * @param error
     */
    void onDb_EndTile(uint query, QVariant result, QVariant error);

    void incomeFuture();

protected:
    void timerEvent(QTimerEvent *);

public:
    static const int MaxCacheTile     = 100;     // максимальный размер кэша тайлов
    static const int MaxUIntCacheSize = 128;     // максимальный размер кэша тайлов

    ConvertColor::ColorFilterFunc func;          // функция изменения гаммы подложки
    QVariantMap graphOpt;                        // опции изменения гаммы

    int prevZoom;                                // предыдущий zoom
    int zoom;                                    // zoom подложки
    int base;                                    // 2^zoom
    qreal cameraScale;                           // предыдущее значение scale камеры
    bool scaleChanged;                           // scale изменился
    qreal cameraAngle;                           // предыдущее значение поворота камеры
    bool angleChanged;                           // камеру повернули
    QHash<quint64, Tile*> tiles;                 // кэш подложки
    QList<quint64> tileHistory;                  // история использования тайлов
    QList<int> types;                            // активные типы загрузчиков

    QSet<quint64> askCache;                      // кэш отправленных запросов на сервер
    QSet<quint64> dbCache;                       // кэш отправленных запросов в базу данных

    QSet<quint64> dbEmptyCache;                  // кэш отсутсвующий тайлов в базе
    QSet<quint64> loadersErrorsCache;            // кэш ошибок на сервере

    QBasicTimer cacheTimer;                      // таймер для очистки кэша ошибок
    static const int errorClearTime = 60000;     // время очистки кэша ошибок

    QHash<quint8, MapTileLoader*> loaders;       // полные перечень доступных загрузчиков

    int levelUp;                                 // уровень тайлов
    //-----------------------------------------------
    dc::DatabaseController *dc;                  // ассинхронная БД
    TilesDB *tileDB;                             // контроллер ассинхронной БД. напрямую не вызывается.

    static const int maxQueueSize = 5;           // максимальная очередь тайлов
    dc::QueryResult *queueTiles;                 // очередь тайлов на сохранение в бд
    QBasicTimer queueTimer;                      // таймер для сохранение в бд
    static const int FlushInterval = 30000;      // инетрвал сохранений

    QMutex keyMutex;
    QMutex mutex;

    int futureCount;
    QWaitCondition wait;
    //-----------------------------------------------

    int tilePixSize;                             // размер отмаштабированного тайла в пикселях
    QTransform tileTr;                           // матрица поворота тайла
    QTransform tileBackTr;                       // обратная матрица

    QRect visionTileRect;

    //-----------------------------------------------

    bool smoothing;

    //-----------------------------------------------
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

Q_DECLARE_OPERATORS_FOR_FLAGS(minigis::Tile::ClearBits)

// -------------------------------------------------------

template<>
class QTypeInfo<minigis::Tile>
{
public:
    enum {
        isComplex = true,
        isStatic = true,
        isLarge = true,
        isPointer = true,
        isDummy = true
    };
    static inline const char *name() { return "minigis::Tile"; }
};

// -------------------------------------------------------


#endif // MAPLAYERTILE_P_H
