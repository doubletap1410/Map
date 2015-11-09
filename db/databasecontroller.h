/****************************************************************************/
/*
databasecontroller.h
autor: Тарасенко А.И.
*/
/** \file databasecontroller.h
  * Класс DatabaseController ассинхронный контроллер для работы с базой данных.
  * Все функции для работы с базой данных осуществляется асинхронно
  * в отдельном потоке. Модуль, который хочет получить доступ к базе данных,
  * помещает запрос через вызов postRequest() с указанием обратного вызова,
  * который будет выполнен по завершении запроса. Сам обработчик запроса может быть разработан
  * третьей стороной, и должен быть наследован от QObject. Для вызова обработчиков, зарегистрированных в
  * контроллере с помощью метода registerHandler() используется механизм invokeMethod
  *
  * Обработчи должны соответствовать следующим требованиям:
  * 1. класс необходимо наследовать от QObject
  * 2. у обработчика не может быть родителя (parent() === NULL)
  * 3. если обработчик "общается" с другими объектами, и уж тем более с графическим интерфейсом,
  *    он должен делать это через slot/signal
  * 4. после регистрации в качестве обработчика, объект автоматически перемещается в поток базы данных
  */

/****************************************************************************/

#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSharedPointer>
#include <QStringList>

namespace dc {

//! Приоритет выполнения запросов
enum {
    RealTimePriority    = 0, //!< очередь реального времени. До тех пор пока она не будет исчерпана остальные не выполняются

    HighestPriority     = 1, //!< очередь высшего приоритета
    AboveNormalPriority = 2, //!< очередь высокого приоритета
    NormalPriority      = 3, //!< очередь нормального приоритета
    BelowNormalPriority = 4, //!< очередь низкого приоритета
    LowestPriority      = 5, //!< очередь нижайшего приоритета
};

//! результат запроса
typedef QList<QVariantMap> QueryResult;

//! класс приватных данных
class ExecutingRequestsPrivate;
class DatabaseController;
//! Выполнение блокирующих запросов
class ExecutingRequests : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(dc::ExecutingRequests)

public:
    //! Конструктор
    ExecutingRequests(DatabaseController *databaseController);

    //! Деструктор
    virtual ~ExecutingRequests();

private slots:
    //! получение данных от БД
    void onResult(
            uint     query,  //!< номер запроса
            QVariant result, //!< результат
            QVariant error   //!< ошибки
            );

    //! выполнить простой запрос
    QVariant Exec(
            const QString &rawSql, //!< скрипт запроса
            QString       *errorString = 0 //!< код ошибки
            );

private:
    ExecutingRequestsPrivate *d_ptr;
};


/**
 * @brief DbTransactor класс автоматического завершения блока транзакции
 */
class DbTransactor
{
public:
    DbTransactor(QSqlDatabase &db, bool rollbackOnExit = true);
    DbTransactor(DatabaseController &dc, bool rollbackOnExit = true);
    ~DbTransactor();

    bool commit();
    bool rollback();

private:
    QSqlDatabase database; // текущая БД
    bool needRollback; // откатывать транзакцию по умолчанию при завершении
    bool beAction; // было ручное подтверждение/откат транзакции
    Q_DISABLE_COPY(DbTransactor)
};


//! класс приватных данных
class DatabaseControllerPrivate;

//! Класс контроллер БД
class DatabaseController : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(dc::DatabaseController)

public:
    //! Конструктор
    DatabaseController();
    //! Деструктор
    virtual ~DatabaseController();

    //! Инстанс
    static DatabaseController *inst();

public slots:
    //! Инициализация контроллера
    bool init(
            QString const     &driver         = QString("QSQLITE"), //!< драйвер СУБД
            QVariantMap const &connData       = QVariantMap(),      //!< данные соединения с БД
            QString           *errorStringPtr = 0                   //!< код ошибки
            );

    //! Завершение работы с контроллером
    bool done();

    //! Установка профилировщика времени выполнения SQL-запросов.
    //! \param initime логировать запросы медленнее этого времени в мс
    void setProfiler(uint initime);

    //! Регистрация обработчика
    /**
        Метод handlerName в обработчике implementerPtr будет вызываться из потока invokeMethod() с Qt::DirectConnection.
        Возвращает false, если в обработчике такой метод не существует.
    */
    bool registerHandler (
            const QString    &requestName,    //!< логическое имя обработчика (функции)
            QPointer<QObject> implementerPtr, //!< объект где реализован обработчик
            const QByteArray &handlerName     //!< функция обработчик
            );

    //! Разрегистрация обработчиков объекта
    void unregisterHandler(
            QObject *implementerPtr //!< объект где реализованы обработчики
            );

    //! Разрегистрация обработчика
    void unregisterHandler(
            const QString &requestName //!< логическое имя обработчика
            );

    //! Список зарегестрированных обработчиков. отладочная информация
    QStringList registeredHandlers();

    //! БД контроллера БД
    /**
        Доступ к БД контроллера напрямую. Доступ возможен только из потока БД.
    */
    QSqlDatabase const *database() const;

    //! Выполнить SQL запрос
    /**
        Этот метод будет вызываться напрямую в потоке контроллера для доступа к базе данных.
        Каждая запись в списке результатов является QVariantMap, являющаяся списком ключ-значение
    */
    bool execQuery(
            const QString &query,          //!< запрос
            const QVariantMap &parameters, //!< параметры запроса
            QueryResult &result            //!< результат выполнения запроса - таблица данных
            );

    //! Конвертация результата запроса
    /**
        Этот метод будет вызываться напрямую в потоке контроллера для доступа к базе данных.
        Каждая запись в списке результатов является QVariantMap, являющаяся списком ключ-значение
    */
    QueryResult convertQueryResult(
            QSqlQuery query //!< выполненный запрос
            );

    //! Старт транзакции
    /**
        Этот метод должен вызываться напрямую в потоке контроллера для доступа к базе данных.
        Перед началом транзакции, выполняется проверка, если транзакция уже запущена, то
        принудительно вызвается откат.
    */
    bool transaction();

    //! Подтвердить транзакцию
    /**
        Этот метод должен вызываться напрямую в потоке контроллера для доступа к базе данных.
    */
    void commit();

    //! Откатить транзакцию
    /**
        Этот метод должен вызываться напрямую в потоке контроллера для доступа к базе данных.
    */
    void rollback();

    //! Поставить запрос в очередь обработки
    /**
        ассинхронно вызвать обработчик с именем requestName и входными параметрами requestParameters (приоритет - средний)
        результат будет получен объктом senderPtr через метод callBackName
        \returns регистрационный номер запроса
    */
    uint postRequest(
            const QString     &requestName,             //!< имя обработчика
            const QVariant    &requestParameters,       //!< параметры запроса
            QPointer<QObject>  senderPtr,               //!< объект получатель результата
            const QByteArray  &callBackName = QByteArray() //!< имя метода обратного вызова, вызываемого после завершения запроса. Если пусто, то обратный вызов не требуется
            );

    //! Поставить запрос в очередь обработки
    /**
        ассинхронно вызвать обработчик с именем requestName, входными параметрами requestParameters и приоритетом выполнения priority
        результат будет получен объктом senderPtr через метод callBackName
        \returns регистрационный номер запроса
    */
    uint postRequest(
            const QString     &requestName,                     //!< имя обработчика
            const QVariant    &requestParameters = QVariant(),  //!< параметры запроса
            int                priority = LowestPriority,       //!< приоритет запроса
            QPointer<QObject>  senderPtr = QPointer<QObject>(), //!< объект получатель результата
            const QByteArray  &callBackName = QByteArray()      //!< имя метода обратного вызова, вызываемого после завершения запроса. Если пусто, то обратный вызов не требуется
            );

    //! Размер очереди запросов
    /**
        размер оставшейся очереди запросов к БД
        \returns размер очереди
    */
    uint requestCount() const;

private slots:
    //! Уничтожение обработчика
    void destroyedHandler(
            QObject *object //!< объект обработчика
            );

    //! Основной процесс обработки очереди запросов
    void mainProcessing();

private:
    void unregisterHandlerPriv(QObject *implementerPtr);
    static QString nextLogicName();
private:
    //! Приватные данные
    DatabaseControllerPrivate *d_ptr;

    //! Отключить копирующие конструкторы
    Q_DISABLE_COPY(DatabaseController)
};

} // namespace dc

#endif // DATABASECONTROLLER_H
