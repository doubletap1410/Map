
#include <QStack>
#include <QDebug>
#include <QMap>

#include "object/mapobject.h"
#include "drawer/mapdrawer.h"
#include "drawer/mapdrawercommon.h"

#include "layers/maprtree.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapRTreePrivate
{
public:
    explicit MapRTreePrivate();

    Node* findObj(MapObject *obj);                // поиск в дереве

    Node* chooseLeaf(const QRectF &r);            // выбор листа + изменение размеров листа и всех его родителей

    void adjustBounds(Node *n, const QRectF &r);  // изменяет размер листа
    qreal adjustValue(const Node *n, const QRectF &r);  // на сколько изменится размер листа
    void resizeBoundsObj(Node *n);
    void resizeBoundsNodes(Node *n);

    void splitNode(Node *n);                      // разделение листа
    void splitDownLevel(Node *n);                 // создание нового уровня дерева
    void splitSameLevel(Node *n);                 // разделение текущего уровня дерева
    void split(MapObjectList objects, Node *right, Node *left); // алгоритм создания новых узлов

    // поиск листьев в дереве
    QList<Node *> findPoint(const QPointF &p) const;
    QList<Node *> findRect(const QRectF &r) const;
    QList<Node *> findPoly(const QPolygonF &p) const;

    Node *root;                                   // корень
    QMap<MapObject*, Node*> cache;                // объект - лист
    QList<Node*> allNodes;                        // список все узлов дерева
    static const int minNodes = 2;                // не явно используется в функции split
    static const int maxNodes = 25;               // максимальное количество элементов в узле

    QSet<MapObject *> insertList;                 // набор вставляемых объектов
    QSet<MapObject *> moveList;                   // набор перемещаемых объектов
    QSet<Node*> deleteList;                       // набор измененных узлов при удалении

    bool locked;                                  // блокирует перстроение дерева

    MapRTree *q_ptr;

private:
    Q_DECLARE_PUBLIC(MapRTree)
    Q_DISABLE_COPY(MapRTreePrivate)
};

// -------------------------------------------------------

MapRTreePrivate::MapRTreePrivate() :
    root(NULL), cache(QMap<MapObject*, Node*>()), allNodes(QList<Node*>()), locked(false)
{    
}

Node *MapRTreePrivate::findObj(MapObject *obj)
{
    foreach (Node *n, findRect(obj->drawer()->boundRect(obj))) {
        if (n->objects.contains(obj))
            return n;
    }
    return NULL;
}

Node *MapRTreePrivate::chooseLeaf(const QRectF &r)
{
    adjustBounds(root, r);
    QStack<Node*> stack;
    stack.append(root);
    while (!stack.isEmpty()) {
        Node* leaf = stack.pop();
        if (leaf->nodes.isEmpty())
            return leaf;

        Node *node = leaf->nodes.first();
        qreal min = adjustValue(node, r);
        for (QListIterator<Node*> it(leaf->nodes); it.hasNext();) {
            Node *n = it.next();
            qreal sq = adjustValue(n, r);
            if (sq < min) {
                min = sq;
                node = n;
                if (qFuzzyIsNull(min))
                    break;
            }
        }
        adjustBounds(node, r);
        stack.append(node);
    }
    return NULL;
}

void MapRTreePrivate::adjustBounds(Node *n, const QRectF &r)
{
    if (!n->rect.contains(r))
        n->rect = QRectF(QPointF(
                             qMin(n->rect.left(), r.left()),
                             qMin(n->rect.top(), r.top())
                             ),
                         QPointF(
                             qMax(n->rect.right(), r.right()),
                             qMax(n->rect.bottom(), r.bottom()))
                         );
}

qreal MapRTreePrivate::adjustValue(Node const *n, const QRectF &r)
{
    if (n->rect.contains(r))
        return 0;

    QRectF rAd(QPointF(
                   qMin(n->rect.left(), r.left()),
                       qMin(n->rect.top(), r.top())
                   ),
               QPointF(
                   qMax(n->rect.right(), r.right()),
                   qMax(n->rect.bottom(), r.bottom()))
               );
    return qAbs(rAd.width() * rAd.height() - r.width() * r.height());
}

void MapRTreePrivate::resizeBoundsObj(Node *n)
{
    if (n->objects.isEmpty())
        return;

    MapObjectPtr mo = n->objects.first();
    QRectF rect(mo->drawer()->boundRect(mo));
    qreal l = rect.left();
    qreal t = rect.top();
    qreal r = rect.right();
    qreal b = rect.bottom();
    foreach (MapObjectPtr p, n->objects) {
        rect = p->drawer()->boundRect(p.data());
        l = qMin(l, rect.left());
        t = qMin(t, rect.top());
        r = qMax(r, rect.right());
        b = qMax(b, rect.bottom());
    }
    n->rect = QRectF(QPointF(l, t), QPointF(r, b));
}

void MapRTreePrivate::resizeBoundsNodes(Node *n)
{
    if (n->nodes.isEmpty())
        return;

    QRectF rect(n->nodes.first()->rect);
    qreal l = rect.left();
    qreal t = rect.top();
    qreal r = rect.right();
    qreal b = rect.bottom();

    foreach (Node *p, n->nodes) {
        l = qMin(l, p->rect.left());
        t = qMin(t, p->rect.top());
        r = qMax(r, p->rect.right());
        b = qMax(b, p->rect.bottom());
    }
    n->rect = QRectF(QPointF(l, t), QPointF(r, b));
}

void MapRTreePrivate::splitNode(Node *n)
{
    if (n->nodes.size() > maxNodes || (n->objects.size() > maxNodes && n == root))
        splitDownLevel(n);
    else if (n->objects.size() > maxNodes && n != root) {
        if (n->parent->nodes.size() + 1 > maxNodes)
            splitDownLevel(n);
        else
            splitSameLevel(n);
    }
}

void MapRTreePrivate::splitDownLevel(Node *n)
{
    Node *left = new Node();
    Node *right= new Node();

    left->parent  = n;
    right->parent = n;

    left->level  = n->level + 1;
    right->level = n->level + 1;

    split(n->objects, right, left);

    n->nodes.append(left);
    n->nodes.append(right);

    n->objects.clear();

    allNodes.append(left);
    allNodes.append(right);
}

void MapRTreePrivate::splitSameLevel(Node *n)
{
    Node *parent = n->parent;

    Node *left  = new Node();
    Node *right = new Node();

    left->parent  = parent;
    right->parent = parent;

    left->level  = n->level;
    right->level = n->level;

    split(n->objects, right, left);

    parent->nodes.removeOne(n);

    parent->nodes.append(left);
    parent->nodes.append(right);

    allNodes.removeOne(n);
    n->objects.clear();
    n->nodes.clear();
    n->parent = NULL;
    delete n;
    n = NULL;

    allNodes.append(left);
    allNodes.append(right);
}

void MapRTreePrivate::split(MapObjectList objects, Node *right, Node *left)
{
    if (objects.size() <= maxNodes)
        return;

    // выбираем начальные точки для новых листов
    qreal maxdist = 0.;
    int lNum = -1;
    int rNum = -1;
    for (int i = 0; i < objects.size() - 1; ++i) {
        MapObjectPtr oOne(objects.at(i));
        QRectF rectOne(oOne->drawer()->boundRect(oOne));

        for (int j = i; j < objects.size(); ++j) {
            MapObjectPtr oTwo(objects.at(j));
            QRectF rectTwo(oTwo->drawer()->boundRect(oTwo));

            qreal len = qMax(
                        QLineF(rectOne.topLeft(), rectTwo.bottomRight()).length(),
                        QLineF(rectOne.bottomLeft(), rectTwo.topRight()).length()
                        );

            if (len > maxdist) {
                maxdist = len;
                lNum = i;
                rNum = j;
            }
        }
    }
    if (lNum != -1 && rNum != -1) {
        if (lNum > rNum) {
            MapObjectPtr p = objects.takeAt(lNum);
            left->objects.append(p);
            left->rect = p->drawer()->boundRect(p);
            cache[p] = left;

            p = objects.takeAt(rNum);
            right->objects.append(p);            
            right->rect = p->drawer()->boundRect(p);
            cache[p] = right;
        }
        else {
            MapObjectPtr p = objects.takeAt(rNum);
            right->objects.append(p);
            right->rect = p->drawer()->boundRect(p);
            cache[p] = right;

            p = objects.takeAt(lNum);
            left->objects.append(p);
            left->rect = p->drawer()->boundRect(p);
            cache[p] = left;
        }
    }

    // ---------
    while (!objects.isEmpty()) {
        MapObjectPtr p = objects.takeFirst();
        if (objects.size() == 1) {
            if (right->objects.size() == 0) {
                right->objects.append(p);
                adjustBounds(right, p->drawer()->boundRect(p));
                cache[p] = right;
                continue;
            }
            if (left->objects.size() == 0) {
                left->objects.append(p);
                adjustBounds(left, p->drawer()->boundRect(p));
                cache[p] = left;
                continue;
            }
        }

        if (objects.isEmpty()) {
            if (right->objects.size() == 1) {
                right->objects.append(p);
                adjustBounds(right, p->drawer()->boundRect(p));
                cache[p] = right;
                continue;
            }
            if (left->objects.size() == 1) {
                left->objects.append(p);
                adjustBounds(left, p->drawer()->boundRect(p));
                cache[p] = left;
                continue;
            }
        }

        qreal lSize = adjustValue(left, p->drawer()->boundRect(p));
        qreal rSize = adjustValue(right, p->drawer()->boundRect(p));
        if (lSize < rSize) {
            if (left->objects.size() < maxNodes) {
                left->objects.append(p);
                adjustBounds(left, p->drawer()->boundRect(p));
                cache[p] = left;
            }
            else {
                right->objects.append(p);
                adjustBounds(right, p->drawer()->boundRect(p));
                cache[p] = right;
            }
        }
        else {
            if (right->objects.size() < maxNodes) {
                right->objects.append(p);
                adjustBounds(right, p->drawer()->boundRect(p));
                cache[p] = right;
            }
            else {
                left->objects.append(p);
                adjustBounds(left, p->drawer()->boundRect(p));
                cache[p] = left;
            }
        }
    }
}


QList<Node *> MapRTreePrivate::findPoint(const QPointF &p) const
{
    QList<Node*> endList;

    QStack<Node*> stack;
    stack.append(root);
    while (!stack.isEmpty()) {
        Node* leaf = stack.pop();
        if (leaf->nodes.isEmpty()) {
            endList.append(leaf);
            continue;
        }

        for (int i = 0; i < leaf->nodes.size(); ++i)
            if (leaf->nodes.at(i)->rect.contains(p))
                stack.append(leaf->nodes.at(i));
    }
    return endList;
}

QList<Node *> MapRTreePrivate::findRect(const QRectF &r) const
{
    if (allNodes.isEmpty())
        return QList<Node *>();

    if (r.size().isEmpty())
        return findPoint(r.topLeft());

    QList<Node*> endList;

    QStack<Node*> stack;
    stack.append(root);
    while (!stack.isEmpty()) {
        Node* leaf = stack.pop();
        if (leaf->nodes.isEmpty()) {
            endList.append(leaf);
            continue;
        }

        for (int i = 0; i < leaf->nodes.size(); ++i) {
            if (leaf->nodes.at(i)->rect.intersects(r))
                stack.append(leaf->nodes.at(i));
        }
    }

    return endList;
}

QList<Node *> MapRTreePrivate::findPoly(const QPolygonF &p) const
{
    if (allNodes.isEmpty())
        return QList<Node *>();

    if (p.isEmpty())
        return QList<Node *>();
    if (p.size() == 1)
        return findPoint(p.first());

    QList<Node*> endList;

    QStack<Node*> stack;
    stack.append(root);
    while (!stack.isEmpty()) {
        Node* leaf = stack.pop();
        if (leaf->nodes.isEmpty())
            endList.append(leaf);
        else
            for (int i = 0; i < leaf->nodes.size(); ++i)
                if (rectIntersectPolyFull(leaf->nodes.at(i)->rect, p))
                    stack.append(leaf->nodes.at(i));
                else {
                    MapObjectList moList = leaf->nodes.at(i)->objects;
                    for (int i = 0; i < moList.size(); ++i) {
                        MapObject *obj = moList.at(i).data();
                        obj->drawer()->hide(obj);
                    }
                }
    }
    return endList;
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

MapRTree::MapRTree(QObject *parent) :
    QObject(parent), d_ptr(new MapRTreePrivate)
{
    d_ptr->q_ptr = this;
}

MapRTree::~MapRTree()
{
}

void MapRTree::clear()
{
    Q_D(MapRTree);
    for (QSetIterator<MapObject*> it(d->insertList); it.hasNext(); ) {
        MapObject *mo = it.next();
        disconnect(mo, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObj(QObject*)));
        disconnect(mo, SIGNAL(classCodeChanged(QString)), this, SLOT(movedObj()));
    }
    d->insertList.clear();
    d->moveList.clear();

    for (QMapIterator<MapObject*, Node*> it(d->cache); it.hasNext(); ) {
        MapObject *mo = it.next().key();
        disconnect(mo, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObj(QObject*)));
        disconnect(mo, SIGNAL(classCodeChanged(QString)), this, SLOT(movedObj()));
    }
    d->cache.clear();
    d->root = NULL;

    qDeleteAll(d->allNodes);
    d->allNodes.clear();
}

bool MapRTree::insert(MapObject *mo)
{
    Q_D(MapRTree);
    return d->locked ? blockingInsertObject(mo) : nonblockingInsertObject(mo);
}

void MapRTree::deleteObj(QObject *obj)
{
    if (!obj)
        return;

    MapObject *mo = qobject_cast<MapObject *>(obj);
    deleteObj(mo);
}

void MapRTree::deleteObj(MapObject *mo)
{
   Q_D(MapRTree);
   d->locked ? blockingDeleteObject(mo) : nonblockingDeleteObject(mo);
}

void MapRTree::deleteObj(const QList<MapObject *> &obj)
{
    Q_D(MapRTree);
    blockingDeleteObject(obj);
    if (!d->locked)
        unblockDelete();
}

void MapRTree::blockingMoveObject(MapObject *mo)
{
    Q_D(MapRTree);
    if (!mo)
        return;
    d->moveList.insert(mo);
}

void MapRTree::nonblockingMoveObject(MapObject *mo)
{
    if (!mo)
        return;

    nonblockingDeleteObject(mo);
    nonblockingInsertObject(mo);
}

void MapRTree::unblockMove()
{
    Q_D(MapRTree);
    if (d->moveList.isEmpty())
        return;

    QSet<Node*> resizeNodes;
    QList<MapObject*> list;
    foreach (MapObject *mo, d->moveList) {
        if (!mo || !mo->drawer())
            continue;

        Node *par = d->cache.value(mo, NULL);
        if (!par)
            return;

        if (!par->rect.contains(mo->drawer()->boundRect(mo)))
            list.append(mo);
        else
            resizeNodes.insert(par);
    }
    d->moveList.clear();

    foreach (Node *n, resizeNodes)
        d->resizeBoundsObj(n);

    blockingDeleteObject(list);
    for (QListIterator<MapObject*> it(list); it.hasNext(); )
        blockingInsertObject(it.next());
}

bool MapRTree::blockingInsertObject(MapObject *mo)
{
    Q_D(MapRTree);
    if (!mo)
        return false;

    d->insertList.insert(mo);

    connect(mo, SIGNAL(destroyed(QObject*)), SLOT(deleteObj(QObject*)), Qt::UniqueConnection);
    connect(mo, SIGNAL(classCodeChanged(QString)), SLOT(movedObj()), Qt::UniqueConnection);

    return true;
}

bool MapRTree::nonblockingInsertObject(MapObject *mo)
{
    Q_D(MapRTree);
    if (!mo)
        return false;

    connect(mo, SIGNAL(destroyed(QObject*)), SLOT(deleteObj(QObject*)), Qt::UniqueConnection);
    connect(mo, SIGNAL(classCodeChanged(QString)), SLOT(movedObj()), Qt::UniqueConnection);

    if (!mo->drawer())
        return true;

    if (d->allNodes.isEmpty()) {
        d->root = new Node(mo->drawer()->boundRect(mo));
        d->root->level = 0;
        d->cache[mo] = d->root;
        d->allNodes.append(d->root);
    }

    Node *n = d->chooseLeaf(mo->drawer()->boundRect(mo));
    if (!n) {
        qDebug() << "Error choose Leaf";
        return false;
    }

    n->objects.append(mo);
    d->cache[mo] = n;

    if (n->objects.size() > d->maxNodes)
        d->splitNode(n);

    return true;
}

void MapRTree::unblockInsert()
{
    Q_D(MapRTree);
    if (d->insertList.isEmpty())
        return;

    bool empty = d->allNodes.isEmpty();
    for (QSetIterator<MapObject*> it(d->insertList); it.hasNext(); ) {
        MapObject *mo = it.next();
        Q_ASSERT(mo);

        if (!mo->drawer())
            continue;

        if (empty) {
            d->root = new Node(mo->drawer()->boundRect(mo));
            d->root->level = 0;

            d->cache[mo] = d->root;

            d->allNodes.append(d->root);
            empty = false;
        }

        Node *n = d->chooseLeaf(mo->drawer()->boundRect(mo));

        if (!n) {
            qDebug() << "Error choose Leaf; " << " object" << mo;
            continue;
        }

        n->objects.append(mo);
        d->cache[mo] = n;

        if (n->objects.size() > d->maxNodes)
            d->splitNode(n);
    }
    d->insertList.clear();
}

void MapRTree::blockingDeleteObject(const QList<MapObject *> &obj)
{
    for (QListIterator<MapObject*> it(obj); it.hasNext(); )
        blockingDeleteObject(it.next());
}

void MapRTree::blockingDeleteObject(MapObject *mo)
{
    Q_D(MapRTree);
    if (!mo)
        return;

    d->insertList.remove(mo);
    d->moveList.remove(mo);

    disconnect(mo, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObj(QObject*)));
    disconnect(mo, SIGNAL(classCodeChanged(QString)), this, SLOT(movedObj()));

    Node *n = d->cache.take(mo);
    if (!n)
        return;
    n->objects.removeOne(mo);
    d->deleteList.insert(n);
}

void MapRTree::nonblockingDeleteObject(MapObject *mo)
{
    Q_D(MapRTree);
    Node *n = d->cache.take(mo);
    if (!n)
        return;

    disconnect(mo, SIGNAL(destroyed(QObject*)), this, SLOT(deleteObj(QObject*)));
    disconnect(mo, SIGNAL(classCodeChanged(QString)), this, SLOT(movedObj()));

    // удаление объекта и изменение размера листа
    n->objects.removeOne(mo);
    d->resizeBoundsObj(n);

    // изменение размеров родителей
    Node *par = n->parent;
    while (par) {
        d->resizeBoundsNodes(par);
        par = par->parent;
    }

    // перестроение дерева
    if (n->objects.size() < 2) {
        MapObjectList obj = n->objects;
        foreach (MapObjectPtr mobj, obj)
            d->cache.remove(mobj.data());
        n->objects.clear();

        Node *par = NULL;
        if (n->parent) {
            par = n->parent;
            n->parent->nodes.removeOne(n);
            n->parent = NULL;
        }

        d->allNodes.removeOne(n);
        delete n;

        if (par && par->nodes.size() == 1) {
            Node *child = par->nodes.first();
            while (child) {
                if (!child->objects.isEmpty()) {
                    par->objects = child->objects;
                    foreach (MapObject *mobj, par->objects)
                        d->cache[mobj] = par;

                    d->allNodes.removeOne(child);

                    par->nodes.clear();
                    child->parent = NULL;

                    child->objects.clear();
                    delete child;
                    child = NULL;
                }
                else if (!child->nodes.isEmpty()) {
                    par->nodes = child->nodes;
                    foreach (Node *nod, par->nodes) {
                        if (nod)
                            nod->parent = par;
                    }

                    child->nodes.clear();
                    d->allNodes.removeOne(child);
                    child->parent = NULL;
                    delete child;
                    child = NULL;

                    if (par->nodes.size() == 1)
                        child = par->nodes.first();
                }
            }
        }

        if (obj.size() == 1)
            insert(obj.first());
    }
}

void MapRTree::unblockDelete()
{
    Q_D(MapRTree);
    if (d->deleteList.isEmpty())
        return;

    MapObjectList objList;

    QSet<Node*>::iterator itNode;
    while (!d->deleteList.isEmpty()) {
        itNode = d->deleteList.begin();
        Node *n = *itNode;
        d->deleteList.erase(itNode);

        if (n->objects.isEmpty()) { // если список объектов пуст
            // удаляем пустой нод
            Node *par = NULL;
            if (n->parent) {
                par = n->parent;
                par->nodes.removeOne(n);
                n->parent = NULL;
            }

            if (!par)
                continue;

            d->allNodes.removeOne(n);
            delete n;

            if (par->nodes.size() == 1) { // если остался один нод
                Node *parpar = par->parent;
                if (parpar) { // перемещаем лист на уровень выше
                    parpar->nodes.removeOne(par);
                    foreach (Node *child, par->nodes)
                        child->parent = parpar;
                    parpar->nodes.append(par->nodes);

                    d->allNodes.removeOne(par);
                    delete par;

                    // изменяем размер ветки
                    d->resizeBoundsObj(parpar);
                    Node *up = parpar->parent;
                    while (up) {
                        d->resizeBoundsObj(up);
                        up = up->parent;
                    }
                }
                else { // перемещаем список объектов/нодов
                    Node *child = par->nodes.first();
                    if (!child->nodes.isEmpty()) {
                        par->nodes = child->nodes;
                        foreach (Node *tmp, child->nodes)
                            tmp->parent = par;
                        child->nodes.clear();
                    }
                    else {
                        par->objects.append(child->objects);
                        foreach (MapObjectPtr mobj, child->objects)
                            d->cache[mobj.data()] = par;
                        par->nodes.clear();
                        child->objects.clear();
                    }
                    child->parent = NULL;

                    d->deleteList.remove(par);
                    d->allNodes.removeOne(child);
                    delete child;
                }
            }
            else {
                // изменяем размер ветки
                d->resizeBoundsObj(par);
                for (Node *up = par->parent; up; up = up->parent)
                    d->resizeBoundsObj(up);
            }
        }
        else if (n->objects.size() >= 1) { // если список объектов не пуст
            // изменяем размер всей ветки
            d->resizeBoundsObj(n);
            Node *par = n->parent;
            while (par) {
                d->resizeBoundsNodes(par);
                par = par->parent;
            }

            if (n->objects.size() == 1) {
                // если остался только один объект, перемещаем его в отдельный список
                // ноду возвращаем в список измененных нодов
                MapObjectList obj = n->objects;
                foreach (MapObjectPtr mobj, obj)
                    d->cache.remove(mobj.data());
                n->objects.clear();

                objList.append(obj);
                d->deleteList.insert(n);
            }
        }
    }

    foreach (MapObjectPtr mo, objList)
        nonblockingInsertObject(mo);
}

void MapRTree::movedObj(MapObject *mo)
{
    Q_D(MapRTree);
    if (!mo) {
        mo = qobject_cast<MapObject*>(sender());
        if (!mo)
            return;
    }
    d->locked ? blockingMoveObject(mo) : nonblockingMoveObject(mo);
}

QList<MapObject *> MapRTree::find(const QRectF &r) const
{
    Q_D(const MapRTree);
    QList<MapObject*> list;
    QList<Node *> nodes = d->findRect(r);
    list.reserve(nodes.size() * qSqrt(d->maxNodes));

    foreach (Node *n, nodes) {
        for (int i = 0; i < n->objects.size(); ++i) {
            MapObject *obj = n->objects.at(i).data();
            if (r.intersects(obj->drawer()->boundRect(obj)))
                list.append(obj);
        }
    }
    return list;
}

QList<MapObject *> MapRTree::find(const QPolygonF &p) const
{
    Q_D(const MapRTree);
    QList<MapObject *> list;
    QList<Node *> nodes = d->findPoly(p);
    list.reserve(nodes.size() * qSqrt(d->maxNodes));

    foreach (Node *n, nodes) {
        for (int i = 0; i < n->objects.size(); ++i) {
            MapObject *obj = n->objects.at(i).data();
            if (rectIntersectPolyFull(obj->drawer()->boundRect(obj), p))
                list.append(obj);
            else
                obj->drawer()->hide(obj);
        }
    }
    return list;
}

void MapRTree::numFeature()
{
    Q_D(MapRTree);

    QList<Node*> leafs;
    foreach (Node *n, allLeafs()) {
        if (n->nodes.isEmpty())
            leafs.append(n);
        else if (!n->objects.isEmpty())
            qDebug() << "Objects error";
    }
    if (leafs.isEmpty()) {
        qDebug() << "empty tree";
        return;
    }

    int maxLevel = 0;
    qreal midLevel = 0;

    int minObj = 1000;
    int maxObj = -1;
    int sumObj = 0;
    foreach (Node *n, leafs) {
        int level = n->level;
        if (level > maxLevel)
            maxLevel = level;
        midLevel += level;

        int objSize = n->objects.size();

        if (objSize > maxObj)
            maxObj = objSize;
        if (objSize < minObj)
            minObj = objSize;

        sumObj += objSize;
    }
    midLevel /= leafs.size();
    qreal midObj = qreal(sumObj) / qreal(leafs.size());

    qDebug() << "max level: " << maxLevel << "; mid level: " << midLevel
             << "; min objects: " << minObj << "; max objects: " << maxObj << "; mid objects: " << midObj
             << "; objects: " << sumObj << "; leafs count: " << leafs.size();

    qDebug() << "in cache: " << d->cache.uniqueKeys().size() << d->cache.keys().size();
}

QList<Node *> MapRTree::allLeafs() const
{
    Q_D(const MapRTree);
    return d->allNodes;
}

Node *MapRTree::treeRoot() const
{
    Q_D(const MapRTree);
    return d->root;
}

void MapRTree::lock()
{
    Q_D(MapRTree);
    d->locked = true;
}

void MapRTree::unlock()
{
    Q_D(MapRTree);
    d->locked = false;
    unblockMove();
    unblockDelete();
    unblockInsert();
}

// -------------------------------------------------------

} // minigis

// -------------------------------------------------------

