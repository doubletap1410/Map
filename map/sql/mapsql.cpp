#include <QDebug>
#include <QByteArray>
#include <QStringList>
#include <QDateTime>
#include <QPointer>
#include <QUuid>
#include <QCryptographicHash>
#include <QDateTime>

#include <db/databasecontroller.h>

//#include "core/maphmatrix.h"
#include "coord/mapcoords.h"
#include "sql/mapsql.h"

// ==================================================================

#ifdef _ru
#undef _ru
#endif
#define _ru(x) QString::fromUtf8(x)

// ==================================================================

class TilesDBPrivate
{
public:
    TilesDBPrivate() : dc(NULL) {}

    dc::DatabaseController *dc;
};

// -----------------------------------------------------------------------------
TilesDB::TilesDB(QObject *parent)
    :QObject(parent), d_ptr(new TilesDBPrivate)
{
}

// -----------------------------------------------------------------------------
TilesDB::~TilesDB()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = NULL;
    }
}

// -----------------------------------------------------------------------------
void TilesDB::setDc(dc::DatabaseController *dc)
{
    Q_D(TilesDB);
    if (!dc)
        return;
    QPointer<QObject> handle(this);
    if (d->dc && d->dc != dc)
        d->dc->unregisterHandler(handle);
    d->dc = dc;

    dc->registerHandler("noop",     handle, "noopSlot");

    dc->registerHandler("create",   handle, "createTables");

    dc->registerHandler("insTiles", handle, "insertTiles");
    dc->registerHandler("selTiles", handle, "loadTiles");
    dc->registerHandler("selQuad" , handle, "loadQuadTile");
    dc->registerHandler("remTiles", handle, "removeTiles");
}

// -----------------------------------------------------------------------------
void TilesDB::noopSlot(QVariant /*params*/, QVariant &/*result*/, QVariant &/*errors*/)
{
}

// -----------------------------------------------------------------------------
void TilesDB::createTables(QVariant, QVariant &result, QVariant &errors)
{
    QVariantMap data;
    dc::QueryResult res;

    // -----------------------------------------------------------------------------

#if 0
    d_ptr->dc->execQuery(_ru(
                             "  select * from pg_catalog.pg_user; \n"
                             )
                         , data, res);
    qDebug() << data;
    qDebug() << res;


    d_ptr->dc->execQuery(_ru(
                             "  create table ttt ( id INTEGER ); \n"
                             )
                         , data, res);
    qDebug() << data;
    qDebug() << res;
#endif

    d_ptr->dc->execQuery(_ru(
                             "PRAGMA foreign_keys=ON;"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists TileBlob \n"
                             "  ( \n"
                             "      id TEXT PRIMARY KEY, /* идентификатор листа */ \n"
                             "      hash TEXT, /* Sha1 хэшсвёртка изображения для поиска дубликатов */ \n"
                             "      tile BLOB, /* собственно изображение */ \n"
                             "      UNIQUE (hash) \n"
                             "  );")
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "CREATE INDEX IF NOT EXISTS TileBlob_hash_index ON TileBlob(hash);"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists Tiles \n"
                             "  ( \n"
                             "      id TEXT PRIMARY KEY, /* ид плитки */ \n"
                             "      nx INTEGER, /* координаты плитки по X */ \n"
                             "      ny INTEGER,  /* координаты плитки по Y */ \n"
                             "      zoom INTEGER, /* масштаб или уровень зума */ \n"
                             "      type INTEGER, /* тип плитки */ \n"
                             "      quadkey TEXT, /* идентификатор листа */ \n"
                             "      inserttime INTEGER, /* время записи листа */ \n"
                             "      expires INTEGER, /* срок годности листа */ \n"
                             "      tile TEXT, /* ид изображения */ \n"
                             "      FOREIGN KEY (tile) REFERENCES TileBlob(id) ON UPDATE CASCADE ON DELETE CASCADE, \n"
                             "      UNIQUE (nx, ny, zoom, type) /* тип, зум, X, Y - обеспечат уникальность плиток */ \n"
                             "  );")
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "CREATE INDEX IF NOT EXISTS Tiles_tile_index ON Tiles(quadkey);"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "CREATE INDEX IF NOT EXISTS Tiles_xyz_index ON Tiles(nx, ny, zoom, type);"
                             )
                         , data, res);



    // --------------------------------------------------------

    result.setValue(res);
    errors.clear();
}

// -----------------------------------------------------------------------------
void TilesDB::insertTiles(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<dc::QueryResult>()) {
//        LogE("SQL") << "Failed read incomming data";
        errors = false;
        return;
    }

    d_ptr->dc->transaction();

    dc::QueryResult res;
    dc::QueryResult tiles = params.value<dc::QueryResult>();
    uint dt = QDateTime::currentDateTime().toTime_t();

    foreach (const QVariantMap &tile, tiles) {
        QString key = QUuid::createUuid().toString();
        int x = tile.value("x").toInt();
        int y = tile.value("y").toInt();
        int z = tile.value("z").toInt();
        QString q = minigis::TileSystem::tileToQuadKey(x, y, z);
        QByteArray blob = tile.value("tile").toByteArray();
        QString hash(QCryptographicHash::hash(blob, QCryptographicHash::Sha1).toHex());

        QVariantMap data;

        data[":HASH"] = hash;
        d_ptr->dc->execQuery("SELECT id FROM TileBlob WHERE hash = :HASH; ", data, res);

        bool tileExist = !res.isEmpty();
        QString tileid = tileExist ? res.first().value("id").toString() : key;

        if (!tileExist) {
            data.clear();
            data[":I"] = tileid;
            data[":T"] = blob;
            data[":H"] = hash;
            d_ptr->dc->execQuery(
                        "INSERT INTO TileBlob (id, hash, tile) VALUES (:I, :H, :T); ",
                        data, res);
        }

        data.clear();
        data[":I"   ] = key;
        data[":X"   ] = x;
        data[":Y"   ] = y;
        data[":Z"   ] = z;
        data[":Q"   ] = q;
        data[":TYPE"] = tile.value("type");
        data[":EXP" ] = tile.value("expires");
        data[":DT"  ] = dt;
        data[":TILE"] = tileid;

        d_ptr->dc->execQuery(
                    "INSERT OR REPLACE INTO Tiles (id, nx, ny, zoom, type, quadkey, inserttime, expires, tile) "
                    "VALUES (:I, :X, :Y, :Z, :TYPE, :Q, :DT, :EXP, :TILE); ",
                    data, res);


    }

    d_ptr->dc->commit();

    result.setValue(tiles);
    errors.clear();
}

// -----------------------------------------------------------------------------
void TilesDB::loadTiles(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QVariantMap>()) {
//        qDebug() << "Failed read incomming data";
        errors = false;
        return;
    }

    d_ptr->dc->transaction();

    dc::QueryResult res;

    dc::QueryResult tmpRes;
    QVariantMap data;

    QVariantMap p = params.value<QVariantMap>();
    int maxShift = p.value("level").toInt();
    int shift = 0;

    QStringList types = p.value("type").toStringList();
    while (shift <= maxShift) {

        int x = p.value("x").toInt() >> shift;
        int y = p.value("y").toInt() >> shift;
        int z = p.value("z").toInt() - shift;

        data[":X"] = x;
        data[":Y"] = y;
        data[":Z"] = z;

        QString typesSum = types.join(",");
        d_ptr->dc->execQuery(QString(
            "SELECT t.id, b.tile, t.type, t.expires FROM Tiles AS t "
            "INNER JOIN TileBlob AS b ON t.tile = b.id "
            "WHERE t.nx = :X AND t.ny = :Y AND t.zoom = :Z AND t.type in (%1); "
            ).arg(typesSum)
            , data, tmpRes);

        QVariantMap xyz;
        xyz["x"] = x;
        xyz["y"] = y;
        xyz["z"] = z;
        for (QMutableListIterator<QVariantMap> it(tmpRes); it.hasNext(); ) {
            QVariantMap &v = it.next();
            v.unite(xyz);
            types.removeAll(v.value("type").toString());
        }
        foreach (QString const &t, types) {
            QVariantMap v = xyz;
            v["type"] = t;
            tmpRes.append(v);
        }

        types.clear();
        for (QListIterator<QVariantMap> it(tmpRes); it.hasNext(); ) {
            QVariantMap v = it.next();
            res.append(v);

            QByteArray b = v.value("tile").toByteArray();
            if (b.isNull())
                types.append(v.value("type").toString());
        }

        if (types.isEmpty())
            break;

        ++shift;
    }

    d_ptr->dc->commit();

    result.setValue(res);
    errors.clear();
}
// -----------------------------------------------------------------------------

void TilesDB::loadQuadTile(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QVariantMap>()) {
//        qDebug() << "Failed read incomming data";
        errors = false;
        return;
    }
    dc::QueryResult res;
    QVariantMap data;

    d_ptr->dc->transaction();

    QVariantMap p = params.value<QVariantMap>();
    data[":X"   ] = p.value("x");
    data[":Y"   ] = p.value("y");
    data[":Z"   ] = p.value("z");
    data[":TYPE"] = p.value("type");
    d_ptr->dc->execQuery(
                "SELECT tile FROM Tiles "
                "WHERE nx = :X AND ny = :Y AND zoom = :Z AND type = :TYPE; "
                , data, res);

    QVariant tile;
    if (!res.isEmpty())
        tile = res.first()["tile"];

    data.clear();

    QString quad = p.value("quad").toString();
    int minsize = quad.size();
    data[":KEY"  ] = quad + "%";
    data[":TYPE" ] = p.value("type");
    data[":MIN"  ] = minsize;
    data[":MAX"  ] = minsize + 6;
    data[":SS"   ] = minsize + 1;
    d_ptr->dc->execQuery(
                "SELECT substr(quadkey, :SS) as key "
                "FROM Tiles "
                "WHERE type = :TYPE AND quadkey LIKE :KEY "
                "    AND length(quadkey) BETWEEN :MIN AND :MAX "
                "ORDER BY length(quadkey);"
                , data, res);

    d_ptr->dc->commit();

    QStringList list;
    for (QListIterator<QVariantMap> it(res); it.hasNext();) {
        QVariantMap v = it.next();
        list.append(v["key"].toString());
    }
    QVariantMap vm;
    vm["tile" ] = tile;
    vm["quads"] = list;
    vm["quad" ] = quad;
    result.setValue(vm);
    errors.clear();
}

void TilesDB::removeTiles(QVariant params, QVariant &/*result*/, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QStringList>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QStringList tiles = params.toStringList();

    d_ptr->dc->execQuery(
                QString("DELETE FROM Tiles "
                "WHERE id IN (%1); ").arg(QString("\'%1\'").arg(tiles.join("\', \'"))),
                QVariantMap(), res);
    // FIXME: возможны висячие части подложек в TileBlob
}

// ==================================================================
#if 0

class HMatrixDBPrivate
{
public:
    HMatrixDBPrivate() : dc(NULL) {}

    dc::DatabaseController *dc;
};

// -----------------------------------------------------------------------------
HMatrixDB::HMatrixDB(QObject *parent)
    :QObject(parent), d_ptr(new HMatrixDBPrivate)
{
}

// -----------------------------------------------------------------------------
HMatrixDB::~HMatrixDB()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = NULL;
    }
}

// -----------------------------------------------------------------------------
void HMatrixDB::setDc(dc::DatabaseController *dc)
{
    Q_D(HMatrixDB);
    if (!dc)
        return;
    QPointer<QObject> handle(this);
    if (d->dc && d->dc != dc)
        d->dc->unregisterHandler(handle);
    d->dc = dc;

    dc->registerHandler("noop",     handle, "noopSlot");

    dc->registerHandler("create",   handle, "createTables");

    dc->registerHandler("saveMatrix" , handle, "saveHMatrix");
    dc->registerHandler("loadMatrix" , handle, "loadHMatrix");
    dc->registerHandler("clearMatrix", handle, "clearHMatrix");
}

// -----------------------------------------------------------------------------
void HMatrixDB::noopSlot(QVariant /*params*/, QVariant &/*result*/, QVariant &/*errors*/)
{
}

void HMatrixDB::createTables(QVariant, QVariant &result, QVariant &errors)
{
    QVariantMap data;
    dc::QueryResult res;

#if 0
    // -----------------------------------------------------------------------------
    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHPoints \n"
                             "  ( \n"
                             "      id INTEGER PRIMARY KEY, /* ид точки */ \n"
                             "      x REAL, /* координата х */ \n"
                             "      y REAL, /* координата y */ \n"
                             "      h REAL /* высота */ \n"
                             "  ); \n"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHTriangles \n"
                             "  ( \n"
                             "      id INTEGER PRIMARY KEY, /* ид треугольника */ \n"
                             "      p1 INTEGER, /* ид 1 вершины */ \n"
                             "      p2 INTEGER, /* ид 2 вершины */ \n"
                             "      p3 INTEGER, /* ид 3 вершины */ \n"
                             "      t1 INTEGER, /* ид 1 треугольника */ \n"
                             "      t2 INTEGER, /* ид 2 треугольника */ \n"
                             "      t3 INTEGER /* ид 3 треугольника */ \n"
                             "  ); \n"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHConvexHull \n"
                             "  ( \n"
                             "      id INTEGER, /* ид выпуклой оболочки */ \n"
                             "      point_id INTEGER /* ид точки */ \n"
                             "  ); \n"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHCache \n"
                             "  ( \n"
                             "      x INTEGER, /* индекс по горизонтали */ \n"
                             "      y INTEGER, /* индекс по вертикали */ \n"
                             "      t INTEGER, /* ид треугольника */ \n"
                             "      dist REAL, /* расстояние */ \n"
                             "      UNIQUE (x, y) \n"
                             "  ); \n"
                             )
                         , data, res);

    // -----------------------------------------------------------------------------
#endif

    result.setValue(res);
    errors.clear();
}

void HMatrixDB::saveHMatrix(QVariant params, QVariant &result, QVariant &errors)
{
    Q_ASSERT(d_ptr->dc);

    if (params.isNull() || !params.canConvert<QVariantMap>()) {
//        LogE("SQL") << "Failed read incomming data";
        errors = false;
        return;
    }

    QTime t;
    t.start();
    dc::QueryResult res;
    QVariantMap storage = params.value<QVariantMap>();

    dc::DbTransactor transactor(*d_ptr->dc);
    {
        QVariantMap data;
        d_ptr->dc->execQuery(_ru("DELETE FROM MapHPoints ; "), data, res);
        d_ptr->dc->execQuery(_ru("DELETE FROM MapHTriangles ; "), data, res);
        d_ptr->dc->execQuery(_ru("DELETE FROM MapHConvexHull ; "), data, res);
        d_ptr->dc->execQuery(_ru("DELETE FROM MapHCache ; "), data, res);
    }

    QList<minigis::PointHPtr> points = storage.value("points").value<QList<minigis::PointHPtr> >();
    foreach (minigis::PointHPtr p, points) {
        QVariantMap data;
        data[":ID"] = p->id();
        data[":X" ] = p->x();
        data[":Y" ] = p->y();
        data[":H" ] = p->h();
        d_ptr->dc->execQuery("INSERT INTO MapHPoints (id, x, y, h) "
                             "VALUES (:ID, :X, :Y, :H); ",
                             data, res);
    }    

    QList<minigis::TriangleHPtr> triangles = storage.value("triangles").value<QList<minigis::TriangleHPtr> >();
    foreach (minigis::TriangleHPtr t, triangles) {
        QVariantMap data;
        data[":ID"] = t->id();
        data[":P1"] = t->nodes[0]->id();
        data[":P2"] = t->nodes[1]->id();
        data[":P3"] = t->nodes[2]->id();
        data[":T1"] = t->triangles[0] ? t->triangles[0]->id() : -1;
        data[":T2"] = t->triangles[1] ? t->triangles[1]->id() : -1;
        data[":T3"] = t->triangles[2] ? t->triangles[2]->id() : -1;
        d_ptr->dc->execQuery("INSERT INTO MapHTriangles (id, p1, p2, p3, t1, t2, t3) "
                             "VALUES (:ID, :P1, :P2, :P3, :T1, :T2, :T3); ",
                             data, res);
    }

    QList<minigis::PointHPtr> hull = storage.value("hull").value<QList<minigis::PointHPtr> >();
    foreach (minigis::PointHPtr p, hull) {
        QVariantMap data;
        data[":ID"] = 1;
        data[":PID" ] = p->id();
        d_ptr->dc->execQuery("INSERT INTO MapHConvexHull (id, point_id) "
                             "VALUES (:ID, :PID); ",
                             data, res);
    }

    minigis::HMatrixCache cache = storage.value("cache").value<minigis::HMatrixCache>();
    for (int i = 0; i < cache.size(); ++i) {
        for (int j = 0; j < cache.at(i).size(); ++j) {
            QVariantMap data;
            data[":X"   ] = i;
            data[":Y"   ] = j;
            data[":T"   ] = cache[i][j].first->id();
            data[":DIST"] = cache[i][j].second;
            d_ptr->dc->execQuery("INSERT INTO MapHCache (x, y, t, dist) "
                                 "VALUES (:X, :Y, :T, :DIST); ",
                                 data, res);
        }
    }
    transactor.commit();

    qDebug() << "Save time" << t.elapsed();
    result.setValue(storage);
    errors.clear();
}

void HMatrixDB::loadHMatrix(QVariant /*params*/, QVariant &result, QVariant &errors)
{
    QTime t;
    t.start();

    QVariantMap data;
    dc::QueryResult res;

    d_ptr->dc->execQuery(
                "SELECT id as ID, x as X, y as Y, h as H FROM MapHPoints ; "
                , data, res);

    QList<minigis::PointHPtr> points;
    points.reserve(res.size());
    foreach (QVariantMap vm, res)
        points.append(
                    minigis::PointHPtr(
                          new minigis::PointH(
                              vm.value("X").toReal(), vm.value("Y").toReal(), vm.value("H").toReal(), vm.value("ID").toInt())
                          )
                      );

    d_ptr->dc->execQuery(
                "SELECT id as ID, p1 as P1, p2 as P2, p3 as P3, t1 as T1, t2 as T2, t3 as T3 FROM MapHTriangles ; "
                , data, res);

    QMap<int, minigis::TriangleHPtr> trianglesMap;
    foreach (QVariantMap vm, res) {
        int id = vm.value("ID").toInt();
        trianglesMap.insert(id, minigis::TriangleHPtr(
                                new minigis::TriangleH(points.at(vm.value("P1").toInt()), points.at(vm.value("P2").toInt()), points.at(vm.value("P3").toInt()), id)
                                )
                            );
    }

    QList<minigis::TriangleHPtr> triangles;
    triangles.reserve(res.size());
    foreach (QVariantMap vm, res) {
        int key = vm.value("ID").toInt();
        minigis::TriangleHPtr current = trianglesMap.value(key);

        int ind = vm.value("T1").toInt();
        if (ind != -1)
            current->triangles[0] = trianglesMap.value(ind);
        ind = vm.value("T2").toInt();
        if (ind != -1)
            current->triangles[1] = trianglesMap.value(ind);
        ind = vm.value("T3").toInt();
        if (ind != -1)
            current->triangles[2] = trianglesMap.value(ind);
        triangles.append(current);
    }

    d_ptr->dc->execQuery(
                "SELECT id as ID, point_id as P FROM MapHConvexHull ; "
                , data, res);

    QList<minigis::PointHPtr> hull;
    hull.reserve(res.size());
    // TODO: unused id
    foreach (QVariantMap vm, res)
        hull.append(points.at(vm.value("P").toInt()));

    d_ptr->dc->execQuery("SELECT MAX(x) as X FROM MapHCache ; ", data, res);
    int gridSize = -1;
    if (!res.isEmpty())
        gridSize = res.first().value("X").toInt() + 1;

    minigis::HMatrixCache cache;
    if (gridSize != -1) {
        for (int i = 0; i < gridSize; ++i) {
            QList<QPair<minigis::TriangleHPtr, qreal> > list;
            for (int j = 0; j < gridSize; ++j)
                list.append(qMakePair(minigis::TriangleHPtr(new minigis::TriangleH()), qreal(0)));
            cache.append(list);
        }

        d_ptr->dc->execQuery(
                    "SELECT x as X, y as Y, y as T, dist as DIST FROM MapHCache ; "
                    , data, res);

        foreach (QVariantMap vm, res)
            cache[vm.value("X").toInt()][vm.value("Y").toInt()] = qMakePair(trianglesMap.value(vm.value("T").toInt()), vm.value("DIST").toReal());
    }
    QVariantMap vm;
    vm["points"].setValue(points);
    vm["triangles"].setValue(triangles);
    vm["hull"].setValue(hull);
    vm["cache"].setValue(cache);

    qDebug() << "Load time" << t.elapsed();

    result.setValue(vm);
    errors.clear();
}

void HMatrixDB::clearHMatrix(QVariant /*params*/, QVariant &result, QVariant &errors)
{
    Q_ASSERT(d_ptr->dc);

    dc::QueryResult res;
    QVariantMap data;

    dc::DbTransactor transactor(*d_ptr->dc);

    d_ptr->dc->execQuery(_ru("DELETE FROM MapHPoints ; "), data, res);
    d_ptr->dc->execQuery(_ru("DELETE FROM MapHTriangles ; "), data, res);
    d_ptr->dc->execQuery(_ru("DELETE FROM MapHConvexHull ; "), data, res);
    d_ptr->dc->execQuery(_ru("DELETE FROM MapHCache ; "), data, res);

    transactor.commit();

    result.clear();
    errors.clear();
}

// ==================================================================

class HMultiMatrixDBPrivate
{
public:
    HMultiMatrixDBPrivate() : dc(NULL) {}

    dc::DatabaseController *dc;
};

// -----------------------------------------------------------------------------
HMultiMatrixDB::HMultiMatrixDB(QObject *parent)
    :QObject(parent), d_ptr(new HMultiMatrixDBPrivate)
{
}

// -----------------------------------------------------------------------------
HMultiMatrixDB::~HMultiMatrixDB()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = NULL;
    }
}

// -----------------------------------------------------------------------------
void HMultiMatrixDB::setDc(dc::DatabaseController *dc)
{
    Q_D(HMultiMatrixDB);
    if (!dc)
        return;
    QPointer<QObject> handle(this);
    if (d->dc && d->dc != dc)
        d->dc->unregisterHandler(handle);
    d->dc = dc;

    dc->registerHandler("noop",     handle, "noopSlot");

    dc->registerHandler("create",   handle, "createTables");

    dc->registerHandler("saveMatrix" , handle, "saveHMatrix");
    dc->registerHandler("loadMatrix" , handle, "loadHMatrix");
    dc->registerHandler("clearMatrix", handle, "clearHMatrix");
}

// -----------------------------------------------------------------------------
void HMultiMatrixDB::noopSlot(QVariant /*params*/, QVariant &/*result*/, QVariant &/*errors*/)
{
}

void HMultiMatrixDB::createTables(QVariant, QVariant &result, QVariant &errors)
{
    QVariantMap data;
    dc::QueryResult res;

    // -----------------------------------------------------------------------------

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHPoints \n"
                             "  ( \n"
                             "      id INTEGER NOT NULL, /* ид матрицы */ \n"
                             "      point_id INTEGER NOT NULL, /* ид точки */ \n"
                             "      x REAL, /* координата х */ \n"
                             "      y REAL, /* координата y */ \n"
                             "      h REAL, /* высота */ \n"
                             "      UNIQUE (id, point_id) \n"
                             "  ); \n"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHTriangles \n"
                             "  ( \n"
                             "      id INTEGER NOT NULL, /* ид матрицы */ \n"
                             "      triangle_id INTEGER NOT NULL, /* ид треугольника */ \n"
                             "      p1 INTEGER, /* ид 1 вершины */ \n"
                             "      p2 INTEGER, /* ид 2 вершины */ \n"
                             "      p3 INTEGER, /* ид 3 вершины */ \n"
                             "      t1 INTEGER, /* ид 1 треугольника */ \n"
                             "      t2 INTEGER, /* ид 2 треугольника */ \n"
                             "      t3 INTEGER /* ид 3 треугольника */ \n"
                             "      UNIQUE (id, triangle_id), \n"
                             "      FOREIGN KEY (id) REFERENCES MapHPoints(id) ON DELETE CASCADE \n"
                             "  ); \n"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHConvexHull \n"
                             "  ( \n"
                             "      id INTEGER NOT NULL, /* ид матрицы */ \n"
                             "      point_id INTEGER NOT NULL, /* ид точки */ \n"
                             "      UNIQUE (id, point_id), \n"
                             "      FOREIGN KEY (id) REFERENCES MapHPoints(id) ON DELETE CASCADE \n"
                             "  ); \n"
                             )
                         , data, res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists MapHCache \n"
                             "  ( \n"
                             "      id INTEGER NOT NULL, /* ид матрицы */ \n"
                             "      x INTEGER, /* индекс по горизонтали */ \n"
                             "      y INTEGER, /* индекс по вертикали */ \n"
                             "      t INTEGER, /* ид треугольника */ \n"
                             "      dist REAL, /* расстояние */ \n"
                             "      UNIQUE (id, x, y), \n"
                             "      FOREIGN KEY (id) REFERENCES MapHPoints(id) ON DELETE CASCADE \n"
                             "  ); \n"
                             )
                         , data, res);

    // -----------------------------------------------------------------------------

    result.setValue(res);
    errors.clear();
}

void HMultiMatrixDB::saveHMatrix(QVariant params, QVariant &result, QVariant &errors)
{
    Q_ASSERT(d_ptr->dc);

    if (params.isNull() || !params.canConvert<QVariantMap>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QVariantMap storage = params.value<QVariantMap>();

    int id = storage.value("id").toInt();

    dc::DbTransactor transactor(*d_ptr->dc);
    {
        QVariantMap data;
        data[":NUMBER"] = id;
        d_ptr->dc->execQuery(_ru("DELETE FROM MapHPoints WHERE id = :NUMBER; "), data, res);
//        d_ptr->dc->execQuery(_ru("DELETE FROM MapHTriangles WHERE id = :NUMBER; "), data, res);
//        d_ptr->dc->execQuery(_ru("DELETE FROM MapHConvexHull WHERE id = :NUMBER; "), data, res);
//        d_ptr->dc->execQuery(_ru("DELETE FROM MapHCache WHERE id = :NUMBER; "), data, res);
    }

    QList<minigis::PointHPtr> points = storage.value("points").value<QList<minigis::PointHPtr> >();
    foreach (minigis::PointHPtr p, points) {
        QVariantMap data;
        data[":NUMBER"] = id;
        data[":ID"] = p->id();
        data[":X" ] = p->x();
        data[":Y" ] = p->y();
        data[":H" ] = p->h();
        d_ptr->dc->execQuery("INSERT INTO MapHPoints (id, point_id, x, y, h) "
                             "VALUES (:NUMBER, :ID, :X, :Y, :H); ",
                             data, res);
    }

    QList<minigis::TriangleHPtr> triangles = storage.value("triangles").value<QList<minigis::TriangleHPtr> >();
    foreach (minigis::TriangleHPtr t, triangles) {
        QVariantMap data;
        data[":NUMBER"] = id;
        data[":ID"] = t->id();
        data[":P1"] = t->nodes[0]->id();
        data[":P2"] = t->nodes[1]->id();
        data[":P3"] = t->nodes[2]->id();
        data[":T1"] = t->triangles[0] ? t->triangles[0]->id() : -1;
        data[":T2"] = t->triangles[1] ? t->triangles[1]->id() : -1;
        data[":T3"] = t->triangles[2] ? t->triangles[2]->id() : -1;
        d_ptr->dc->execQuery("INSERT INTO MapHTriangles (id, triangle_id, p1, p2, p3, t1, t2, t3) "
                             "VALUES (:NUMBER, :ID, :P1, :P2, :P3, :T1, :T2, :T3); ",
                             data, res);
    }

    QList<minigis::PointHPtr> hull = storage.value("hull").value<QList<minigis::PointHPtr> >();
    foreach (minigis::PointHPtr p, hull) {
        QVariantMap data;
        data[":NUMBER"] = id;
        data[":ID" ] = p->id();
        d_ptr->dc->execQuery("INSERT INTO MapHConvexHull (id, point_id) "
                             "VALUES (:NUMBER, :ID); ",
                             data, res);
    }

    minigis::HMatrixCache cache = storage.value("cache").value<minigis::HMatrixCache>();
    for (int i = 0; i < cache.size(); ++i) {
        for (int j = 0; j < cache.at(i).size(); ++j) {
            QVariantMap data;
            data[":NUMBER"] = id;
            data[":X"   ] = i;
            data[":Y"   ] = j;
            data[":T"   ] = cache[i][j].first->id();
            data[":DIST"] = cache[i][j].second;
            d_ptr->dc->execQuery("INSERT INTO MapHCache (id, x, y, t, dist) "
                                 "VALUES (:NUMBER, :X, :Y, :T, :DIST); ",
                                 data, res);
        }
    }
    transactor.commit();

    result.setValue(storage);
    errors.clear();
}

void HMultiMatrixDB::loadHMatrix(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<int>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QVariantMap data;

    int id = params.toInt();
    data[":NUMBER"] = id;

    d_ptr->dc->execQuery(
                "SELECT point_id as ID, x as X, y as Y, h as H FROM MapHPoints WHERE id = :NUMBER; "
                , data, res);

    QList<minigis::PointHPtr> points;
    points.reserve(res.size());
    foreach (QVariantMap vm, res)
        points.append(
                    minigis::PointHPtr(
                          new minigis::PointH(
                              vm.value("X").toReal(), vm.value("Y").toReal(), vm.value("H").toReal(), vm.value("ID").toInt())
                          )
                      );

    d_ptr->dc->execQuery(
                "SELECT trianlge_id as ID, p1 as P1, p2 as P2, p3 as P3, t1 as T1, t2 as T2, t3 as T3 FROM MapHTriangles WHERE id = :NUMBER; "
                , data, res);

    QMap<int, minigis::TriangleHPtr> trianglesMap;
    foreach (QVariantMap vm, res) {
        int id = vm.value("ID").toInt();
        trianglesMap.insert(id, minigis::TriangleHPtr(
                                new minigis::TriangleH(points.at(vm.value("P1").toInt()), points.at(vm.value("P2").toInt()), points.at(vm.value("P3").toInt()), id)
                                )
                            );
    }

    QList<minigis::TriangleHPtr> triangles;
    triangles.reserve(res.size());
    foreach (QVariantMap vm, res) {
        int key = vm.value("ID").toInt();
        minigis::TriangleHPtr current = trianglesMap.value(key);

        int ind = vm.value("T1").toInt();
        if (ind != -1)
            current->triangles[0] = trianglesMap.value(ind);
        ind = vm.value("T2").toInt();
        if (ind != -1)
            current->triangles[1] = trianglesMap.value(ind);
        ind = vm.value("T3").toInt();
        if (ind != -1)
            current->triangles[2] = trianglesMap.value(ind);
        triangles.append(current);
    }

    d_ptr->dc->execQuery(
                "SELECT id as ID, point_id as P FROM MapHConvexHull WHERE id = :NUMBER; "
                , data, res);

    QList<minigis::PointHPtr> hull;
    hull.reserve(res.size());
    // TODO: unused id
    foreach (QVariantMap vm, res)
        hull.append(points.at(vm.value("P").toInt()));

    d_ptr->dc->execQuery("SELECT MAX(x) as X FROM MapHCache WHERE id = :NUMBER; ", data, res);
    int gridSize = -1;
    if (!res.isEmpty())
        gridSize = res.first().value("X").toInt() + 1;

    minigis::HMatrixCache cache;
    if (gridSize != -1) {
        for (int i = 0; i < gridSize; ++i) {
            QList<QPair<minigis::TriangleHPtr, qreal> > list;
            for (int j = 0; j < gridSize; ++j)
                list.append(qMakePair(minigis::TriangleHPtr(new minigis::TriangleH()), qreal(0)));
            cache.append(list);
        }

        d_ptr->dc->execQuery(
                    "SELECT x as X, y as Y, y as T, dist as DIST FROM MapHCache WHERE id = :NUMBER; "
                    , data, res);

        foreach (QVariantMap vm, res)
            cache[vm.value("X").toInt()][vm.value("Y").toInt()] = qMakePair(trianglesMap.value(vm.value("T").toInt()), vm.value("DIST").toReal());
    }
    QVariantMap vm;
    vm["id"].setValue(id);
    vm["points"].setValue(points);
    vm["triangles"].setValue(triangles);
    vm["hull"].setValue(hull);
    vm["cache"].setValue(cache);

    result.setValue(vm);
    errors.clear();
}

void HMultiMatrixDB::idHMatrix(QVariant /*params*/, QVariant &result, QVariant &errors)
{
    dc::QueryResult res;
    QVariantMap data;

    d_ptr->dc->execQuery(_ru("SELECT DISTINCT id as NUMBER FROM MapHPoints; "), data, res);

    QQueue<int> numbers;
    for (QMutableListIterator<QVariantMap> it(res); it.hasNext(); ) {
        QVariantMap v = it.next();
        numbers.enqueue(v.value("NUMBER").toInt());
    }
    result.setValue(QVariant::fromValue<QQueue<int> >(numbers));
    errors.clear();
}

void HMultiMatrixDB::clearHMatrix(QVariant /*params*/, QVariant &result, QVariant &errors)
{
    Q_ASSERT(d_ptr->dc);

    dc::QueryResult res;
    QVariantMap data;
    data[":NUMBER"] = result.toInt();

    dc::DbTransactor transactor(*d_ptr->dc);

    d_ptr->dc->execQuery(_ru("DELETE FROM MapHPoints WHERE id = :NUMBER; "), data, res);
//    d_ptr->dc->execQuery(_ru("DELETE FROM MapHTriangles WHERE id = :NUMBER; "), data, res);
//    d_ptr->dc->execQuery(_ru("DELETE FROM MapHConvexHull WHERE id = :NUMBER; "), data, res);
//    d_ptr->dc->execQuery(_ru("DELETE FROM MapHCache WHERE id = :NUMBER; "), data, res);

    transactor.commit();

    result.clear();
    errors.clear();
}
#endif
// ==================================================================

class SearchesDBPrivate
{
public:
    SearchesDBPrivate() : dc(NULL) {}

    dc::DatabaseController *dc;
};

// -----------------------------------------------------------------------------
SearchesDB::SearchesDB(QObject *parent)
    :QObject(parent), d_ptr(new SearchesDBPrivate)
{
}

// -----------------------------------------------------------------------------
SearchesDB::~SearchesDB()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = NULL;
    }
}

// -----------------------------------------------------------------------------
void SearchesDB::setDc(dc::DatabaseController *dc)
{
    Q_D(SearchesDB);
    if (!dc)
        return;
    QPointer<QObject> handle(this);
    if (d->dc && d->dc != dc)
        d->dc->unregisterHandler(handle);
    d->dc = dc;

    dc->registerHandler("noop",         handle, "noopSlot");
    dc->registerHandler("create",       handle, "createTables");
    dc->registerHandler("insHistory",   handle, "insertHistory");
    dc->registerHandler("selHistory",   handle, "loadHistory");
    dc->registerHandler("insFavorite" , handle, "insertFavorite");
    dc->registerHandler("remFavorite",  handle, "removeFavorite");
    dc->registerHandler("selFavorite",  handle, "loadFavorites");
}

// -----------------------------------------------------------------------------
void SearchesDB::noopSlot(QVariant /*params*/, QVariant &/*result*/, QVariant &/*errors*/)
{
}

// -----------------------------------------------------------------------------
void SearchesDB::createTables(QVariant, QVariant &result, QVariant &errors)
{
    dc::QueryResult res;

// -----------------------------------------------------------------------------
    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists History \n"
                             "  ( \n"
                             "      request TEXT PRIMARY KEY, /* текста запроса */ \n"
                             "      inserttime INTEGER        /* время запроса */ \n"
                             "  ); \n")
                         , QVariantMap(), res);

    d_ptr->dc->execQuery(_ru(
                             "  CREATE TABLE IF NOT Exists Favorite \n"
                             "  ( \n"
                             "      id TEXT PRIMARY KEY,    /* идентификатор записи */ \n"
                             "      title TEXT,             /* заголовок */ \n"
                             "      description TEXT,       /* подпись */ \n"
                             "      x  INTEGER,             /* координата X местоположения */ \n"
                             "      y  INTEGER,             /* координата Y местоположения */ \n"
                             "      x1 INTEGER,             /* координата X верхнего левого угла */ \n"
                             "      y1 INTEGER,             /* координата Y верхнего левого угла */ \n"
                             "      x2 INTEGER,             /* координата X правого нижнего угла */ \n"
                             "      y2 INTEGER,             /* координата Y правого нижнего угла */ \n"
                             "      inserttime INTEGER,     /* время запроса */ \n"
                             "      UNIQUE (title, description, x, y) \n"
                             "  ); \n")
                         , QVariantMap(), res);

    result.setValue(res);
    errors.clear();
}

// -----------------------------------------------------------------------------
void SearchesDB::insertHistory(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QVariantMap>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QVariantMap vm = params.value<QVariantMap>();
    QVariantMap data;
    data[":REQUEST"] = vm.value("request");
    data[":TIME"   ] = vm.value("time");
    d_ptr->dc->execQuery("INSERT OR REPLACE INTO History (request, inserttime) "
                         "VALUES (:REQUEST, :TIME); ",
                         data, res);

    result.setValue(res);
    errors.clear();
}

// -----------------------------------------------------------------------------
void SearchesDB::loadHistory(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QVariantMap>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QVariantMap vm = params.value<QVariantMap>();
    QVariantMap data;
    data[":LIMIT" ] = vm.value("limit");
    data[":OFFSET"] = vm.value("offset");
    d_ptr->dc->execQuery("SELECT request, inserttime FROM History "
                         "ORDER BY inserttime DESC LIMIT :LIMIT OFFSET :OFFSET ",
                         data, res);

    result.setValue(res);
    errors.clear();
}

// -----------------------------------------------------------------------------
void SearchesDB::insertFavorite(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QVariantMap>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QVariantMap vm = params.value<QVariantMap>();
    QVariantMap data;
    data[":ID"         ] = vm.value("id");
    data[":TITLE"      ] = vm.value("title");
    data[":DESCRIPTION"] = vm.value("description");
    data[":X"          ] = vm.value("x");
    data[":Y"          ] = vm.value("y");
    data[":X1"         ] = vm.value("x1");
    data[":Y1"         ] = vm.value("y1");
    data[":X2"         ] = vm.value("x2");
    data[":Y2"         ] = vm.value("y2");
    data[":INSERTTIME" ] = vm.value("inserttime");
    d_ptr->dc->execQuery("INSERT OR REPLACE INTO Favorite (id, title, description, x, y, x1, y1, x2, y2, inserttime) "
                         "VALUES (:ID, :TITLE, :DESCRIPTION, :X, :Y, :X1, :Y1, :X2, :Y2, :INSERTTIME); ",
                         data, res);

    result.setValue(res);
    errors.clear();
}

// -----------------------------------------------------------------------------
void SearchesDB::removeFavorite(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull()) {
        errors = false;
        return;
    }

    QStringList keys;
     if (params.canConvert<QString>())
         keys.append(params.toString());
     else if (params.canConvert<QStringList>())
         keys = params.toStringList();
     else {
         errors = false;
         return;
     }

     dc::QueryResult res;
     d_ptr->dc->execQuery(QString("DELETE FROM Favorite WHERE id IN (%1); ").arg(QString("\'%1\'").arg(keys.join("\', \'"))), QVariantMap(), res);
     result.setValue(res);
     errors.clear();
}

// -----------------------------------------------------------------------------
void SearchesDB::loadFavorites(QVariant params, QVariant &result, QVariant &errors)
{
    if (params.isNull() || !params.canConvert<QVariantMap>()) {
        errors = false;
        return;
    }

    dc::QueryResult res;
    QVariantMap vm = params.value<QVariantMap>();
    QVariantMap data;
    data[":LIMIT" ] = vm.value("limit");
    data[":OFFSET"] = vm.value("offset");
    d_ptr->dc->execQuery("SELECT id, title, description, x, y, x1, y1, x2, y2, inserttime FROM Favorite "
                         "ORDER BY inserttime DESC LIMIT :LIMIT OFFSET :OFFSET ",
                         data, res);

    result.setValue(res);
    errors.clear();
}

#undef _ru

// ==================================================================
// ==================================================================
// ==================================================================
