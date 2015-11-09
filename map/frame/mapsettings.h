#ifndef MAPSETTINGS_H
#define MAPSETTINGS_H

#include <QObject>
#include "map/core/mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

struct MapSettingsPrivate;
class MapSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QString proxyHost READ proxyHost WRITE setProxyHost NOTIFY proxyHostChanged)
    Q_PROPERTY(quint16 proxyPort READ proxyPort WRITE setProxyPort NOTIFY proxyPortChanged)
    Q_PROPERTY(MapOptions mapOptions READ mapOptions WRITE setMapOptions NOTIFY mapOptionsChanged)
    Q_PROPERTY(bool proxy READ isProxyEnabled WRITE enableProxy NOTIFY proxyChanged)
    Q_PROPERTY(bool http READ isHttpEnabled WRITE enableHttp NOTIFY httpChanged)
    Q_PROPERTY(bool nightMode READ isNightModeEnabled WRITE enableNightMode NOTIFY nightModeChanged)
    Q_PROPERTY(bool autoNightMode READ isAutoNightMode WRITE enableAutoNightMode NOTIFY autoNightModeChanged)
    Q_PROPERTY(int minSunHour READ minSunHour WRITE setMinSunHour NOTIFY minSunHourChanged)
    Q_PROPERTY(int maxSunHour READ maxSunHour WRITE setMaxSunHour NOTIFY maxSunHourChanged)
    Q_PROPERTY(bool grid READ isGridEnabled WRITE enableGrid NOTIFY gridChanged)

public:
    explicit MapSettings(QObject *parent = 0);
    ~MapSettings();

public slots:
    qreal saturation() const;
    qreal value() const;
    QString proxyHost() const;
    quint16 proxyPort() const;
    MapOptions mapOptions() const;
    bool isProxyEnabled() const;
    bool isHttpEnabled() const;
    bool isNightModeEnabled() const;
    bool isAutoNightMode() const;
    int minSunHour() const;
    int maxSunHour() const;
    int currentSearchType() const;
    bool isSearchHelper() const;
    bool isGridEnabled() const;

    void setSaturation(qreal sat);
    void setValue(qreal val);
    void setProxyHost(const QString &host);
    void setProxyPort(quint16 port);
    void setMapOptions(MapOptions opt);
    void enableProxy(bool flag);
    void enableHttp(bool flag);
    void enableNightMode(bool flag);
    void enableAutoNightMode(bool flag);
    void setMinSunHour(int h);
    void setMaxSunHour(int h);
    void setCurrentSearchType(int type);
    void enableSearchHelper(bool flag);
    void enableGrid(bool flag);

signals:
    void saturationChanged();
    void valueChanged();
    void proxyHostChanged();
    void proxyPortChanged();
    void mapOptionsChanged();
    void proxyChanged();
    void httpChanged();
    void nightModeChanged();
    void autoNightModeChanged();
    void minSunHourChanged();
    void maxSunHourChanged();
    void currentSearchTypeChanged();
    void searchHelperChanged();
    void gridChanged();

protected:
    virtual void timerEvent(QTimerEvent *);

private:
    Q_DISABLE_COPY(MapSettings)
    Q_DECLARE_PRIVATE(MapSettings)
    MapSettingsPrivate *d_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPSETTINGS_H
