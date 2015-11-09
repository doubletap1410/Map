#include "interact/maphelpersubhelpers.h"

#include "interact/maphelperevent.h"

#include "maphelper_p.h"

// ---------------------------------------

namespace minigis {

// ---------------------------------------

MapHelperPrivate::MapHelperPrivate(QObject *parent)
    :QObject(parent)
{
}

MapHelperPrivate::~MapHelperPrivate()
{
}

void MapHelperPrivate::init(MapFrame *map)
{
    this->map = map;
    started = false;
    enabled = true;
}

// ---------------------------------------

MapHelperUniversalPrivate::MapHelperUniversalPrivate(QObject *parent)
    : MapHelperPrivate(parent)
{
    touch = NULL;
}

MapHelperUniversalPrivate::~MapHelperUniversalPrivate()
{
}

void MapHelperUniversalPrivate::init(MapFrame *map)
{
    MapHelperPrivate::init(map);
    touch = new MapHelperTouch(map, this);
}

void MapHelperUniversalPrivate::addSubhelper(MapSubHelper *sub)
{
    if (!sub)
        return;

    States st = States(sub->type());

    states |= st;
    connect(sub, SIGNAL(handler(minigis::MHEvent*)), this, SIGNAL(handler(minigis::MHEvent*)));
    connect(sub, SIGNAL(worldChanged(QTransform)), this, SLOT(worldTranform(QTransform)));
    connect(this, SIGNAL(worldChanged(QObject*, QTransform)), sub, SLOT(correctWorld(QObject*,QTransform)));
    connect(sub, SIGNAL(autoReject()), q_ptr, SLOT(finish()));
    subhelpers.insert(sub->type(), sub);

    emit stateChanged(st);
}

bool MapHelperUniversalPrivate::removeSubhelper(quint8 key)
{
    MapSubHelper *sub = subhelpers.take(key);
    if (!sub)
        return false;

    sub->reject(true);
    states &= ~key;

    delete sub;

    return true;
}

void MapHelperUniversalPrivate::worldTranform(QTransform tr)
{
    emit worldChanged(sender(), tr);
}

// ---------------------------------------

} // namespace minigis

// ---------------------------------------
