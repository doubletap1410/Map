#ifndef MAPHELPER_P_BETA_H
#define MAPHELPER_P_BETA_H

#include <QTransform>
#include <QObject>

#include "interact/maphelper.h"
#include "interact/maphelpertouch.h"

// ---------------------------------------

namespace minigis {

// ---------------------------------------

class MapSubHelper;
class MHEvent;

// ---------------------------------------

class MapHelperPrivate : public QObject
{
    Q_OBJECT

public:
    explicit MapHelperPrivate(QObject *parent = 0);
    virtual ~MapHelperPrivate();
    virtual void init(MapFrame *map);

public:
    bool enabled;
    QPointer<MapFrame> map;
    QPointer<QObject> recv;
    bool              started;

    MapHelper *q_ptr;

private:
    Q_DECLARE_PUBLIC(MapHelper)
    Q_DISABLE_COPY(MapHelperPrivate)
};

// ---------------------------------------

class MapHelperUniversalPrivate : public MapHelperPrivate
{
    Q_OBJECT

public:
    explicit MapHelperUniversalPrivate(QObject *parent = 0);
    virtual ~MapHelperUniversalPrivate();

    // инициализация хелпера
    virtual void init(MapFrame *map);

    void addSubhelper(MapSubHelper *sub);
    bool removeSubhelper(quint8 key);

signals:
    // пользовательская обработка
    void handler(minigis::MHEvent*);
    // сигнал об изменении мира
    void worldChanged(QObject *, QTransform);
    // изменился статус хелпера
    void stateChanged(States);

public slots:
    void worldTranform(QTransform);

public:
    QObject       *_recv;
    const char    *_abortSignal;
    const char    *_handlerSlot;

    States states;                          // состояние хелпера

private:
    QMap<quint8, MapSubHelper*> subhelpers; // классы включенных состояний
    MapSubHelper* currentHelper;

    MapHelperTouch *touch;

private:
    Q_DECLARE_PUBLIC(MapHelperUniversal)
    Q_DISABLE_COPY(MapHelperUniversalPrivate)
};

// ---------------------------------------

} //namespace minigis

// ---------------------------------------

#endif // MAPHELPER_P_BETA_H
