#ifndef MAPSQL_H
#define MAPSQL_H

#include <QObject>
#include <QVariant>
#include <QPointer>

namespace dc {
    class DatabaseController;
}

// ==================================================================

typedef QList< QMap < QString, QVariant > > ListMapStringVariant;

Q_DECLARE_METATYPE(ListMapStringVariant)
Q_DECLARE_METATYPE(QPointer<QObject>)

// ==================================================================

// обработчик БД. подложка
class TilesDBPrivate;
class TilesDB : public QObject
{
    Q_OBJECT

public:
    explicit TilesDB(QObject *parent = 0);
    ~TilesDB();
    void setDc(dc::DatabaseController *);

private slots:
    //! ничего не сделать
    void noopSlot(QVariant params, QVariant &result, QVariant &errors);

    //! создать таблицы если ещё никто этого не сделал
    void createTables(QVariant params, QVariant &result, QVariant &errors);

    //! сохранить перечень плиток у себя в БД (при наличии - будет замена)
    void insertTiles(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить список плиток
    void loadTiles(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить список quadkey и плитку с минимальным quadkey
    void loadQuadTile(QVariant params, QVariant &result, QVariant &errors);
    //! удалить перечень плиток у себя из БД
    void removeTiles(QVariant params, QVariant &result, QVariant &errors);

private:
    Q_DECLARE_PRIVATE(TilesDB)
    Q_DISABLE_COPY(TilesDB)
    TilesDBPrivate *d_ptr;
};

// ==================================================================
#if 0
// обработчик БД. матрица высот
class HMatrixDBPrivate;
class HMatrixDB : public QObject
{
    Q_OBJECT

public:
    explicit HMatrixDB(QObject *parent = 0);
    ~HMatrixDB();
    void setDc(dc::DatabaseController *);

private slots:
    //! ничего не сделать
    void noopSlot(QVariant params, QVariant &result, QVariant &errors);

    //! создать таблицы если ещё никто этого не сделал
    void createTables(QVariant params, QVariant &result, QVariant &errors);

    //! сохранить матрицу высот
    void saveHMatrix(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить матрицу высот
    void loadHMatrix(QVariant params, QVariant &result, QVariant &errors);
    //! очистить бд
    void clearHMatrix(QVariant params, QVariant &result, QVariant &errors);

private:
    Q_DECLARE_PRIVATE(HMatrixDB)
    Q_DISABLE_COPY(HMatrixDB)
    HMatrixDBPrivate *d_ptr;
};

// ==================================================================

// обработчик БД. мульти матрица высот
class HMultiMatrixDBPrivate;
class HMultiMatrixDB : public QObject
{
    Q_OBJECT

public:
    explicit HMultiMatrixDB(QObject *parent = 0);
    ~HMultiMatrixDB();
    void setDc(dc::DatabaseController *);

private slots:
    //! ничего не сделать
    void noopSlot(QVariant params, QVariant &result, QVariant &errors);

    //! создать таблицы если ещё никто этого не сделал
    void createTables(QVariant params, QVariant &result, QVariant &errors);

    //! сохранить матрицу высот
    void saveHMatrix(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить матрицу высот
    void loadHMatrix(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить ид матриц высот
    void idHMatrix(QVariant params, QVariant &result, QVariant &errors);
    //! очистить бд
    void clearHMatrix(QVariant params, QVariant &result, QVariant &errors);

private:
    Q_DECLARE_PRIVATE(HMultiMatrixDB)
    Q_DISABLE_COPY(HMultiMatrixDB)
    HMultiMatrixDBPrivate *d_ptr;
};
#endif
// ==================================================================

// обработчик БД. поиск
class SearchesDBPrivate;
class SearchesDB : public QObject
{
    Q_OBJECT

public:
    explicit SearchesDB(QObject *parent = 0);
    ~SearchesDB();
    void setDc(dc::DatabaseController *);

private slots:
    //! ничего не сделать
    void noopSlot(QVariant params, QVariant &result, QVariant &errors);

    //! создать таблицы если ещё никто этого не сделал
    void createTables(QVariant params, QVariant &result, QVariant &errors);

    //! сохранить поисковый запрос в БД (при наличии - будет замена)
    void insertHistory(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить список поисковых запросов
    void loadHistory(QVariant params, QVariant &result, QVariant &errors);
    //! сохранить избранное местоположение в БД (при наличии - будет замена)
    void insertFavorite(QVariant params, QVariant &result, QVariant &errors);
    //! удалить избранное местоположение из БД
    void removeFavorite(QVariant params, QVariant &result, QVariant &errors);
    //! загрузить список избранных местоположений
    void loadFavorites(QVariant params, QVariant &result, QVariant &errors);

private:
    Q_DECLARE_PRIVATE(SearchesDB)
    Q_DISABLE_COPY(SearchesDB)
    SearchesDBPrivate *d_ptr;
};

// ==================================================================
#endif // MAPSQL_H
