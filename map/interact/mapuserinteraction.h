#ifndef MAPUSERINTERACTION_H
#define MAPUSERINTERACTION_H

// -------------------------------------------------------

#include <QObject>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapFrame;
class MapCamera;

// -------------------------------------------------------

struct MapUserInteractionPrivate;
/**
 * @brief Сзаимодействие с пользователем
 */
class MapUserInteraction : public QObject
{
    Q_OBJECT

public:
    explicit MapUserInteraction(QObject *parent = 0);
    virtual ~MapUserInteraction();

    // установить карту
    void setMap(MapFrame *map);

protected:
    /**
     * @brief eventFilter фильтр обработки действий пользователя
     * @param obj объект
     * @param event событие
     * @return true если дальнейшая обработка события и false в противном случае
     */
    virtual bool eventFilter(QObject *obj, QEvent *event);

protected:
    explicit MapUserInteraction(MapUserInteractionPrivate &dd, QObject *parent = 0);
    MapUserInteractionPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(MapUserInteraction)
    Q_DISABLE_COPY(MapUserInteraction)
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPUSERINTERACTION_H




