#ifndef MAPTILELOADER_H
#define MAPTILELOADER_H

#include <QObject>
#include <QImage>
#include <QNetworkReply>

// ---------------------------------

namespace minigis {

// ---------------------------------

class MapLayerTile;

// ---------------------------------

class MapTileLoaderPrivate;
class MapTileLoader : public QObject
{
    Q_OBJECT
public:
    explicit MapTileLoader();
    virtual ~MapTileLoader();

    enum {Type = 0};
    virtual quint8 type() const = 0;
    virtual void getTile(int x, int y, int z) = 0;
    virtual QString description() const = 0;
    virtual QString fileformat() const;
    virtual bool isTemporaryTiles() const = 0;
    virtual bool nightModeAvalible() const = 0;

    void setEnabled(bool);
    bool isEnabled();

public Q_SLOTS:
    virtual void init(MapLayerTile *) = 0;
    virtual void done() = 0;

Q_SIGNALS:
    void imageReady(QImage, int, int, int, int, int);
    void errorKey(int, int, int, int);

protected:
    MapTileLoader(MapTileLoaderPrivate &);

    Q_DISABLE_COPY(MapTileLoader)
    Q_DECLARE_PRIVATE(MapTileLoader)

    MapTileLoaderPrivate* d_ptr;
};

// ---------------------------------

class MapTileLoaderHttpPrivate;
class MapTileLoaderHttp : public MapTileLoader
{
    Q_OBJECT
public:
    explicit MapTileLoaderHttp();
    virtual ~MapTileLoaderHttp();    

public Q_SLOTS:
    virtual void init(MapLayerTile *);
    virtual void done();
    virtual void setProxyEnabled(bool);
    virtual bool isProxyEnabled();
    virtual void setProxy(const QString &host, quint16 port);

protected Q_SLOTS:
    void getError(QNetworkReply::NetworkError);
    void replyFinished(QNetworkReply*);

protected:
    MapTileLoaderHttp(MapTileLoaderHttpPrivate &);

    Q_DISABLE_COPY(MapTileLoaderHttp)
    Q_DECLARE_PRIVATE(MapTileLoaderHttp)
};

// ---------------------------------

class MapTileLoaderHttpTemplPrivate;
class MapTileLoaderHttpTempl : public MapTileLoaderHttp
{
public:
    explicit MapTileLoaderHttpTempl(QString const &url, QString const &desc, QString const &imType, quint8 type, bool isTmp, bool isNight);
    virtual ~MapTileLoaderHttpTempl();
    virtual void getTile(int x, int y, int z);
    virtual QString description() const;
    virtual quint8 type() const;
    virtual bool isTemporaryTiles() const;
    virtual bool nightModeAvalible() const;

protected:
    bool parseUrl(QString const &url);

    Q_DISABLE_COPY(MapTileLoaderHttpTempl)
    Q_DECLARE_PRIVATE(MapTileLoaderHttpTempl)
};

// ---------------------------------

class MapTileLoaderHttpTemplLoader
{
public:
    static QList<MapTileLoaderHttp *> load(QString const &fileName);
};

// ---------------------------------

class MapTileLoaderYandexPrivate;
class MapTileLoaderYandex : public MapTileLoaderHttp
{
    Q_OBJECT
public:
    explicit MapTileLoaderYandex(QString const &url, QString const &desc, QString const &imType, quint8 type, bool isTmp, bool isNight);
    virtual ~MapTileLoaderYandex();

    virtual void getTile(int x, int y, int z);
    virtual QString description() const;
    virtual quint8 type() const;
    virtual bool isTemporaryTiles() const;
    virtual bool nightModeAvalible() const;

protected Q_SLOTS:
    void replyFinished(QNetworkReply*);

protected:
    bool parseUrl(QString const &url);

    Q_DISABLE_COPY(MapTileLoaderYandex)
    Q_DECLARE_PRIVATE(MapTileLoaderYandex)
};

// ---------------------------------

class MapTileLoaderWMSPrivate;
class MapTileLoaderWMS : public MapTileLoaderHttp
{
    Q_OBJECT
public:
    explicit MapTileLoaderWMS(QString const &url, QString const &layers, QString const &desc, QString const &imType, quint8 type, bool isTmp, bool isNight);
    virtual ~MapTileLoaderWMS();

    virtual void getTile(int x, int y, int z);
    virtual QString description() const;
    virtual quint8 type() const;
    virtual bool isTemporaryTiles() const;
    virtual bool nightModeAvalible() const;

protected Q_SLOTS:
    void replyFinished(QNetworkReply*);

protected:
    bool parseUrl(QString const &url);

    Q_DISABLE_COPY(MapTileLoaderWMS)
    Q_DECLARE_PRIVATE(MapTileLoaderWMS)
};
// ---------------------------------
// FIXME: ProxyServer

#if 0
class MapTileLoaderYandexWeatherPrivate;
class MapTileLoaderYandexWeather: public MapTileLoaderYandex
{
    Q_OBJECT
public:
    explicit MapTileLoaderYandexWeather();
    virtual ~MapTileLoaderYandexWeather();

    enum {Type = 6, IsTmp = 0, NightMode = 0};
    virtual quint8 type() const {return Type;}
    virtual void getTile(int x, int y, int z);
    virtual QString description() const;
    virtual bool isTemporaryTiles() const {return IsTmp;}
    virtual bool nightModeAvalible() const {return NightMode;}

protected Q_SLOTS:
    void replyFinished(QNetworkReply*);

protected:
    Q_DISABLE_COPY(MapTileLoaderYandexWeather)
    Q_DECLARE_PRIVATE(MapTileLoaderYandexWeather)
};
#endif

// ---------------------------------

class MapTileLoaderCheckFillDataBasePrivate;
class MapTileLoaderCheckFillDataBase: public MapTileLoader
{
    Q_OBJECT
public:
    explicit MapTileLoaderCheckFillDataBase();
    virtual ~MapTileLoaderCheckFillDataBase();

    virtual void init(MapLayerTile *);
    virtual void done();

    enum {Type = 51, IsTmp = 1, NightMode = 1};
    virtual quint8 type() const {return Type;}
    virtual void getTile(int x, int y, int z);
    virtual QString description() const;
    virtual bool isTemporaryTiles() const {return IsTmp;}
    virtual bool nightModeAvalible() const {return NightMode;}

    void setWatchType(int type);
    int watchType();

protected Q_SLOTS:
    void onDb_QuadIncome(uint query, QVariant result, QVariant error);

protected:
    Q_DISABLE_COPY(MapTileLoaderCheckFillDataBase)
    Q_DECLARE_PRIVATE(MapTileLoaderCheckFillDataBase)
};

// ---------------------------------


} //minigis

#endif // MAPTILELOADER_H
