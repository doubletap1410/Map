
#include "object/mapobject.h"

#include "layers/maplayer_p.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

MapLayerPrivate::MapLayerPrivate(QObject *parent)
    : QObject(parent), map(NULL), zOrder(0),
      visible(true), camera(NULL)
{

}

// -------------------------------------------------------

MapLayerWithObjectsPrivate::MapLayerWithObjectsPrivate(QObject *parent)
    : MapLayerPrivate(parent)
{

}

void MapLayerWithObjectsPrivate::objectUidChanged(QString /*newUid*/, QString oldUid)
{
    MapObject *mo = static_cast<MapObject *>(sender());
    mObjects.insert(mo->uid(), mo);
    mObjects.remove(oldUid);
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------


