/****************************************************************************/
/*
DatabaseController.cpp
autor: Тарасенко А.И.
*/

/** \file DatabaseController.cpp
 *
 */

/****************************************************************************/
#include <QtSql>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaType>

#include "databasecontroller.h"

/****************************************************************************/
/* FUTURE
 * 1. синхронная обёртка, не блокирующая остальные события, для ожидания прихода данных
 * 2. тест полученного массива данных в Дебаг версии на частичное и строгое совпадение
 * 3. простой обработчик запроса на основе SQL и плоских/квадратных входных данных
 * 4. поддержка вложенных транзакций и пресекающихся по возможности
 */
/****************************************************************************/

#if QT_VERSION < 0x040700
template <class T>
class QArgument<T &>: public QGenericArgument
{
public:
    inline QArgument(const char *aName, T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};
#endif

/********************** Global **************************/

namespace dc {

//! Логическое имя БД
#define DATABASE_LOGIC_NAME_DEF "DC"
//! Драйвер БД
#define DATABASE_DRIVER_NAME_DEF "QSQLITE"
//! Использовать проверку на пустоту БД
//#define USE_CHECK_EMPTY_DATABASE

/***************** ExecutingRequests *********************/

class ExecutingRequestsPrivate
{
public:
    QEventLoop *eventLoop;
    DatabaseController *dc;
};

//---------------------------------------------------------------
ExecutingRequests::ExecutingRequests(DatabaseController *databaseController)
    : QObject(), d_ptr(new ExecutingRequestsPrivate)
{
    Q_D(ExecutingRequests);
    d->eventLoop = NULL;
    d->dc = databaseController;
}

//---------------------------------------------------------------
ExecutingRequests::~ExecutingRequests()
{
    if (d_ptr) {
        ExecutingRequestsPrivate *tmp = d_ptr;
        d_ptr = NULL;
        delete tmp;
    }
}

//---------------------------------------------------------------
void ExecutingRequests::onResult(uint, QVariant /*result*/, QVariant)
{
    Q_D(ExecutingRequests);
    if (!d->eventLoop)
        return;
    d->eventLoop->exit(0);
}

//---------------------------------------------------------------
QVariant ExecutingRequests::Exec(const QString &/*rawSql*/, QString */*errorString*/)
{
    return QVariant(); // kill me after uncomment follow code

//    Q_D(ExecutingRequests);
//    if (d->eventLoop) {
////        LogW3(CommissioningName) << "Recursive call detected";
//        return;
//    }
//    QEventLoop eventLoop;
//    d->eventLoop = &eventLoop;
//    QPointer<ExecutingRequests> guard = this;

//    d->dc->postRequest("raw-sql", rawSql, )

//    int ret = eventLoop.exec();
//    if (guard.isNull())
//        return;
//    d->eventLoop = NULL;
//    if (res)
//        *res = ret;
}

/********************** Request **************************/
//! Класс - элемент очереди
class Request
{
    public:
        //! Конструктор
        Request();
        //! Конструктор с параметрами
        Request(
                uint                numberQuery,       //!< регистрационный номер запроса
                uint                requestHash,       //!< свёртка имени обработчика
                const QVariant     &requestParameters, //!< параметры запроса
                QPointer<QObject>   senderPtr,         //!< объект получатель ответа
                const QByteArray   &callBackName = QByteArray() //!< имя метода обратного вызова, вызываемого после завершения запроса. Если пусто, то обратный вызов не требуется
                );

    public:
        uint              number;     //!< номер запроса
        uint              request;    //!< свёртка имени обработчика
        QVariant          parameters; //!< параметры запроса
        QPointer<QObject> sender;     //!< объект получатель ответа
        QByteArray        callBack;   //!< имя метода обратного вызова, вызываемого после завершения запроса. Если пусто, то обратный вызов не требуется
};

//---------------------------------------------------------------
Request::Request()
    : number(0), request(0)
{
}

//---------------------------------------------------------------
Request::Request(
        uint                numberQuery,
        uint                requestHash,
        const QVariant     &requestParameters,
        QPointer<QObject>   senderPtr,
        const QByteArray   &callBackName
        )
    : number(numberQuery), request(requestHash), parameters(requestParameters), sender(senderPtr), callBack(callBackName)
{
}

/********************** PriorityQueue **************************/

//! Класс - Очередь с приоритетом
/**
    Каждая последующая очередь по рангу приоритета выполняется реже в 2 раза от более высокоприоритетной
    Каждый элемент даже из самой низкоприоритетной очереди будет выполнен несмотря на наличие более высокоприоритетных
*/
template <typename T>
class PriorityQueue
{
public:
    typedef QQueue<T> QueueT;
    typedef QVector<QueueT> VectorQT;

    //---------------------------------------------------------------
    //! Конструктор
    //! \param priorityCount количество приоритетов (1..32). Если priorityCount == 0, то очередь вырождается в одноранговую
    PriorityQueue(int priorityCount = 0) {
        int size = qBound(0, priorityCount+1, 32);
        _counter = 1;
        _bit     = 1;
        _max     = 0x1 << size;
        _queues.resize(size);
    }

    //---------------------------------------------------------------
    //! Деструктор
    ~PriorityQueue() {
        _queues.clear();
    }

    //---------------------------------------------------------------
    //! Поместить элемент в очередь с заданным приоритетом
    //! \param item элемент помещаемый в очередь
    //! \param priority приоритет
    inline void enqueue(const T &item, int priority = 0) {
        _queues[qBound(0, priority, _queues.size() - 1)].enqueue(item);
    }

    //---------------------------------------------------------------
    //! Извлечь очередной элемент из очереди согласно рангу
    inline T dequeue() {
        // сначала обработать очередь вне приоритета
        QueueT &realTimeQueue = _queues.first();
        if (!realTimeQueue.isEmpty())
            return realTimeQueue.dequeue();

        // количество попыток чтения удвоенное
        for (int tryReads = _queues.size() << 1; tryReads; --tryReads) {
            if (++_bit >= _queues.size()) {
                _bit = 1;
                _counter += 2; // не учитываем первую очередь с номером ноль
                if (_counter >= _max)
                    _counter = 2;
            }
            if ((_counter & (0x1 << _bit)) != 0 && !_queues[_bit].isEmpty())
                return _queues[_bit].dequeue();
        }

        // найти ну хоть что-нибудь
        for (typename VectorQT::iterator it = _queues.begin(); it != _queues.end(); ++it)
            if (!it->isEmpty())
                return it->dequeue();

        // не нашли - варнинг с эксепшеном
        qWarning() << "Empty queue !!!";
        throw 0;
    }

    //---------------------------------------------------------------
    //! количество элементов в очереди
    inline int count() const {
        int count = 0;
        for (typename VectorQT::const_iterator it = _queues.begin(); it != _queues.end(); ++it)
            count += it->count();
        return count;
    }

    //---------------------------------------------------------------
    //! Проверка на то что все очереди пусты
    inline bool isEmpty() const {
        return count() == 0;
    }

private:
    int _counter; // счётчик от 1 до _max
    int _bit; // номер текущего бита
    int _max; // максимальное число = 2^(число очередей)
    VectorQT _queues; // очереди
};

/********************** DbTransactor **************************/

DbTransactor::DbTransactor(QSqlDatabase &db, bool rollbackOnExit)
{
    database     = db;
    needRollback = rollbackOnExit;
    beAction     = false;
    database.transaction();
}

//---------------------------------------------------------------

DbTransactor::DbTransactor(DatabaseController &dc, bool rollbackOnExit)
{
    if (!dc.database()) {
        qWarning() << "Not find database";
        beAction = true;
        return;
    }
    database     = *dc.database();
    needRollback = rollbackOnExit;
    beAction     = false;
    database.transaction();
}

//---------------------------------------------------------------

DbTransactor::~DbTransactor()
{
    if (!beAction) {
        if (needRollback)
            rollback();
        else
            commit();
    }
}

//---------------------------------------------------------------

bool DbTransactor::commit()
{
    bool r = false;
    if (!beAction)
        r = database.commit();
    beAction = true;
    return r;
}

//---------------------------------------------------------------

bool DbTransactor::rollback()
{
    bool r = false;
    if (!beAction)
        r = database.rollback();
    beAction = true;
    return r;
}

/********************** DatabaseControllerPrivate **************************/

typedef QPair<QPointer<QObject>, QByteArray> DCHandler;

//! Класс данных контроллера БД
class DatabaseControllerPrivate
{
    public:
    DatabaseControllerPrivate()
        : requestQueue(5) {}

        bool           beInited; //!< истина - если инициализация прошла успешно, иначе - ложь
        bool           needQuit; //!< признак необходимости выхода
        QMutex         mutex;    //!< мъютекс защиты данных
        QWaitCondition wait;     //!< ожидание новых данных
        QThread        thread;   //!< поток работы БД
        int            number;   //!< номер текущего запроса БД
        QSqlDatabase   db;       //!< дискриптор БД

        bool                     inTransaction; //!< признак открытой транзакции
        QHash<uint, DCHandler>   handlers;      //!< список обработчиков
        QHash<uint, QString >    handlerNames;  //!< список имён обработчиков (отладочная информация)
        PriorityQueue<Request *> requestQueue;  //!< очередь запросов
};

/********************** DatabaseController **************************/

//---------------------------------------------------------------
DatabaseController::DatabaseController() :
    QObject(), d_ptr(new DatabaseControllerPrivate)
{
    setObjectName("DatabaseController");
    Q_D(DatabaseController);

    d->number        = 0;
    d->beInited      = false;
    d->needQuit      = false;
    d->inTransaction = false;
    d->thread.setObjectName("DC Thread");

    // регистрация новых типов данных
    qRegisterMetaType<bool*>             ("bool*");
    qRegisterMetaType<QString*>          ("QString*");
    qRegisterMetaType<QPointer<QObject> >("QPointer<QObject>");
    qRegisterMetaType<QueryResult>       ("QueryResult");

    moveToThread(&d->thread);
    d->thread.start();
}

//---------------------------------------------------------------
DatabaseController::~DatabaseController()
{
    Q_D(DatabaseController);
    done();
    d->thread.quit();
    d->thread.wait();
    delete d_ptr;
}

//---------------------------------------------------------------
DatabaseController *DatabaseController::inst()
{
    static DatabaseController inst;
    return &inst;
}

//---------------------------------------------------------------
bool DatabaseController::init(
        QString const     &driver,
        QVariantMap const &connData,
        QString           *errorStringPtr
        )
{
    if (QThread::currentThread() != thread())
    {
        bool result = false;
        QMetaObject::invokeMethod(
                    this,
                    "init",
                    Qt::BlockingQueuedConnection,
                    Q_RETURN_ARG(bool, result),
                    Q_ARG(QString,     driver),
                    Q_ARG(QVariantMap, connData),
                    Q_ARG(QString*,    errorStringPtr)
                    );

        return result;
    }

    Q_D(DatabaseController);
    QMutexLocker locker(&d->mutex);
    if (d->beInited) {
        if (errorStringPtr)
            *errorStringPtr = "double initialization of the database";
        return false;
    }

    QString dbLogicName_p = nextLogicName();
    if (dbLogicName_p.isEmpty()) // хз. нафсякий
        dbLogicName_p = DATABASE_LOGIC_NAME_DEF;
    d->thread.setObjectName(d->thread.objectName() + "-" + dbLogicName_p);

    if (!QSqlDatabase::drivers().contains(driver)) {
        if (errorStringPtr)
            *errorStringPtr =
                QString("Unable to find driver for work with %1 [%2]")
                .arg(driver)
                .arg(QSqlDatabase::drivers().join(", "));
        return false;
    }

    if (QSqlDatabase::contains(dbLogicName_p))
        d->db = QSqlDatabase::database(dbLogicName_p);
    else {
        d->db = QSqlDatabase::addDatabase(driver, dbLogicName_p);
        d->db.setDatabaseName(connData.value("name").toString());
        d->db.setUserName    (connData.value("user").toString());
        d->db.setPassword    (connData.value("pass").toString());
        d->db.setHostName    (connData.value("host").toString());
        d->db.setPort        (connData.value("port").toInt());
    }
    if (!d->db.isOpen()) {
        if (!d->db.open()) {
            if (errorStringPtr)
                *errorStringPtr = "Unable to connect to database " + d->db.lastError().text();
            return false;
        }
    }

//    QString errorString;
//    if (!sqliteMemoryFile(d->db, d->fileDb, false, &errorString)) {
//        qWarning() << errorString << "[" << d->fileDb << "]";
    //    }

    if (driver == "QSQLITE") {
        QSqlQuery query(d->db);

        #define ExecPragma(sql, err) \
            if (!query.exec(sql)) { \
            if (errorStringPtr) \
            *errorStringPtr = err; \
            return false; \
            }

        //    ExecPragma("PRAGMA cipher=salg;",        "future");
        //    ExecPragma("PRAGMA cipher=ldbk;",        "future");
        //    ExecPragma("PRAGMA integrity_check;",      "Failed integrity check");
        ExecPragma("PRAGMA encoding = \"UTF-8\";", "Failed to set encoding utf-8");
        //    ExecPragma("PRAGMA temp_store=MEMORY;",    "Failed to enable temp store");
        ExecPragma("PRAGMA foreign_keys=ON;",      "Failed to enable foreign keys");
        //    ExecPragma("PRAGMA synchronous=OFF;",      "Failed to disable synchronous");

#if USE_CHECK_EMPTY_DATABASE
        ExecPragma("SELECT count(*) FROM sqlite_master;", "Failed fetching result");
        if (!query.first()) {
            if (errorStringPtr)
                *errorStringPtr = "Error getting row";
            return false;
        }
        if (query.value(0).toInt() == 0) {
            if (errorStringPtr)
                *errorStringPtr = "Error, empty database";
            return false;
        }
#endif
    }

    d->needQuit = false;
    d->beInited = true;

    if (!QMetaObject::invokeMethod(
                this,
                "mainProcessing",
                Qt::QueuedConnection
                )
            ) {
        if (errorStringPtr)
            *errorStringPtr = "Not invoked mainProcessing";
        return false;
    }

    return true;
}

//---------------------------------------------------------------
bool DatabaseController::done()
{
    Q_D(DatabaseController);

    {
        QMutexLocker locker(&d->mutex);
        d->needQuit = true;

        const QList<DCHandler> &handlers = d->handlers.values();
        foreach (DCHandler const &key, handlers)
            unregisterHandlerPriv(key.first);

        d->handlers.clear();
        d->handlerNames.clear();
        try {
            for (int i = d->requestQueue.count(); i; --i)
                delete d->requestQueue.dequeue();
        }
        catch (...) {}

        d->wait.wakeAll();
    }

    d->beInited = false;
    return true;
}

//---------------------------------------------------------------
//void BDProfiler(void* udp, const char* sql, sqlite3_uint64 time)
//{
//    uint uptime = *(reinterpret_cast<uint*>(udp));
//    if (time/1000000 >= uptime)
//        qDebug() << time/1000000 << "ms Query:" << sql;  //  time/1000000 - время выполнения sql-запроса в мс
//}

//---------------------------------------------------------------
void DatabaseController::setProfiler(uint initime)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, "setProfiler",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(uint, initime));
        return;
    }

//    Q_D(DatabaseController);
//    static uint uptime = initime;

//    sqlite3_profile(*static_cast<sqlite3 **>(d->db.driver()->handle().data()),
//                    &BDProfiler, &uptime); // установка обработчиками завершения sql-запроса
}

//---------------------------------------------------------------
void DatabaseController::destroyedHandler(QObject *object)
{
    unregisterHandler(object);
}

//---------------------------------------------------------------
bool DatabaseController::registerHandler (
        const QString    &requestName,
        QPointer<QObject> implementerPtr,
        const QByteArray &handlerName
        )
{
    Q_D(DatabaseController);
    if (requestName.isEmpty()) {
        qWarning() << "ERROR: requestName is empty";
        return false;
    }
    if (implementerPtr.isNull()) {
        qWarning() << "ERROR: implementerPtr is NULL";
        return false;
    }
    if (handlerName.isEmpty()) {
        qWarning() << "ERROR: handlerName is empty";
        return false;
    }
    static const char *arguments = "(QVariant,QVariant&,QVariant&)";
    if (implementerPtr->metaObject()->indexOfSlot(handlerName+arguments) == -1) {
        qWarning() << "ERROR: unknown handlerName method or not invokable";
        return false;
    }

    QMutexLocker locker(&d->mutex);

#ifdef DEBUG
    {
        // параноидальная проверка на дублирование свёрток от разных запросов
        static QHash<uint, QString> _testHash;
        uint h = qHash(requestName);
        if (_testHash.contains(h) && _testHash.value(h) != requestName)
            qWarning() << QString::fromUtf8("ERROR: Hashes row %1 and %2 are the same and equal to %3")
                        .arg(requestName).arg(_testHash.value(h)).arg(h);
    }
#endif // DEBUG

    uint topic = qHash(requestName);
    if (d->handlers.contains(topic)) {
        qWarning() << QString("Handler named \"%1\" is already registered").arg(requestName);
        return false;
    }
    d->handlers[topic] = DCHandler(implementerPtr, handlerName);
    d->handlerNames[topic] = requestName;
    if (implementerPtr->thread() != thread())
        implementerPtr->moveToThread(thread()); // перенести обработчик в поток контроллера БД
    connect(implementerPtr.data(), SIGNAL(destroyed(QObject*)), this, SLOT(destroyedHandler(QObject*)), Qt::UniqueConnection);
    return true;
}

//---------------------------------------------------------------
void DatabaseController::unregisterHandler(
        QObject *implementerPtr
        )
{
    Q_D(DatabaseController);
    QMutexLocker locker(&d->mutex);
    unregisterHandlerPriv(implementerPtr);
}

//---------------------------------------------------------------
void DatabaseController::unregisterHandlerPriv(
        QObject *implementerPtr
        )
{
    Q_D(DatabaseController);
    for (QMutableHashIterator<uint, DCHandler> it(d->handlers); it.hasNext(); ) {
        it.next();
        if (it.value().first == implementerPtr) {
            d->handlerNames.remove(it.key());
            it.remove();
        }
    }
}

//---------------------------------------------------------------
QString DatabaseController::nextLogicName()
{
    static int npp = 0;
    return QString("%1-%2")
            .arg(DATABASE_LOGIC_NAME_DEF)
            .arg(++npp);
}

//---------------------------------------------------------------
void DatabaseController::unregisterHandler(
        const QString &requestName
        )
{
    Q_D(DatabaseController);
    if (requestName.isEmpty()) {
        qWarning() << "ERROR: requestName is empty";
        return;
    }
    QMutexLocker locker(&d->mutex);
    uint topic = qHash(requestName);
    if (!d->handlers.contains(topic)) {
        qWarning() << QString("Handler named \"%1\" is not registered").arg(requestName);
        return;
    }
    d->handlers.remove(topic);
    d->handlerNames.remove(topic);
}

//---------------------------------------------------------------
QStringList DatabaseController::registeredHandlers()
{
    Q_D(DatabaseController);
    QMutexLocker locker(&d->mutex);
    QStringList res;
    for (QHashIterator<uint, QString> it(d->handlerNames); it.hasNext(); ) {
        it.next();
        QObject *obj = d->handlers.value(it.key()).first;
        res.append(QString("[%1][%2]").arg(it.value()).arg(obj ? obj->objectName() : QString()));
    }
    return res;
}

//---------------------------------------------------------------
QSqlDatabase const *DatabaseController::database() const
{
    Q_D(const DatabaseController);
    if (QThread::currentThread() != thread()) {
        qWarning() << "Stream flow is different from the database";
        return NULL;
    }
    return &d->db;
}

//---------------------------------------------------------------
bool DatabaseController::execQuery(
        const QString &query,
        const QVariantMap &parameters,
        QueryResult &result
        )
{
    Q_D(DatabaseController);
    result.clear();
    if (QThread::currentThread() != thread()) {
        qWarning() << "Stream flow is different from the database";
        return false;
    }

//    QMutexLocker lock(&d->mutex);
    QSqlQuery q(d->db);
    q.prepare(query);
    if (!parameters.isEmpty())
        for (QMapIterator<QString, QVariant > it(parameters); it.hasNext(); ) {
            it.next();
            q.bindValue(it.key(), it.value());
        }
    if (!q.exec()) {
        qWarning() << "ERROR execute query " << query << " with parameters " << parameters << q.lastError();
        return false;
    }

    result = convertQueryResult(q);
    return true;
}

//---------------------------------------------------------------
QueryResult DatabaseController::convertQueryResult(
        QSqlQuery query
        )
{
    QueryResult result;
    if (!query.isSelect())
        return result;

    QSqlRecord rec = query.record();
    int fields = rec.count();
    QVariantMap row;
    QStringList names;
    for (int f = 0; f < fields; ++f)
        names.append(rec.fieldName(f));
    result.clear();
    while (query.next()) {
        row.clear();
        for (int f = 0; f < fields; ++f)
            row[names[f]] = query.value(f);
        result.append(row);
    }
    return result;
}

//---------------------------------------------------------------
bool DatabaseController::transaction()
{
    Q_D(DatabaseController);
    if (QThread::currentThread() != thread()) {
        qWarning() << "Stream flow is different from the database";
        return false;
    }
//    QMutexLocker(&d->mutex);

    if (d->inTransaction)
        rollback();
    if (!d->db.transaction()) {
        qWarning() << "Error start transaction " << d->db.lastError();
        return false;
    }
    d->inTransaction = true;
    return true;
}

//---------------------------------------------------------------
void DatabaseController::commit()
{
    Q_D(DatabaseController);
    if (QThread::currentThread() != thread()) {
        qWarning() << "Stream flow is different from the database";
        return;
    }
//    QMutexLocker(&d->mutex);
    if (!d->db.commit())
        qWarning() << "Error commit transaction " << d->db.lastError();
    d->inTransaction = false;
}

//---------------------------------------------------------------
void DatabaseController::rollback()
{
    Q_D(DatabaseController);
    if (QThread::currentThread() != thread()) {
        qWarning() << "Stream flow is different from the database";
        return;
    }
//    QMutexLocker(&d->mutex);
    if (!d->db.rollback())
        qWarning() << "Error rollback transaction " << d->db.lastError();
    d->inTransaction = false;
}

//---------------------------------------------------------------
uint DatabaseController::postRequest(
        const QString     &requestName,
        const QVariant    &requestParameters,
        QPointer<QObject>  senderPtr,
        const QByteArray  &callBackName
        )
{
    return postRequest(
                requestName,
                requestParameters,
                NormalPriority,
                senderPtr,
                callBackName
                );
}

//---------------------------------------------------------------
uint DatabaseController::postRequest(
        const QString     &requestName,
        const QVariant    &requestParameters,
        int                priority,
        QPointer<QObject>  senderPtr,
        const QByteArray  &callBackName
        )
{
    Q_D(DatabaseController);
    if (requestName.isEmpty()) {
        qWarning() << "Error. requestName of query is empty";
        return 0;
    }
    static const char *arguments = "(uint,QVariant,QVariant)";
    if (senderPtr && !callBackName.isEmpty())
        if (senderPtr->metaObject()->indexOfSlot(callBackName+arguments) == -1) {
            qWarning() << "ERROR: unknown callBackName method or not invokable";
            return 0;
        }
    uint npp;
    {
        QMutexLocker locker(&d->mutex);
        npp = ++d->number;
        uint h = qHash(requestName);
        if (!d->handlers.contains(h))
            qWarning() << QString::fromUtf8("Handler \"%1\" not found. hope that soon will be.").arg(requestName);
        d->requestQueue.enqueue(new Request(npp, h, requestParameters, senderPtr, callBackName), priority);
    }
    d->wait.wakeAll();
    return npp;
}

//---------------------------------------------------------------
uint DatabaseController::requestCount() const
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->requestQueue.count();
}

//---------------------------------------------------------------
void DatabaseController::mainProcessing()
{
    Q_D(DatabaseController);
    Request *request = NULL;
    QPair<QPointer<QObject>, QByteArray> handler;
    QVariant result;
    QVariant errors;

    forever {
        if (request) {
            delete request;
            request = NULL;
        }

        { // считать запрос из очереди
            QMutexLocker locker(&d->mutex);

            while (d->requestQueue.isEmpty() && !d->needQuit)
                d->wait.wait(&d->mutex);
            if (d->needQuit) {
                if (d->requestQueue.count() > 0)
                    qWarning() << QString("Stopping the flow has become more raw %1 request").arg(d->requestQueue.count());
                break;
            }
            if (d->requestQueue.isEmpty()) {
                qWarning() << "Unable to process next query because queue is empty";
                continue;
            }
            request = d->requestQueue.dequeue();
            handler = d->handlers.value(request->request);
        } // end - считать запрос

        if (handler.first.isNull()) {
            qWarning() << "The handler has disconnected before his request was processed.";
            continue;
        }
        if (!QMetaObject::invokeMethod(
                    handler.first,
                    handler.second,
                    Qt::DirectConnection,
                    Q_ARG(QVariant, request->parameters),
                    Q_ARG(QVariant&, result),
                    Q_ARG(QVariant&, errors)
                    ))
            qWarning() << QString::fromUtf8("ERROR: no invoke slot %1 for %2").arg(handler.second.constData()).arg(handler.first->objectName());

        if (request->sender.isNull() || request->callBack.isEmpty())
            continue;
        if (!QMetaObject::invokeMethod(
                    request->sender.data(),
                    request->callBack,
                    Qt::QueuedConnection,
                    Q_ARG(uint, request->number),
                    Q_ARG(QVariant, result),
                    Q_ARG(QVariant, errors)
                    ))
            qWarning() << QString::fromUtf8("ERROR: no invoke slot %1 for %2").arg(request->callBack.constData()).arg(request->sender.data()->objectName());
        result.clear();
    } // forever
}

//---------------------------------------------------------------

} // namespace dc
