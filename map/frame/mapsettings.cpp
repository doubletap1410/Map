
#include <QVariantMap>
#include <QBasicTimer>
#include <QTimerEvent>
#include <QTime>

#include "frame/mapsettings.h"

// -------------------------------------------------------

namespace minigis {
// -------------------------------------------------------
// -------------------------------------------------------

struct MapSettingsPrivate
{
    qreal saturation;
    qreal value;

    QString proxyHost;
    quint16  proxyPort;
    bool proxyEnable;
    bool httpEnable;

    MapOptions options;

    QBasicTimer nightModeTimer;
    int minSunHour;
    int maxSunHour;
    bool autoNightNode;
    bool nightMode;

    int currentSearchType;
    bool searchHelper;

    bool useGrid;
};

// ----------------------------------------------

MapSettings::MapSettings(QObject *parent)
    : QObject(parent), d_ptr(new MapSettingsPrivate())
{
    Q_D(MapSettings);
    d->proxyHost = "";
    d->proxyPort = 0;
    d->proxyEnable = false;
    d->httpEnable = true;
    d->nightMode = false;

    d->currentSearchType = 0;
    d->searchHelper = true;

    d->useGrid = true;
}

MapSettings::~MapSettings()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = 0;
    }
}

qreal MapSettings::saturation() const
{
    Q_D(const MapSettings);
    return d->saturation;
}

qreal MapSettings::value() const
{
    Q_D(const MapSettings);
    return d->value;
}

QString MapSettings::proxyHost() const
{
    Q_D(const MapSettings);
    return d->proxyHost;
}

quint16 MapSettings::proxyPort() const
{
    Q_D(const MapSettings);
    return d->proxyPort;
}

MapOptions MapSettings::mapOptions() const
{
    Q_D(const MapSettings);
    return d->options;
}

bool MapSettings::isProxyEnabled() const
{
    Q_D(const MapSettings);
    return d->proxyEnable;
}

bool MapSettings::isHttpEnabled() const
{
    Q_D(const MapSettings);
    return d->httpEnable;
}

bool MapSettings::isNightModeEnabled() const
{
    Q_D(const MapSettings);
    return d->nightMode;
}

bool MapSettings::isAutoNightMode() const
{
    Q_D(const MapSettings);
    return d->autoNightNode;
}

int MapSettings::minSunHour() const
{
    Q_D(const MapSettings);
    return d->minSunHour;
}

int MapSettings::maxSunHour() const
{
    Q_D(const MapSettings);
    return d->maxSunHour;
}

int MapSettings::currentSearchType() const
{
    Q_D(const MapSettings);
    return d->currentSearchType;
}

bool MapSettings::isSearchHelper() const
{
    Q_D(const MapSettings);
    return d->searchHelper;
}

bool MapSettings::isGridEnabled() const
{
    Q_D(const MapSettings);
    return d->useGrid;
}

void MapSettings::setSaturation(qreal sat)
{
    Q_D(MapSettings);
    sat = qBound<double>(-1., sat, 1.);
    if (d->saturation != sat) {
        d->saturation = sat;
        emit saturationChanged();
    }
}

void MapSettings::setValue(qreal val)
{
    Q_D(MapSettings);
    val = qBound<double>(-1., val, 1.);
    if (d->value != val) {
        d->value = val;
        emit valueChanged();
    }
}

void MapSettings::setProxyHost(const QString &host)
{
    Q_D(MapSettings);
    if (d->proxyHost != host) {
        d->proxyHost = host;
        emit proxyHostChanged();
    }
}

void MapSettings::setProxyPort(quint16 port)
{
    Q_D(MapSettings);
    if (d->proxyPort != port) {
        d->proxyPort = port;
        emit proxyPortChanged();
    }
}

void MapSettings::setMapOptions(MapOptions opt)
{
    Q_D(MapSettings);
    if (d->options != opt) {
        d->options = opt;
        emit mapOptionsChanged();
    }
}

void MapSettings::enableProxy(bool flag)
{
    Q_D(MapSettings);
    if (d->proxyEnable != flag) {
        d->proxyEnable = flag;
        emit proxyChanged();
    }
}

void MapSettings::enableHttp(bool flag)
{
    Q_D(MapSettings);
    if (d->httpEnable != flag) {
        d->httpEnable = flag;
        emit httpChanged();
    }
}

void MapSettings::enableNightMode(bool flag)
{
    Q_D(MapSettings);
    if (d->nightMode != flag) {
        d->nightMode = flag;
        emit nightModeChanged();
    }
}

void MapSettings::enableAutoNightMode(bool flag)
{
    Q_D(MapSettings);
    if (d->autoNightNode != flag) {
        d->autoNightNode = flag;
        d->nightModeTimer.stop();
        if (d->autoNightNode) {
            QTime t = QTime::currentTime();
            enableNightMode((d->minSunHour < d->maxSunHour) ?
                                (t.hour() < d->minSunHour || t.hour() > d->maxSunHour) :
                                (t.hour() <= d->minSunHour && t.hour() >= d->maxSunHour));
            int waitMinutes = 60 - t.minute();
            d->nightModeTimer.start(waitMinutes * 60 * 1000, this);
        }
        emit autoNightModeChanged();
    }
}

void MapSettings::setMinSunHour(int h)
{
    Q_D(MapSettings);
    if (d->minSunHour != h) {
        d->minSunHour = h;
        emit minSunHourChanged();
    }
}

void MapSettings::setMaxSunHour(int h)
{
    Q_D(MapSettings);
    if (d->maxSunHour != h) {
        d->maxSunHour = h;
        emit maxSunHourChanged();
    }
}

void MapSettings::setCurrentSearchType(int type)
{
    Q_D(MapSettings);
    if (d->currentSearchType != type) {
        d->currentSearchType = type;
        emit currentSearchTypeChanged();
    }
}

void MapSettings::enableSearchHelper(bool flag)
{
    Q_D(MapSettings);
    if (d->searchHelper != flag) {
        d->searchHelper = flag;
        emit searchHelperChanged();
    }
}

void MapSettings::enableGrid(bool flag)
{
    Q_D(MapSettings);
    if (d->useGrid != flag) {
        d->useGrid = flag;
        emit gridChanged();
    }
}

void MapSettings::timerEvent(QTimerEvent */*e*/)
{
#if 0
    Q_D(MapSettings);
    if (e->timerId() == d->nightModeTimer.timerId()) {
        QTime t = QTime::currentTime();
        enableNightMode(d->minSunHour < d->maxSunHour ?
                            t.hour() < d->minSunHour || t.hour() > d->maxSunHour :
                            t.hour() <= d->minSunHour && t.hour() >= d->maxSunHour);
        d_ptr->nightModeTimer.stop();
        d_ptr->nightModeTimer.start(3600000, this); // раз в час
    }
#endif
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
