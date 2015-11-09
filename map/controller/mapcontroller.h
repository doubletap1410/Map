#ifndef MAPCONTROLLER_H
#define MAPCONTROLLER_H

#include <QObject>
#include <QQmlListProperty>
#include <QSharedPointer>

#include "map/core/mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

class MapFrame;

// -------------------------------------------------------

/**
 * @brief The Callable class - интерфейс вызова функции, используется для вызова виртульных функций в QML
 */
class Callable : public QObject
{
    Q_OBJECT

public:
    virtual ~Callable() { }
    Q_INVOKABLE virtual void call(bool) { }
};
typedef QSharedPointer<Callable> SharedCall;

/**
 * @brief The CallableMethod class - создает объект вызывающий метод класса через интерфейс Callable
 */
template <typename T>
class CallableMethod : public Callable
{
public:
    typedef void(T::*func)(bool);

    CallableMethod() = default;
    CallableMethod(T *obj, func f) : o_(obj), f_(f) { }
    virtual void call(bool flag) { (o_->*f_)(flag); }

private:
    T *o_;
    func f_;
};

template<typename T>
SharedCall makeCallable(T *obj, void(T::*func)(bool)) {
    return SharedCall(new CallableMethod<T>(obj, func));
}

// -------------------------------------------------------

/**
 * @brief The MapQMLButton class - кнопка на QML-карте
 * @bImage - картинка на кнопке
 * @bCheckable - кнопка чекабельная
 * @bAutoRepeat - кнопка с автоповтором
 * @bFunc - функтор функции кнопки
 */
class MapQMLButton : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bImage READ image CONSTANT)
    Q_PROPERTY(bool bCheckable READ checkable CONSTANT)
    Q_PROPERTY(bool bAutoRepeat READ autoRepeat CONSTANT)
    Q_PROPERTY(minigis::Callable * bFunc READ func CONSTANT)

public:
    MapQMLButton() = default;
    MapQMLButton(SharedCall func,
                 const QString &image,
                 bool checkable,
                 bool autoRepeat,
                 QObject *parent = NULL)
        : QObject(parent), func_(func), image_(image),
          checkable_(checkable), autoRepeat_(autoRepeat) { }

    QString image() { return image_; }
    bool checkable() { return checkable_; }
    bool autoRepeat() { return autoRepeat_; }

    minigis::Callable * func() { return func_.data(); }

private:
    SharedCall func_;
    QString image_;
    bool checkable_;
    bool autoRepeat_;
};

// -------------------------------------------------------

/**
 * @brief The MapQMLPlugin class - представляет собой картину, название и qml-форму модуля карты
 */
class MapQMLPlugin : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bPlugin READ plugin CONSTANT)
    Q_PROPERTY(QString bText   READ text   CONSTANT)
    Q_PROPERTY(QString bImage  READ image  CONSTANT)

public:
    MapQMLPlugin() = default;
    MapQMLPlugin(const QString &plugin,
                 const QString &text,
                 const QString &image = QString(),
                 QObject *parent = NULL)
        : QObject(parent), plugin_(plugin),
          text_(text), image_(image) { }

    Q_INVOKABLE QString image()  { return image_;  }
    Q_INVOKABLE QString text()   { return text_;   }
    Q_INVOKABLE QString plugin() { return plugin_; }

private:
    QString plugin_;
    QString text_;
    QString image_;
};

// -------------------------------------------------------

/**
 * @brief The MapController class - управляет взаимодействием С++ и QML, используям QML - контейнеры и функции регистрации
 * @controls - список "управлющих" кнопок, регистрируется кнопка ф-ций registerController(...)
 */
struct MapControllerPrivate;
class MapController: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<minigis::MapQMLButton> controls READ controls CONSTANT)
    Q_PROPERTY(QQmlListProperty<minigis::MapQMLPlugin> settings READ settings CONSTANT)
    Q_PROPERTY(QQmlListProperty<minigis::MapQMLPlugin> panels   READ panels   CONSTANT)

public:
    explicit MapController(QObject *parent = 0);
    virtual ~MapController();

    void init(MapFrame *map); // инициализация модуля
    void autoRegistration();  // регистрация кнопок по-умолчанию

    Q_INVOKABLE QQmlListProperty<minigis::MapQMLButton> controls(); // список "управляющий" кнопок
    /**
     * @brief registerController зарегистрировать "управляющую" кнопку
     * @param target класс с методом func
     * @param func метод отвечающий за функцию кнопки
     * @param image картинка на конпке
     * @param checkable чекабельная кнопка
     * @param autoRepeat кнопка с автоповтором
     * @param st вкл/выкл кнопку при изменении режима хелпера
     */
    template <typename T>
    void registerController(T *target, void(T::*func)(bool), const QString &image,
                            bool checkable, bool autoRepeat, States st = stNoop);

    QQmlListProperty<minigis::MapQMLPlugin> settings(); // список панелей настроек
    Q_INVOKABLE void registerSetting(const QString &qmlform, const QString &text, const QString &image = QString());
    QQmlListProperty<minigis::MapQMLPlugin> panels();   // список панелей управления модулями
    Q_INVOKABLE void registerPanel(const QString &qmlform, const QString &text, const QString &image = QString());

signals:
    void breakHepler();                             // сигнал окончания работы с картой
    bool buttonStateChanged(int idx, bool flag);    // сигнал изменения состояния кнопки

private slots:
    void moveMap(bool);     // вкл/выкл перемещение карты
    void rotateMap(bool);   // вкл/выкл вращение карты
    void zoomIn(bool);      // приблизить карту
    void zoomOut(bool);     // отдалить карту

    void stateChanged(States st); // реагируем на изменение состояния хелпера
    bool grabMouse();             // попытаться получить управления хелпера

private:
    Q_DECLARE_PRIVATE(MapController)
    Q_DISABLE_COPY(MapController)

    MapControllerPrivate *d_ptr;
};

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPCONTROLLER_H
