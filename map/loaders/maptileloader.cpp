
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QDomDocument>
#include <QDomElement>
#include <QImage>
#include <QThread>
#include <QPainter>
#include <QFile>
#include <QDateTime>
#include <QString>

#include <functional>

#include "db/databasecontroller.h"

#include "core/mapdefs.h"
#include "coord/mapcoords.h"
#include "sql/mapsql.h"
#include "loaders/maptileloader.h"
#include "layers/maplayertile.h"

// --------------------------------------------------

namespace minigis {

// --------------------------------------------------

namespace {

QString strTrimmed(const QString &str) {
    return str.trimmed();
}

}

// --------------------------------------------------

// выводить в лог ошибки получения подложки
//#ifdef PRINT_NETWORK_ERR

// --------------------------------------------------

class MapTileLoaderPrivate
{    
public:
    QString fileFormat;
    bool isEnabled;
};

// --------------------------------------------------

MapTileLoader::MapTileLoader()
    : d_ptr(new MapTileLoaderPrivate())
{    
    setObjectName("MapTileLoader");
    d_ptr->isEnabled = true;
}

MapTileLoader::MapTileLoader(MapTileLoaderPrivate &dd)
    : d_ptr(&dd)
{
    setObjectName("MapTileLoader");
    d_ptr->isEnabled = true;
}

MapTileLoader::~MapTileLoader()
{
    if (d_ptr) {
        MapTileLoaderPrivate *tmp = d_ptr;
        d_ptr = NULL;
        delete tmp;
    }
}

QString MapTileLoader::fileformat() const
{
    return d_ptr->fileFormat;
}

void MapTileLoader::setEnabled(bool a)
{
    d_ptr->isEnabled = a;
}

bool MapTileLoader::isEnabled()
{
    return d_ptr->isEnabled;
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

class MapTileLoaderHttpPrivate : public MapTileLoaderPrivate
{
public:
    MapTileLoaderHttpPrivate() : manager(NULL), proxyEnabled(false) {}
    ~MapTileLoaderHttpPrivate()
    {
        if (manager) {
            delete manager;
            manager = NULL;
        }
    }

    QNetworkAccessManager *manager;
    QNetworkProxy proxy;
    bool proxyEnabled;
};

// ----------------------------------------------------
MapTileLoaderHttp::MapTileLoaderHttp()
    : MapTileLoader(*new MapTileLoaderHttpPrivate)
{
    setObjectName("MapTileLoaderHttp");
    Q_D(MapTileLoaderHttp);
    d->proxy.setType(QNetworkProxy::HttpProxy);
}

MapTileLoaderHttp::MapTileLoaderHttp(MapTileLoaderHttpPrivate &dd)
    : MapTileLoader(dd)
{
    setObjectName("MapTileLoaderHttp");
    Q_D(MapTileLoaderHttp);
    d->proxy.setType(QNetworkProxy::HttpProxy);
}

MapTileLoaderHttp::~MapTileLoaderHttp()
{
}

void MapTileLoaderHttp::init(MapLayerTile *)
{
    Q_D(MapTileLoaderHttp);
    if (d->manager)
        delete d->manager;
    d->manager = new QNetworkAccessManager;

    connect(d->manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
}

void MapTileLoaderHttp::done()
{
    Q_D(MapTileLoaderHttp);
    if (d->manager) {
        delete d->manager;
        d->manager = NULL;
    }
}

void MapTileLoaderHttp::setProxyEnabled(bool e)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this,
                                  "setProxyEnabled",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, e));
        return;
    }

    Q_D(MapTileLoaderHttp);
    d->proxyEnabled = e;
    d->manager->setProxy(d->proxyEnabled ? d->proxy : QNetworkProxy());
}

bool MapTileLoaderHttp::isProxyEnabled()
{
    Q_D(MapTileLoaderHttp);
    return d->proxyEnabled;
}

void MapTileLoaderHttp::setProxy(const QString &host, quint16 port)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this,
                                  "setProxy",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, host),
                                  Q_ARG(quint16, port));
        return;
    }

    Q_D(MapTileLoaderHttp);
    d->proxy.setHostName(host);
    d->proxy.setPort(port);
    if (d->proxyEnabled)
        d->manager->setProxy(d->proxy);
}

void MapTileLoaderHttp::getError(QNetworkReply::NetworkError err)
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    int x = reply->property("x").toInt();
    int y = reply->property("y").toInt();
    int z = reply->property("z").toInt();
    int type = reply->property("type").toInt();
    emit errorKey(x, y, z, type);

#ifdef PRINT_NETWORK_ERR
    qDebug() << "=======================================";
    qDebug() << "MapTileLoader::error" << err;
    qDebug() << qPrintable(reply->errorString());
    qDebug() << "url =" << qPrintable(reply->url().toString());
    qDebug() << "x =" << x << "; y =" << y << "; z =" << z << "type =" << type;
#else
    Q_UNUSED(err);
#endif
}

void MapTileLoaderHttp::replyFinished(QNetworkReply *reply)
{
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError)
        return;

    QImage img;
    img.loadFromData(reply->readAll());

    int expires = 0;
    if (reply->hasRawHeader("Expires"))
    {
        QDateTime dt = QLocale("en").toDateTime(reply->rawHeader("Expires").constData(), "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
        dt.setTimeSpec(Qt::UTC);
        expires = dt.toTime_t();
    }

    emit imageReady(img, reply->property("x").toInt(), reply->property("y").toInt(), reply->property("z").toInt(), reply->property("type").toInt(), expires);
    reply->deleteLater();
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

class ValueSequence
{
public:
    virtual ~ValueSequence() { }
    virtual QString value() const = 0;
};

class IntSequence : public ValueSequence
{
public:
    IntSequence() { }
    IntSequence(int _val) : val(_val) { }
    virtual QString value() const { return QString::number(val); }
    void set(int _val) { val = _val; }
    int val;
};

class QuadSequence : public ValueSequence
{
public:
    QuadSequence() { }
    QuadSequence(int _x, int _y, int _z) : x(_x), y(_y), z(_z) { }
    virtual QString value() const { return minigis::TileSystem::tileToQuadKey(x, y, z); }
    void set(int _x, int _y, int _z) { x = _x; y = _y; z = _z; }
    int x, y, z;
};

class LetterSequence : public ValueSequence
{
public:
    LetterSequence() { }
    LetterSequence(QStringList const &_seq) : pos(0) { set(_seq); }
    QString value() const {
        if (seq.isEmpty())
            return QString();
        if (++pos >= seq.size())
            pos = 0;
        return seq.at(pos);
    }
    void set(QStringList const &_seq) {
        seq = _seq;

        std::transform(seq.begin(), seq.end(), seq.begin(), strTrimmed);
//        std::transform(seq.begin(), seq.end(), seq.begin(), std::mem_fun_ref(&QString::trimmed));
        std::remove_if(seq.begin(), seq.end(), std::mem_fun_ref(&QString::isEmpty));
    }

    mutable int pos;
    QStringList seq;
};

// ----------------------------------------------------

class MapTileLoaderHttpTemplPrivate : public MapTileLoaderHttpPrivate
{
public:

    QString url;
    QMap<QString, ValueSequence*> vals;

    QString desc;
    quint8 type;
    bool isTmp;
    bool isNight;
};

// ----------------------------------------------------

MapTileLoaderHttpTempl::MapTileLoaderHttpTempl(const QString &url, const QString &desc, QString const &imType, quint8 type, bool isTmp, bool isNight)
    : MapTileLoaderHttp(*new MapTileLoaderHttpTemplPrivate)
{
    Q_D(MapTileLoaderHttpTempl);
    parseUrl(url);
    d->desc       = desc;
    d->fileFormat = imType;
    d->type       = type;
    d->isTmp      = isTmp;
    d->isNight    = isNight;
}

MapTileLoaderHttpTempl::~MapTileLoaderHttpTempl()
{
    Q_D(MapTileLoaderHttpTempl);
    QMap<QString, ValueSequence*> tmp;
    tmp.swap(d->vals);
    for (QMapIterator<QString, ValueSequence*> it(tmp); it.hasNext(); ) {
        it.next();
        ValueSequence *v = it.value();
        if (v)
            delete v;
    }
}

void MapTileLoaderHttpTempl::getTile(int x, int y, int z)
{
    Q_D(MapTileLoaderHttpTempl);

#define TestAndSet(KEY, VAL, CLS) { CLS *v = dynamic_cast<CLS *>(d->vals.value(KEY)); if (v) v->set(VAL); }
#define _ ,
    TestAndSet("{x}", x, IntSequence);
    TestAndSet("{y}", y, IntSequence);
    TestAndSet("{z}", z, IntSequence);
    TestAndSet("{q}", x _ y _ z, QuadSequence);
#undef _
#undef TestAndSet

    QString address(d->url);
    for (QMapIterator<QString, ValueSequence*> it(d->vals); it.hasNext(); ) {
        it.next();
        address.replace(it.key(), it.value()->value(), Qt::CaseInsensitive);
    }

    QUrl url(address);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "noop");
    QNetworkReply *reply = d->manager->get(request);

    reply->setProperty("x", x);
    reply->setProperty("y", y);
    reply->setProperty("z", z);
    reply->setProperty("type", type());

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
}

QString MapTileLoaderHttpTempl::description() const
{
    Q_D(const MapTileLoaderHttpTempl);
    return d->desc;
}

quint8 MapTileLoaderHttpTempl::type() const
{
    Q_D(const MapTileLoaderHttpTempl);
    return d->type;
}

bool MapTileLoaderHttpTempl::isTemporaryTiles() const
{
    Q_D(const MapTileLoaderHttpTempl);
    return d->isTmp;
}

bool MapTileLoaderHttpTempl::nightModeAvalible() const
{
    Q_D(const MapTileLoaderHttpTempl);
    return d->isNight;
}

bool MapTileLoaderHttpTempl::parseUrl(QString const &url)
{
    Q_D(MapTileLoaderHttpTempl);
    QString str = url;
    int n = 0;
    QString nKey;
    QString switchKey = "{switch:";
    int switchPos, bracePos;
    while (true) {
        if ((switchPos = str.indexOf(switchKey, 0, Qt::CaseInsensitive)) < 0)
            break;
        if ((bracePos = str.indexOf("}", switchPos, Qt::CaseInsensitive)) < 0) {
            qWarning() << "Error in" << url << "after" << switchPos << "position";
            return false;
        }
        nKey = QString("{%1}").arg(++n);
        d->vals[nKey] = new LetterSequence(str.mid(switchPos+switchKey.length(), bracePos-switchPos-switchKey.length()).split(","));
        str.replace(switchPos, bracePos-switchPos+1, nKey);
    }

#define TestAndCreate(KEY, CLS) { if (str.indexOf(KEY) >=0) d->vals[KEY] = new CLS(); }
    TestAndCreate("{x}", IntSequence);
    TestAndCreate("{y}", IntSequence);
    TestAndCreate("{z}", IntSequence);
    TestAndCreate("{q}", QuadSequence);
#undef TestAndCreate

    d->url = str;
    return true;
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

QList<MapTileLoaderHttp *> MapTileLoaderHttpTemplLoader::load(QString const &fileName)
{
    QList<MapTileLoaderHttp *> servers;

    QDomDocument doc("servers");
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No file: " << fileName;
        return servers;
    }
    if (!doc.setContent(&file)) {
        qDebug() << "No Content: " << fileName;
        file.close();
        return servers;
    }
    file.close();

    QDomElement root = doc.documentElement();

    QString addr;
    QString name;
    QString cls;
    QString ext;
    QString prop;
    QStringList propl;
    for (QDomElement e = root.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
        addr = e.attribute("addr").trimmed();
        if (addr.isEmpty())
            continue;
        name = e.attribute("name").trimmed();
        cls = e.attribute("class").trimmed().toLower();
        if (cls.isEmpty())
            cls = "default";
        ext = e.attribute("ext").trimmed().toUpper();
        if (ext.isEmpty())
            ext = "PNG";
        if (e.hasAttribute("prop"))
            prop = e.attribute("prop").toLower();
        else
            prop = "night";
        propl = prop.split(",");

        std::transform(propl.begin(), propl.end(), propl.begin(), strTrimmed);
//        std::transform(propl.begin(), propl.end(), propl.begin(), std::mem_fun_ref(&QString::trimmed));

        MapTileLoaderHttp *srv = NULL;
        if (cls == "default")
            srv = new MapTileLoaderHttpTempl(
                        addr,
                        name,
                        ext,
                        e.attribute("id").trimmed().toInt(),
                        propl.contains("tmp"),
                        propl.contains("night")
                        );
        else if (cls == "sphere")
            srv = new MapTileLoaderYandex(
                        addr,
                        name,
                        ext,
                        e.attribute("id").trimmed().toInt(),
                        propl.contains("tmp"),
                        propl.contains("night")
                        );
        else if (cls == "wms")
            srv = new MapTileLoaderWMS(
                        addr,
                        e.attribute("layers").trimmed(),
                        name,
                        ext,
                        e.attribute("id").trimmed().toInt(),
                        propl.contains("tmp"),
                        propl.contains("night")
                        );
        if (srv)
            servers.append(srv);
    }

    return servers;
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

struct ImageExpire
{
    ImageExpire(QImage *img = NULL, int expire = 0) : image(img), timeExpire(expire) {}

    QSharedPointer<QImage> image;
    int timeExpire;
};

// ----------------------------------------------------

struct YandexTileKey
{
    YandexTileKey() : fFull(false), sFull(false) {}

    TileKey fKey;
    TileKey sKey;

    bool fFull;
    bool sFull;

    QSharedPointer<QImage> fImg;
    QSharedPointer<QImage> sImg;

    int fExpires;
    int sExpires;

    void caclTilesKey(int x, int y, int z) {
        QRectF tileRect = TileSystem::tileBounds(QPoint(y, x), z);

        qreal midY = tileRect.center().y();
        QPoint tl = MyUtils::metersToEllipticTile(QPointF(tileRect.left() , midY), z);
        QPoint br = MyUtils::metersToEllipticTile(QPointF(tileRect.right(), midY), z);

        tl = QPoint(tl.y(), tl.x());
        br = QPoint(br.y(), br.x());

        fKey = TileKey(tl.x(), tl.y(), z);
        sKey = TileKey(br.x(), br.y(), z);

        if (fKey == sKey)
            sFull = true;
    }
};

// ----------------------------------------------------

class MapTileLoaderYandexPrivate : public MapTileLoaderHttpPrivate
{
public:
    QString url;
    QMap<QString, ValueSequence*> vals;

    QString desc;
    quint8 type;
    bool isTmp;
    bool isNight;

    // ----------------------

    QMap<TileKey, YandexTileKey> cache;
    QMap<TileKey, ImageExpire> imageCache;
    static const int CacheSize = 25;

    QImage createYandexTile(const TileKey &key, const YandexTileKey &yKeys);
};

QImage MapTileLoaderYandexPrivate::createYandexTile(const TileKey &key, const YandexTileKey &yKeys)
{
    int base = 1 << key.z;

    QRectF rectb = MyUtils::tileBoundsElliptic(QPoint(base - 1 - key.y, key.x), key.z);
    QRectF rb1 = TileSystem::tileBounds(QPoint(base - 1 - yKeys.fKey.y, yKeys.fKey.x), yKeys.fKey.z);

    QPointF q1 = TileSystem::metersToPix(rectb.topLeft(), key.z);
    QPointF q2 = TileSystem::metersToPix(rectb.bottomRight(), key.z);
    QPointF q3 = TileSystem::metersToPix(rb1.topLeft(), key.z);

    QImage bigImg = QImage(QSize(256, 512), QImage::Format_ARGB32_Premultiplied);
    bigImg.fill(Qt::transparent);
    QImage img = QImage(QSize(256, 256), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&bigImg);
    p.drawImage(QPoint(), *yKeys.fImg);
    p.drawImage(QPoint(0, 256), *yKeys.sImg);
    p.end();

    QPainter painter(&img);
    painter.drawImage(QRectF(QPointF(0, 0), QSizeF(256, 256)), bigImg, QRectF(QPointF(0, (q3 - q1).x()), QSizeF(256, (q2 - q1).x())));
    painter.end();

    return img;
}

// ----------------------------------------------------

MapTileLoaderYandex::MapTileLoaderYandex(QString const &url, QString const &desc, QString const &imType, quint8 type, bool isTmp, bool isNight)
    : MapTileLoaderHttp(*new MapTileLoaderYandexPrivate)
{
    setObjectName("MapTileLoaderYandex");
    Q_D(MapTileLoaderYandex);
    parseUrl(url);
    d->desc       = desc;
    d->fileFormat = imType;
    d->type       = type;
    d->isTmp      = isTmp;
    d->isNight    = isNight;
}

MapTileLoaderYandex::~MapTileLoaderYandex()
{
    Q_D(MapTileLoaderYandex);
    QMap<QString, ValueSequence*> tmp;
    tmp.swap(d->vals);
    for (QMapIterator<QString, ValueSequence*> it(tmp); it.hasNext(); ) {
        it.next();
        ValueSequence *v = it.value();
        if (v)
            delete v;
    }
}

bool MapTileLoaderYandex::parseUrl(QString const &url)
{
    Q_D(MapTileLoaderYandex);
    QString str = url;
    int n = 0;
    QString nKey;
    QString switchKey = "{switch:";
    int switchPos, bracePos;
    while (true) {
        if ((switchPos = str.indexOf(switchKey, 0, Qt::CaseInsensitive)) < 0)
            break;
        if ((bracePos = str.indexOf("}", switchPos, Qt::CaseInsensitive)) < 0) {
            qWarning() << "Error in" << url << "after" << switchPos << "position";
            return false;
        }
        nKey = QString("{%1}").arg(++n);
        d->vals[nKey] = new LetterSequence(str.mid(switchPos+switchKey.length(), bracePos-switchPos-switchKey.length()).split(","));
        str.replace(switchPos, bracePos-switchPos+1, nKey);
    }

#define TestAndCreate(KEY, CLS) { if (str.indexOf(KEY) >=0) d->vals[KEY] = new CLS(); }
    TestAndCreate("{x}", IntSequence);
    TestAndCreate("{y}", IntSequence);
    TestAndCreate("{z}", IntSequence);
    TestAndCreate("{q}", QuadSequence);
#undef TestAndCreate

    d->url = str;
    return true;
}

void MapTileLoaderYandex::getTile(int x, int y, int z)
{
    Q_D(MapTileLoaderYandex);

    TileKey parentKey(x, y, z);
    YandexTileKey key;
    key.caclTilesKey(x, y, z);
    if (d->cache.contains(parentKey))
        return;

//    ImageExpire fIe = d->imageCache.value(key.fKey);
//    if (!fIe.image.isNull()) {
//        key.fFull = true;
//        key.fImg = fIe.image;
//        key.fExpires = fIe.timeExpire;
//    }
//    ImageExpire sIe = d->imageCache.value(key.sKey);
//    if (!sIe.image.isNull()) {
//        key.sFull = true;
//        key.sImg = fIe.image;
//        key.sExpires = fIe.timeExpire;
//    }

    d->cache.insert(parentKey, key);

    if (!key.fFull) {
        #define TestAndSet(KEY, VAL, CLS) { CLS *v = dynamic_cast<CLS *>(d->vals.value(KEY)); if (v) v->set(VAL); }
        #define _ ,
        TestAndSet("{x}", key.fKey.x, IntSequence);
        TestAndSet("{y}", key.fKey.y, IntSequence);
        TestAndSet("{z}", key.fKey.z, IntSequence);
        TestAndSet("{q}", key.fKey.z _ key.fKey.y _ key.fKey.z, QuadSequence);
        #undef _
        #undef TestAndSet

        QString address(d->url);
        for (QMapIterator<QString, ValueSequence*> it(d->vals); it.hasNext(); ) {
            it.next();
            address.replace(it.key(), it.value()->value(), Qt::CaseInsensitive);
        }

        QNetworkReply *fReply = d->manager->get(QNetworkRequest(QUrl(address)));

        fReply->setProperty("x", key.fKey.x);
        fReply->setProperty("y", key.fKey.y);
        fReply->setProperty("z", key.fKey.z);
        fReply->setProperty("parent_x", x);
        fReply->setProperty("parent_y", y);
        fReply->setProperty("parent_z", z);
        fReply->setProperty("type", type());

        connect(fReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    }

    if (!key.sFull) {
        #define TestAndSet(KEY, VAL, CLS) { CLS *v = dynamic_cast<CLS *>(d->vals.value(KEY)); if (v) v->set(VAL); }
        #define _ ,
        TestAndSet("{x}", key.sKey.x, IntSequence);
        TestAndSet("{y}", key.sKey.y, IntSequence);
        TestAndSet("{z}", key.sKey.z, IntSequence);
        TestAndSet("{q}", key.sKey.z _ key.sKey.y _ key.sKey.z, QuadSequence);
        #undef _
        #undef TestAndSet

        QString address(d->url);
        for (QMapIterator<QString, ValueSequence*> it(d->vals); it.hasNext(); ) {
            it.next();
            address.replace(it.key(), it.value()->value(), Qt::CaseInsensitive);
        }

        QNetworkReply *sReply = d->manager->get(QNetworkRequest(QUrl(address)));

        sReply->setProperty("x", key.sKey.x);
        sReply->setProperty("y", key.sKey.y);
        sReply->setProperty("z", key.sKey.z);
        sReply->setProperty("parent_x", x);
        sReply->setProperty("parent_y", y);
        sReply->setProperty("parent_z", z);
        sReply->setProperty("type", type());

        connect(sReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    }

//    if (key.sFull && key.fFull) {
//        d->cache.remove(parentKey);
//        emit imageReady(key.sImg ? *key.fImg : d->createYandexTile(parentKey, key), x, y, z, type(), qMin(key.fExpires, key.sExpires));
//    }
}


QString MapTileLoaderYandex::description() const
{
    Q_D(const MapTileLoaderYandex);
    return d->desc;
}

quint8 MapTileLoaderYandex::type() const
{
    Q_D(const MapTileLoaderYandex);
    return d->type;
}

bool MapTileLoaderYandex::isTemporaryTiles() const
{
    Q_D(const MapTileLoaderYandex);
    return d->isTmp;
}

bool MapTileLoaderYandex::nightModeAvalible() const
{
    Q_D(const MapTileLoaderYandex);
    return d->isNight;
}

void MapTileLoaderYandex::replyFinished(QNetworkReply *reply)
{
    Q_D(MapTileLoaderYandex);
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError)
        return;

    QImage img;
    img.loadFromData(reply->readAll());

    int expires = 0;
    if (reply->hasRawHeader("Expires"))
    {
        QDateTime dt = QLocale("en").toDateTime(reply->rawHeader("Expires").constData(), "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
        dt.setTimeSpec(Qt::UTC);
        expires = dt.toTime_t();
    }

    TileKey tmpKey(reply->property("parent_x").toInt(), reply->property("parent_y").toInt(), reply->property("parent_z").toInt());
    TileKey key(reply->property("x").toInt(), reply->property("y").toInt(), reply->property("z").toInt());

    YandexTileKey yKey = d->cache.value(tmpKey);

    ImageExpire ie(new QImage(img), expires);

    if (yKey.fKey == key) {
        yKey.fFull = true;
        yKey.fImg = ie.image;
        yKey.fExpires = expires;
    }
    else if (yKey.sKey == key) {
        yKey.sFull = true;
        yKey.sImg = ie.image;
        yKey.sExpires = expires;
    }
    d->cache.insert(tmpKey, yKey);

    if (yKey.fFull && yKey.sFull) {
        d->cache.remove(tmpKey);
        emit imageReady(yKey.sImg.isNull() ? *yKey.fImg : d->createYandexTile(tmpKey, yKey), tmpKey.x, tmpKey.y, tmpKey.z, reply->property("type").toInt(), qMin(yKey.fExpires, yKey.sExpires));
        reply->deleteLater();
    }

//    d->imageCache.insert(key, ie);
//    if (d->imageCache.size() > d->CacheSize)
//        d->imageCache.clear();
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

class MapTileLoaderWMSPrivate : public MapTileLoaderHttpPrivate
{
public:

    QString url;
    QString layers;
    QMap<QString, ValueSequence*> vals;

    QString desc;
    quint8 type;
    bool isTmp;
    bool isNight;
};

// ----------------------------------------------------

MapTileLoaderWMS::MapTileLoaderWMS(const QString &url, QString const &layers, const QString &desc, QString const &imType, quint8 type, bool isTmp, bool isNight)
    : MapTileLoaderHttp(*new MapTileLoaderWMSPrivate)
{
    setObjectName("MapTileLoaderWMS");
    Q_D(MapTileLoaderWMS);
    parseUrl(url);
    d->layers     = layers;
    d->desc       = desc;
    d->fileFormat = imType;
    d->type       = type;
    d->isTmp      = isTmp;
    d->isNight    = isNight;
}

MapTileLoaderWMS::~MapTileLoaderWMS()
{
    Q_D(MapTileLoaderWMS);
    QMap<QString, ValueSequence*> tmp;
    tmp.swap(d->vals);
    for (QMapIterator<QString, ValueSequence*> it(tmp); it.hasNext(); ) {
        it.next();
        ValueSequence *v = it.value();
        if (v)
            delete v;
    }
}

void MapTileLoaderWMS::getTile(int x, int y, int z)
{
    Q_D(MapTileLoaderWMS);

#define TestAndSet(KEY, VAL, CLS) { CLS *v = dynamic_cast<CLS *>(d->vals.value(KEY)); if (v) v->set(VAL); }
#define _ ,
    TestAndSet("{x}", x, IntSequence);
    TestAndSet("{y}", y, IntSequence);
    TestAndSet("{z}", z, IntSequence);
    TestAndSet("{q}", x _ y _ z, QuadSequence);
#undef _
#undef TestAndSet

    QString address(d->url);
    for (QMapIterator<QString, ValueSequence*> it(d->vals); it.hasNext(); ) {
        it.next();
        address.replace(it.key(), it.value()->value(), Qt::CaseInsensitive);
    }

    // invert axis
    int ix = (1 << z) - 1 - y;
    int iy = x;

    QPointF p1 = TileSystem::pixToMeters(TileSystem::tileToPixel(QPoint(ix, iy)), z);
    QPointF p2 = TileSystem::pixToMeters(TileSystem::tileToPixel(QPoint(ix+1, iy+1)), z);

    address +=
            QString("?service=WMS&request=GetMap&version=1.3&layers=%1&styles=&format=image/png&transparent=true&"
                    "noWrap=true&f=image&tiled=true&height=256&width=256&crs=EPSG:3857&srs=EPSG:3857&bbox=%2,%3,%4,%5")
            .arg(d->layers)
            .arg(p1.y(), 16, 'f', 8)
            .arg(p1.x(), 16, 'f', 8)
            .arg(p2.y(), 16, 'f', 8)
            .arg(p2.x(), 16, 'f', 8);

    QUrl url(address);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "noop");
    QNetworkReply *reply = d->manager->get(request);

    reply->setProperty("x", x);
    reply->setProperty("y", y);
    reply->setProperty("z", z);
    reply->setProperty("type", type());

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
}

QString MapTileLoaderWMS::description() const
{
    Q_D(const MapTileLoaderWMS);
    return d->desc;
}

quint8 MapTileLoaderWMS::type() const
{
    Q_D(const MapTileLoaderWMS);
    return d->type;
}

bool MapTileLoaderWMS::isTemporaryTiles() const
{
    Q_D(const MapTileLoaderWMS);
    return d->isTmp;
}

bool MapTileLoaderWMS::nightModeAvalible() const
{
    Q_D(const MapTileLoaderWMS);
    return d->isNight;
}

bool MapTileLoaderWMS::parseUrl(QString const &url)
{
    Q_D(MapTileLoaderWMS);
    QString str = url;
    int n = 0;
    QString nKey;
    QString switchKey = "{switch:";
    int switchPos, bracePos;
    while (true) {
        if ((switchPos = str.indexOf(switchKey, 0, Qt::CaseInsensitive)) < 0)
            break;
        if ((bracePos = str.indexOf("}", switchPos, Qt::CaseInsensitive)) < 0) {
            qWarning() << "Error in" << url << "after" << switchPos << "position";
            return false;
        }
        nKey = QString("{%1}").arg(++n);
        d->vals[nKey] = new LetterSequence(str.mid(switchPos+switchKey.length(), bracePos-switchPos-switchKey.length()).split(","));
        str.replace(switchPos, bracePos-switchPos+1, nKey);
    }

#define TestAndCreate(KEY, CLS) { if (str.indexOf(KEY) >=0) d->vals[KEY] = new CLS(); }
    TestAndCreate("{x}", IntSequence);
    TestAndCreate("{y}", IntSequence);
    TestAndCreate("{z}", IntSequence);
    TestAndCreate("{q}", QuadSequence);
#undef TestAndCreate

    d->url = str;
    return true;
}

void MapTileLoaderWMS::replyFinished(QNetworkReply *reply)
{
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError)
        return;

    QImage img;
    img.loadFromData(reply->readAll());

    int expires = 0;
    if (reply->hasRawHeader("Expires"))
    {
        QDateTime dt = QLocale("en").toDateTime(reply->rawHeader("Expires").constData(), "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
        dt.setTimeSpec(Qt::UTC);
        expires = dt.toTime_t();
    }

    emit imageReady(img, reply->property("x").toInt(), reply->property("y").toInt(), reply->property("z").toInt(), reply->property("type").toInt(), expires);
    reply->deleteLater();
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

#if 0
struct CityWeather {
    int temperature;
    QPointF pos;
    QString img;

    CityWeather() : temperature(0) {}
};

struct TileWeather {
    int x;
    int y;
    int z;
    int expires;
    QList<CityWeather> cities;
    int parts;

    TileWeather() : x(0), y(0), z(0), expires(0), parts(0) {}
};

class MapTileLoaderYandexWeatherPrivate : public MapTileLoaderYandexPrivate
{
    Q_DECLARE_PUBLIC(MapTileLoaderYandexWeather)

public:
    MapTileLoaderYandexWeatherPrivate(MapTileLoaderYandexWeather *q) : q_ptr(q) {}

public:
    QHash<QString, QImage> icons;
    QHash<QString, TileWeather> tileWeather;

private:
    MapTileLoaderYandexWeather *q_ptr;

public:
    void tileBounds(int x, int y, int z, QPointF &tl, QPointF &br);
    QImage trimEmptyLines(const QImage &sourceImage);
    void generateTiles();
    void generateTile(const QString &quadKey);

};

// ----------------------------------------------------

void MapTileLoaderYandexWeatherPrivate::tileBounds(int x, int y, int z, QPointF &tl, QPointF &br)
{
    tl = minigis::TileSystem::metersToLonLat(minigis::TileSystem::pixToMeters(QPointF((pow(2, z) - y) * minigis::TileSize, x * minigis::TileSize), z));
    br = minigis::TileSystem::metersToLonLat(minigis::TileSystem::pixToMeters(QPointF((pow(2, z) - y - 1) * minigis::TileSize, (x + 1) * minigis::TileSize), z));
}

QImage MapTileLoaderYandexWeatherPrivate::trimEmptyLines(const QImage &sourceImage)
{
    QImage destImage(sourceImage.size(), sourceImage.format());
    destImage.fill(Qt::transparent);

    int destY = 0;
    for (int y = 0; y < sourceImage.height(); ++y)
    {
        const QRgb *source = reinterpret_cast<const QRgb*>(sourceImage.scanLine(y));
        QRgb *dest = reinterpret_cast<QRgb*>(destImage.scanLine(destY));
        bool isEmpty = true;
        for (int x = 0; x < sourceImage.width(); ++x) {
            if (source[x] == 0x00000000)
                continue;

            isEmpty = false;
            dest[x] = source[x];
        }

        if (!isEmpty)
            ++destY;
    }

    if (destY == sourceImage.height())
        return sourceImage;

    QImage result(QSize(destImage.width(), destY), destImage.format());
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.drawImage(QPoint(), destImage);
    p.end();

    return result;
}

void MapTileLoaderYandexWeatherPrivate::generateTiles()
{
    foreach (const QString &quadKey, tileWeather.keys())
        generateTile(quadKey);
}

void MapTileLoaderYandexWeatherPrivate::generateTile(const QString &quadKey)
{
    TileWeather tile = tileWeather.take(quadKey);
    bool iconsReady = true;
    foreach (const CityWeather &cw, tile.cities) {
        if (!icons.contains(cw.img)) {
            iconsReady = false;
            break;
        }
    }

    if (!iconsReady) {
        tileWeather[quadKey] = tile;
        return;
    }

    static const int boobleHeight = 10;
    static const int cityW = 55;
    static const int cityH = 24 + boobleHeight;
    const int R = 5;
    const int R2 = R * 2;
    minigis::ProjSph<minigis::ParamsWGS> proj;

    QTextOption op;
    op.setAlignment(Qt::AlignCenter);
    QRectF tileRect(QPointF(), QSizeF(minigis::TileSize, minigis::TileSize));

    QImage img(QSize(minigis::TileSize, minigis::TileSize), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter pTile(&img);

    for (int i = 0; i < tile.cities.size(); ++i) {
        const CityWeather &cw = tile.cities.at(i);
        QPointF pos = minigis::TileSystem::metersToPix(proj.geoToWorld(cw.pos), tile.z) - QPoint((1 << tile.z) - 1 - tile.y, tile.x) * minigis::TileSize;
        pos = QPointF(pos.y(), 256 - pos.x());
        QPointF cityPos = pos - QPoint(cityW*.4, cityH);
        QRectF boobleRect(cityPos, QSizeF(cityW, cityH));
        if (!tileRect.intersects(boobleRect))
            continue;

        QImage city(QSize(cityW, cityH), QImage::Format_ARGB32_Premultiplied);
        city.fill(Qt::transparent);

        QRect cityRect = city.rect().adjusted(2, 2, -2, -boobleHeight);
        qreal k = qreal(cityRect.center().y()) / qreal(city.rect().height()) * 1.2;

        QPainterPath path;
        path.moveTo(cityRect.topRight() + QPoint(-R, 0));
        path.lineTo(cityRect.topLeft() + QPoint(R, 0));
        QPoint pt = cityRect.topLeft();
        path.arcTo(pt.x(), pt.y(), R2, R2, 90., 90.);
        path.lineTo(cityRect.bottomLeft() + QPoint(0, -R));
        pt = cityRect.bottomLeft() + QPoint(0, -R2);
        path.arcTo(pt.x(), pt.y(), R2, R2, 180., 90.);
        path.lineTo(cityRect.bottomLeft() + QPoint(cityRect.width() * .4, 0));
        path.lineTo(pos - cityPos);
        path.lineTo(cityRect.bottomLeft() + QPoint(cityRect.width() * .55, 0));
        path.lineTo(cityRect.bottomRight() + QPoint(-R, 0));
        pt = cityRect.bottomRight() + QPoint(-R2, -R2);
        path.arcTo(pt.x(), pt.y(), R2, R2, 270., 90.);
        path.lineTo(cityRect.topRight() + QPoint(0, R));
        pt = cityRect.topRight() + QPoint(-R2, 0);
        path.arcTo(pt.x(), pt.y(), R2, R2, 0., 90.);
        path.closeSubpath();

        QPainter p(&city);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(92, 92, 92), 1));
        QLinearGradient gradient(city.rect().topLeft(), city.rect().bottomLeft());
        gradient.setColorAt(0.00, QColor(255, 255, 255));
        gradient.setColorAt(k - .0001, QColor(255, 255, 255));
        gradient.setColorAt(k, QColor(220, 220, 220));
        gradient.setColorAt(1.00, QColor(220, 220, 220));
        p.setBrush(gradient);
        p.drawPath(path);
        const QImage &weatherIcon = icons.value(cw.img);
        p.drawImage(5, cityRect.y() + (cityRect.height() - weatherIcon.height())*.5, weatherIcon);
        QFont f = p.font();
        f.setPixelSize(12);
        f.setWeight(QFont::Light);
        p.setFont(f);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(Qt::black);
        p.drawText(cityRect.adjusted(weatherIcon.width() + 5, 2, -2, -2), QString("%1%2").arg(cw.temperature > 0 ? "+" : "").arg(cw.temperature) + QChar(0x00B0), op);
        p.end();

        pTile.drawImage(cityPos, city);
    }
    pTile.end();

    Q_Q(MapTileLoaderYandexWeather);
    emit q->imageReady(img, tile.x, tile.y, tile.z, q->type(), tile.expires);
}

// ----------------------------------------------------

MapTileLoaderYandexWeather::MapTileLoaderYandexWeather()
    : MapTileLoaderYandex(*new MapTileLoaderYandexWeatherPrivate(this))
{
    setObjectName("MapTileLoaderYandexWeather");
}

MapTileLoaderYandexWeather::~MapTileLoaderYandexWeather()
{
}

void MapTileLoaderYandexWeather::getTile(int x, int y, int z)
{
    Q_D(MapTileLoaderYandexWeather);

    QPointF tl;
    QPointF br;

    for (int i = 0; i < 9; ++i) {
        d->tileBounds(x - 1 + (i / 3), y - 1 + (i % 3), z, tl, br);
        QString request(QString("http://pogoda.yandex.ru/async/maps-forecast.xml?lt=%1,%2&rb=%3,%4&zoom=%5&day=0&lang=ru").arg(tl.x()).arg(tl.y()).arg(br.x()).arg(br.y()).arg(z/* + 2*/));
        QNetworkReply *reply = d->manager->get(QNetworkRequest(QUrl(request)));
        reply->setProperty("x", x);
        reply->setProperty("y", y);
        reply->setProperty("z", z);
        reply->setProperty("part", i);
        reply->setProperty("type", type());
        reply->setProperty("request-type", "json");
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(getError(QNetworkReply::NetworkError)));
    }
}

QString MapTileLoaderYandexWeather::description() const
{
    return QString::fromUtf8("Яндекс погода");
}

void MapTileLoaderYandexWeather::replyFinished(QNetworkReply *reply)
{
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError)
        return;

    Q_D(MapTileLoaderYandexWeather);

    QString requestType = reply->property("request-type").toString();
    if (requestType == "json") {
        QString json = QString::fromUtf8(reply->readAll().constData());
        if (!json.isEmpty()) {
            if (json.size() >= 2) {
                json.remove(0, 1);
                json.remove(json.length() - 1, 1);
            }
            QStringList cities = json.split("}, {");
            int x = reply->property("x").toInt();
            int y = reply->property("y").toInt();
            int z = reply->property("z").toInt();
            QString quadKey = minigis::TileSystem::tileToQuadKey(x, y, z);

            TileWeather tileWeather;
            tileWeather.x = x;
            tileWeather.y = y;
            tileWeather.z = z;

            bool weatherIconsReady = true;
            foreach (const QString &s, cities) {
                if (s.trimmed().isEmpty())
                    continue;
                QString city = s;
                city.replace("\"", "");
                city.replace("{", "");
                city.replace("}", "");
                QStringList params = city.split(", ");
                CityWeather cityWeather;
                bool validZoom = true;
                foreach (const QString &param, params) {
                    QStringList pair = param.split(":");
                    if (pair.size() != 2)
                        continue;

                    QString p = pair.at(0).trimmed();
                    QString v = pair.at(1).trimmed();
                    if (p == "zoom") {
                        if (z < v.toInt()) {
                            validZoom = false;
                            break;
                        }
                        continue;
                    }

                    if (p == "temperature") {
                        cityWeather.temperature = v.toInt();
                        continue;
                    }
                    if (p == "lng") {
                        cityWeather.pos.setX(v.toFloat());
                        continue;
                    }
                    if (p == "lat") {
                        cityWeather.pos.setY(v.toFloat());
                        continue;
                    }
                    if (p == "image_v2") {
                        cityWeather.img = v;
                        if (!d->icons.contains(cityWeather.img)) {
                            weatherIconsReady = false;
                            QNetworkReply *weatherIconReply = d->manager->get(QNetworkRequest(QUrl(QString("http://yandex.st/weather/1.2.1/i/icons/30x30/%1.png").arg(cityWeather.img))));
                            weatherIconReply->setProperty("request-type", "weather-icon");
                            weatherIconReply->setProperty("icon-id", cityWeather.img);
                        }
                        continue;
                    }
                }

                if (!validZoom)
                    continue;
                tileWeather.cities.append(cityWeather);
            }

            if (reply->property("part").toInt() == 4 && reply->hasRawHeader("Expires")) {
                int expires = 0;
                QDateTime dt = QLocale("en").toDateTime(reply->rawHeader("Expires").constData(), "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
                dt.setTimeSpec(Qt::UTC);
                expires = dt.toTime_t();
                tileWeather.expires = expires;
            }

            TileWeather tile = d->tileWeather.value(quadKey);
            tileWeather.parts = tile.parts + 1;
            tileWeather.cities.append(tile.cities);
            if (reply->property("part").toInt() != 4)
                tileWeather.expires = tile.expires;
            d->tileWeather[quadKey] = tileWeather;
            if (weatherIconsReady && tileWeather.parts == 9)
                d->generateTile(quadKey);
        }
    }
    else if (requestType == "weather-icon") {
        QImage img;
        img.loadFromData(reply->readAll());
        d->icons[reply->property("icon-id").toString()] = d->trimEmptyLines(img.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        d->generateTiles();
    }

    reply->deleteLater();
}

#endif

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------

class MapTileLoaderCheckFillDataBasePrivate : public MapTileLoaderPrivate
{
public:
    dc::DatabaseController *dc;
    int type;
    QList<QColor> colors;
};

// ----------------------------------------------------

MapTileLoaderCheckFillDataBase::MapTileLoaderCheckFillDataBase()
    :MapTileLoader(*new MapTileLoaderCheckFillDataBasePrivate)
{
    setObjectName("MapTileLoaderCheckFillBase");
    Q_D(MapTileLoaderCheckFillDataBase);
    d->fileFormat = "PNG";
    d->dc = NULL;
    d->type = 0;
    d->colors.append(QColor(0, 255, 0));
    d->colors.append(QColor(0, 224, 0));
    d->colors.append(QColor(0, 192, 0));
    d->colors.append(QColor(0, 160, 0));
    d->colors.append(QColor(0, 128, 0));
    d->colors.append(QColor(0,  96, 0));
    d->colors.append(QColor(0,  64, 0));
}

MapTileLoaderCheckFillDataBase::~MapTileLoaderCheckFillDataBase()
{
}

void MapTileLoaderCheckFillDataBase::init(MapLayerTile *l)
{
    Q_D(MapTileLoaderCheckFillDataBase);
    d->dc = l->dc();
}

void MapTileLoaderCheckFillDataBase::done()
{
    Q_D(MapTileLoaderCheckFillDataBase);
    d->dc = NULL;
}

void MapTileLoaderCheckFillDataBase::getTile(int x, int y, int z)
{
    Q_D(MapTileLoaderCheckFillDataBase);

    if (d->type == 0)
        return;

    QVariantMap v;
    v["quad"] = minigis::TileSystem::tileToQuadKey(x, y, z);
    v["x"] = x;
    v["y"] = y;
    v["z"] = z;
    v["type"] = d->type;
    d->dc->postRequest("selQuad", v, dc::AboveNormalPriority, this, "onDb_QuadIncome");
}

QString MapTileLoaderCheckFillDataBase::description() const
{
    return QString::fromUtf8("Слой детализации");
}

void MapTileLoaderCheckFillDataBase::setWatchType(int type)
{
    Q_D(MapTileLoaderCheckFillDataBase);
    d->type = type;
}

int MapTileLoaderCheckFillDataBase::watchType()
{
    Q_D(MapTileLoaderCheckFillDataBase);
    return d->type;
}

void MapTileLoaderCheckFillDataBase::onDb_QuadIncome(uint /*query*/, QVariant result, QVariant /*error*/)
{
    Q_D(MapTileLoaderCheckFillDataBase);
    if (result.isNull() || !result.canConvert<QVariantMap>()) {
        //        LogE(MapSettingsName) << "Failed read incomming data";
        return;
    }

    QVariantMap res = result.value<QVariantMap>();

    QByteArray imgData = res.value("tile").toByteArray();
    QImage img;
    img.loadFromData(imgData);

    if (imgData.isEmpty() || img.isNull()) {
        img = QImage(TileSize, TileSize, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
    }
    else
        img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);


    QString quadKey = res.value("quad").toString();

    QStringList quads = res.value("quads").toStringList();
    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);
    foreach (QString quad, quads) {
        QPoint tPos;
        int tZoom;
        minigis::TileSystem::quadKeyToTile(quad, tPos, tZoom);
        int tSize = TileSize >> tZoom;
        QRect r = QRect(tPos * tSize, QSize(tSize, tSize));
        painter.setPen(d->colors.at(tZoom));
        painter.drawRect(r);
    }
    painter.end();

    QPoint pos;
    int zoom;
    minigis::TileSystem::quadKeyToTile(quadKey, pos, zoom);
    emit imageReady(img, pos.x(), pos.y(), zoom, type(), 0);
}

// ----------------------------------------------------
// ----------------------------------------------------
// ----------------------------------------------------


} // minigis
