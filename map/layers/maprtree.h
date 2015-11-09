#ifndef MAPRTREE_H
#define MAPRTREE_H

#include <QObject>
#include <QRectF>
#include <QPolygonF>
#include <QPointer>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapObject;
typedef QPointer<MapObject> MapObjectPtr;
typedef QList<MapObjectPtr> MapObjectList;

struct Node {
    Node(QRectF r = QRectF(), Node *_parent = NULL, QList<Node*>list = QList<Node*>()) : rect(r), parent(_parent), nodes(list) {}
    int level;
    QRectF rect;

    Node *parent;
    QList<Node*> nodes;

    MapObjectList objects;
};

class MapRTreePrivate;
class MapRTree : public QObject
{
    Q_OBJECT
public:
    explicit MapRTree(QObject *parent = 0);
    virtual ~MapRTree();

    /**
     * @brief clear очистить все дерево
     */
    void clear();

    /**
     * @brief insert вставка объекта в дерево
     * @param obj объект
     * @return результат выполнения операции
     */
    bool insert(MapObject *mo);

    /**
     * @brief objectsInLeaf поиск объектов попавших в область
     * @param r область
     * @return список объектов
     */
    QList<MapObject *> find(const QRectF &r) const;

    /**
     * @brief objectsInLeaf поиск объектов попавших в область
     * @param p область
     * @return список объектов
     */
    QList<MapObject *> find(const QPolygonF &p) const;

    /**
     * @brief numFeature вывод основных характеристик дерева
     */
    void numFeature();

    // ============================

    /**
     * @brief allLeafs список всех узлов дерева
     * @return
     */
    QList<Node *> allLeafs() const;
    /**
     * @brief treeRoot корень дерева
     * @return
     */
    Node *treeRoot() const;
    
signals:

public slots:
    /**
     * @brief lock заблокировать изменение дерева
     * (для удаление и перемещения большого количества обектов)
     * Важно! необходимо разблокировать дерево
     */
    void lock();
    /**
     * @brief unlock разблокировать и перестроить дерево
     */
    void unlock();

    /**
     * @brief moveObj объект переместился
     * @param obj объект
     */
    void movedObj(MapObject *obj = NULL);

    /**
     * @brief deleteObj удаление объекта(объектов) из дерева
     * @param obj объект(список объектов)
     */
    void deleteObj(QObject *obj);
    void deleteObj(MapObject *obj);
    void deleteObj(QList<MapObject*> const &obj);

private:
    /**
     * @brief blockingMoveObject перемещение объектов в дереве без перестроения дерева
     * @param mo объект
     */
    void blockingMoveObject(MapObject *mo);
    /**
     * @brief nonblockingMoveObject перемещение объектов в дереве
     * @param mo обьъект
     */
    void nonblockingMoveObject(MapObject *mo);

    /**
     * @brief unblockMove перестроить дерево после перемещения объектов
     */
    void unblockMove();

    /**
     * @brief blockingInsertObject вставка объектов в дерево без перестроения дерева
     * @param mo объект
     * @return результат
     */
    bool blockingInsertObject(MapObject *mo);
    /**
     * @brief nonblockingInsertObject вставка объектов в дерево
     * @param mo объект
     * @return результат
     */
    bool nonblockingInsertObject(MapObject *mo);

    /**
     * @brief unblockInsert перестроить дерево после вставки объектов
     */
    void unblockInsert();

    /**
     * @brief blockingDeleteObjects удаление объектов из дерева без перестроения дерева
     * @param obj список объектов
     */
    void blockingDeleteObject(QList<MapObject*> const &obj);
    void blockingDeleteObject(MapObject *mo);
    /**
     * @brief nonblockingDeleteObjects удаление объекта из дерева
     * @param obj объект
     */
    void nonblockingDeleteObject(MapObject *mo);

    /**
     * @brief unblockDelete перестроить дерево после удаления объектов
     */
    void unblockDelete();

private:
    Q_DECLARE_PRIVATE(MapRTree)
    Q_DISABLE_COPY(MapRTree)

    MapRTreePrivate *d_ptr;
};

// -------------------------------------------------------

} // minigis

// -------------------------------------------------------

#endif // MAPRTREE_H
