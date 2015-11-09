#ifndef COORD_HPP
#define COORD_HPP

#include <QDebug>
#include <QStringList>
#include <QPointF>
#include <QPolygonF>
#include <vector>
#include <math.h>
#include <QDataStream>

#ifdef __cplusplus

// TODO: сделать разные компараторы для целых и вещественных типов. для nan(), qfuzzyisnull()

//////////////////////////////// Predefine ////////////////////////////////

#define NoopTypeValue       (0     )
#define PointTypeValue      (1 << 0)
#define Point3TypeValue     (1 << 1)
#define SizeTypeValue       (1 << 2)
#define RectTypeValue       (1 << 3)
#define LineTypeValue       (1 << 4)
#define MetricTypeValue     (1 << 5)

//////////////////////////////// Templ ////////////////////////////////

// подобрать тип данных в зависимости от условия
template <bool, typename Ta, typename Tb>
struct __IfThenElse;
template <typename Ta, typename Tb>
struct __IfThenElse<true, Ta, Tb> { typedef Ta type; };
template <typename Ta, typename Tb>
struct __IfThenElse<false, Ta, Tb> { typedef Tb type; };
struct __IfThenElse_dummy { };

// тип доступен при соблюдении условия
template<bool, typename = void> struct __Enable_If {};
template<typename T> struct __Enable_If<true, T> { typedef T type; };

// тип заблокирован при соблюдении условия
template<bool, typename T = void> struct __Disable_If {};
template<typename T> struct __Disable_If<false, T> { typedef T type; };

// булевы операции
template <bool A, bool B> struct __Bool_And { enum { value = A && B }; };
template <bool A, bool B> struct __Bool_Or  { enum { value = A || B }; };
template <bool A, bool B> struct __Bool_Xor { enum { value = A ^ B }; };
template <bool A>         struct __Bool_Not { enum { value = !A }; };


// TODO: не работает с перегруженными методами
template <typename T>
struct __HasEl {
private:
    typedef char One[1];
    typedef char Two[2];
    struct __Dummy { enum { value = 0 }; };

//#define TestP(Name, Param, Const, ExParam)
#define Test(Name, Param, Const) \
private: \
    template <typename S> struct __Exist_##Name { \
        template <typename U> static One& test_##Name(char (*)[sizeof(&U::Name)]); \
        template <typename  > static Two& test_##Name(...); \
        enum { value = sizeof(test_##Name<S>(0)) == sizeof(One) }; \
    }; \
    template <typename S> struct __IsMethod_##Name { \
        template <typename U, typename R, R (U::*)() Const = &U::Name> struct struct_0##Name { Two _; }; \
        template <typename U, typename R, typename P, R (U::*)(Param) Const = &U::Name> struct struct_1##Name { Two _; }; \
        static One &test_##Name(...); \
        template <typename U, typename R> static struct_0##Name<U, R> &test_##Name(R (U::*)() Const); \
        template <typename U, typename R, typename P> static struct_1##Name<U, R, P> &test_##Name(R (U::*)(P) Const); \
        enum { value = sizeof(test_##Name(&S::Name)) != sizeof(One) }; \
    }; \
public: \
    enum { \
        exist_##Name  = __Exist_##Name<T>::value, \
        method_##Name = __IfThenElse<exist_##Name, __IsMethod_##Name<T>, __Dummy>::type::value, \
        field_##Name  = __Bool_And<exist_##Name, !method_##Name>::value \
    };

    Test(x, , const)
    Test(y, , const)
    Test(z, , const)

    Test(setX, P, )
    Test(setY, P, )
    Test(setZ, P, )

    Test(width, , const)
    Test(height, , const)

    Test(begin, , const)
    Test(end, , const)
    Test(constBegin, , const)
    Test(constEnd, , const)

    Test(swap, , )
    Test(clear, , )
    Test(capacity, , )
    Test(resize, P, )
    Test(reserve, P, )
    Test(push_back, P, )
    Test(push_front, P, )

    Test(toString, , const)
    Test(fromString, P, )

#undef Test

    enum { is_point_    = __Bool_And<exist_x, exist_y>::value }; // тут возможна точка
    enum { is_point     = __Bool_And<method_x, method_y>::value }; // точка из методов
    enum { is_point_s   = __Bool_And<is_point, !method_z>::value }; // строго плоская точка из методов
    enum { is_point_f   = __Bool_And<field_x, field_y>::value }; // точка из полей
    enum { is_point_f_s = __Bool_And<is_point_f, !field_z>::value }; // строго плоская точка из полей
    enum { is_point3_   = __Bool_And<is_point_, exist_z>::value }; // тут возможна объёмная точка
    enum { is_point3    = __Bool_And<is_point, method_z>::value }; // объёмная точка из методов
    enum { is_point3_f  = __Bool_And<is_point_f, field_z>::value }; // объёмная точка из полей
    enum { is_size_     = __Bool_And<exist_width, exist_height>::value }; // тут возможен размер
    enum { is_size      = __Bool_And<method_width, method_height>::value }; // размер из методов
    enum { is_size_f    = __Bool_And<field_width, field_height>::value }; // размер из полей
    enum { is_rect_     = __Bool_And<is_point_, is_size_>::value }; // тут возможен регион
    enum { is_rect      = __Bool_And<is_point, is_size>::value }; // регион из методов
    enum { is_rect_f    = __Bool_And<is_point_f, is_size_f>::value }; // регион из полей
    enum { is_container = __Bool_And<method_clear, method_reserve>::value /* begin end size push_back*/}; // это контейнер
};

//////////////////////////////// Namespace ////////////////////////////////

namespace coord {

//////////////////////////////// Literas ////////////////////////////////

// TODO: для каждого интегрального типа определиться с "пустым" значением

// предопределение описанных ниже структур
template<typename T> class Point_;
template<typename T> class Point3_;
template<typename T> class Size_;
template<typename T> class Rect_;
template<typename T> class Line_;
template<typename T> class Metric_;


template <typename T> struct MetricClassType                 { enum { type = NoopTypeValue   }; };
template <typename T> struct MetricClassType < Point_  <T> > { enum { type = PointTypeValue  }; };
template <typename T> struct MetricClassType < Point3_ <T> > { enum { type = Point3TypeValue }; };
template <typename T> struct MetricClassType < Size_   <T> > { enum { type = SizeTypeValue   }; };
template <typename T> struct MetricClassType < Rect_   <T> > { enum { type = RectTypeValue   }; };
template <typename T> struct MetricClassType < Line_   <T> > { enum { type = LineTypeValue   }; };
template <typename T> struct MetricClassType < Metric_ <T> > { enum { type = MetricTypeValue }; };

// тест на целое число
template <typename T> struct __is_numeric {
    typedef T type;
    enum { value = 0 };
};
#define IS_NUM(T) template <> struct __is_numeric<T> { typedef T type; enum { value = 1 }; };
IS_NUM(signed char)
IS_NUM(unsigned char)
IS_NUM(signed short)
IS_NUM(unsigned short)
IS_NUM(signed int)
IS_NUM(unsigned int)
//IS_NUM(signed long)
//IS_NUM(unsigned long)
IS_NUM(signed long long)
IS_NUM(unsigned long long)
#undef IS_NUM

// тест на вещественное число
template <typename T> struct __is_floating {
    typedef T type;
    enum { value = 0 };
};
#define IS_FLOAT(T) template <> struct __is_floating<T> { typedef T type; enum { value = 1 }; };
IS_FLOAT(float)
IS_FLOAT(double)
#ifndef __NO_LONG_DOUBLE_MATH
//IS_FLOAT(long double)
#endif
#undef IS_FLOAT

// тест на любое число
template <typename T> struct __is_number {
    typedef T type;
    enum { value = __Bool_Or<__is_numeric<T>::value, __is_floating<T>::value>::value };
};

template<typename T> struct FunctorDataType {};

#define NUM_Functor(T) \
template<> struct FunctorDataType<T> { \
    static inline T getZero() { return 0; } \
    static inline T getNan() { return 0; } \
    static inline bool isZerro(T const &a) { return a == 0; } \
    static inline bool isNan(T const &a) { return a == 0; } \
    static inline bool isNull(T const &a) { return a == 0; } \
    static inline bool less(T const &a, T const &b) { return a < b; } \
    static inline bool eq(T const &a, T const &b) { return a == b; } \
    static inline int fromString(QString const &s, bool *ok = NULL) { return s.toInt(ok); } \
};

NUM_Functor(signed char)
NUM_Functor(unsigned char)
NUM_Functor(signed short)
NUM_Functor(unsigned short)
NUM_Functor(signed int)
NUM_Functor(unsigned int)
//NUM_Functor(signed long)
//NUM_Functor(unsigned long)
NUM_Functor(signed long long)
NUM_Functor(unsigned long long)

#undef NUM_Functor

template<> struct FunctorDataType<float> {
    static inline float getZero() { return 0.0; }
    static inline float getNan() { return ::nan("0"); }
    static inline bool isZerro(float const &a) { return qFuzzyIsNull(a); }
    static inline bool isNan(float const &a) { return isnan(a); }
    static inline bool isNull(float const &a) { return isZerro(a) || isNan(a); }
    static inline bool less(float const &a, float const &b) { return a < b; }
    static inline bool eq(float const &a, float const &b) { return qFuzzyCompare(a, b); }
    static inline float fromString(QString const &s, bool *ok = NULL) { return s.toFloat(ok); }
};
template<> struct FunctorDataType<double> {
    static inline double getZero() { return 0.0; }
    static inline double getNan() { return ::nan("0"); }
    static inline bool isZerro(double const &a) { return qFuzzyIsNull(a); }
    static inline bool isNan(double const &a) { return isnan(a); }
    static inline bool isNull(double const &a) { return isZerro(a) || isNan(a); }
    static inline bool less(double const &a, double const &b) { return a < b; }
    static inline bool eq(double const &a, double const &b) { return qFuzzyCompare(a, b); }
    static inline double fromString(QString const &s, bool *ok = NULL) { return s.toFloat(ok); }
};
#ifndef __NO_LONG_DOUBLE_MATH
#if 0
template<> struct FunctorDataType<long double> {
    static inline double getZero() { return 0.0; }
    static inline double getNan() { return ::nan("0"); }
    static inline bool isZerro(double const &a) { return qFuzzyIsNull(a); }
    static inline bool isNan(double const &a) { return isnan(a); }
    static inline bool isNull(double const &a) { return isZerro(a) || isNan(a); }
    static inline bool less(double const &a, double const &b) { return a < b; }
    static inline bool eq(double const &a, double const &b) { return qFuzzyCompare(a, b); }
    static inline double fromString(QString const &s, bool *ok = NULL) { return s.toFloat(ok); }
};
#endif
#endif

template<typename> struct LiteraDataType { enum {litera = '-'}; typedef void   type; };

#define LDT(T, C) template<> struct LiteraDataType<T> { enum {litera = C}; typedef T type; };

LDT(signed char,        'c')
LDT(unsigned char,      'C')
LDT(signed short,       's')
LDT(unsigned short,     'S')
LDT(signed int,         'i')
LDT(unsigned int,       'I')
//LDT(signed long,        'l')
//LDT(unsigned long,      'L')
LDT(signed long long,   'u')
LDT(unsigned long long, 'U')
LDT(float,              'f')
LDT(double,             'd')
#ifndef __NO_LONG_DOUBLE_MATH
//LDT(long double,        'D')
#endif

#undef LDT

#define DeclareListTypesExt(BT, P1, P2, TN) \
    typedef BT P1 <signed char>        P2 TN##c; \
    typedef BT P1 <unsigned char>      P2 TN##C; \
    typedef BT P1 <signed short>       P2 TN##s; \
    typedef BT P1 <unsigned short>     P2 TN##S; \
    typedef BT P1 <signed int>         P2 TN##i; \
    typedef BT P1 <unsigned int>       P2 TN##I; \
    /* typedef BT P1 <signed long>        P2 TN##l; */ \
    /* typedef BT P1 <unsigned long>      P2 TN##L; */ \
    typedef BT P1 <signed long long>   P2 TN##u; \
    typedef BT P1 <unsigned long long> P2 TN##U; \
    typedef BT P1 <float>              P2 TN##f; \
    typedef BT P1 <double>             P2 TN##d; \
    /* typedef BT P1 <long double>        P2 TN##D; */


#define COMPOUSE_LDT(TYPE) \
template<typename T> struct LiteraDataType<TYPE<T> >  { enum {litera = LiteraDataType<T>::litera}; typedef typename LiteraDataType<T>::type type; };

COMPOUSE_LDT(Point_)
COMPOUSE_LDT(Point3_)
COMPOUSE_LDT(Size_)
COMPOUSE_LDT(Rect_)
COMPOUSE_LDT(Line_)
COMPOUSE_LDT(Metric_)

#undef COMPOUSE_LDT

#define CMK_NUM(s, t) (((int(s) & 0xff) << 8) | (int(t) & 0xff))

template<typename T> struct ItemDimentionSize               { enum { size = '-' };};
template<typename T> struct ItemDimentionSize< Point_<T> >  { enum { size = '2' }; };
template<typename T> struct ItemDimentionSize< Point3_<T> > { enum { size = '3' }; };

template<typename T> struct PointDataType { enum {
        size = ItemDimentionSize<T>::size,
        type = LiteraDataType<T>::litera,
        num  = CMK_NUM(size, type)
    }; };


template <typename T>
struct StringArg {
    QString _;
    StringArg (QString const &s) : _(s) {}
    inline StringArg<T> &operator () (T val) { _ = _.arg(val); return *this; }
};
template<> struct StringArg<float> {
    QString _;
    StringArg (QString const &s) : _(s) {}
    inline StringArg<float> &operator () (float val) { _ = _.arg(val, 0, 'g', 10); return *this; }
};
template<> struct StringArg<double> {
    QString _;
    StringArg (QString const &s) : _(s) {}
    inline StringArg<double> &operator () (double val) { _ = _.arg(val, 0, 'g', 20); return *this; }
};

//////////////////////////////// Point ////////////////////////////////

/*!
  Шаблон 2D точки.
*/
template<typename T> class Point_
{
public:
    typedef T value_type;

    Point_();
    Point_(T _x, T _y);
    template<typename U> Point_(Point_<U> const &p);
    template<typename U> Point_(Point3_<U> const &p);
//    template<typename U> Point_(const std::vector<U> &pt);
    Point_(QPoint const &p); // для Qt
    Point_(QPointF const &p); // для Qt

    template<typename U> Point_ &operator = (Point_<U> const &p);
    template<typename U> Point_ &operator = (Point3_<U> const &p);
    template<typename U> typename __Enable_If<__HasEl<U>::is_point, Point_>::type &operator = (U const &p);
    template<typename U> typename __Enable_If<__HasEl<U>::is_point_f, Point_>::type &operator = (U const &p);

    template<typename U> operator Point_<U>() const;
    template<typename U> operator U() const;

    // формат "<x> <y>" - например "111 222"
    bool fromString(QString const &text);
    QString toString() const;

    //! длинна вектора
    double length() const;
    //! нормаль к текущему вектору
    Point_ normal() const;
    //! единичный вектор
    Point_ unit() const;
    //! скалярное произведение
    T dot(Point_ const &p) const;
    //! Скалярное произведение двойной точности
    double ddot(Point_ const &p) const;
    //! векторное произведение
    double cross(Point_ const &p) const;
    //! принадлежность точки прямоугольнику
    bool inside(Rect_<T> const &r) const;
    //! проверка на пустоту (x == y == 0)
    bool isNull() const;

    T x, y; // Координаты точки
};

DeclareListTypesExt(Point_, , , Point2)
typedef Point2i Point;


/*!
  Шаблон 3D точки.
*/
template<typename T> class Point3_
{
public:
    typedef T value_type;

    Point3_();
    Point3_(T _x, T _y);
    Point3_(T _x, T _y, T _z);
    template<typename U> Point3_(Point3_<U> const &p);
    template<typename U> Point3_(Point_<U> const &p);
    //template<typename U> Point3_(std::vector<U> const &p);
    Point3_(QPoint const &p); // для Qt
    Point3_(QPointF const &p); // для Qt

    template<typename U> typename __Enable_If<__HasEl<U>::is_point_s, Point3_>::type &operator = (U const &p);
    template<typename U> typename __Enable_If<__HasEl<U>::is_point_f_s, Point3_>::type &operator = (U const &p);
    template<typename U> typename __Enable_If<__HasEl<U>::is_point3, Point3_>::type &operator = (U const &p);
    template<typename U> typename __Enable_If<__HasEl<U>::is_point3_f, Point3_>::type &operator = (U const &p);

    template<typename U> operator Point3_<U>() const;
    template<typename U> operator U() const;

    // формат "<x> <y> <z>" - например "111 222 333"
    bool fromString(QString const &text);
    QString toString() const;

    //! длинна вектора
    double length() const;
    //! нормаль к текущему вектору (в плоскости XY, Z не учитывается)
    Point3_ normalXY() const;
    //! единичный вектор (в плоскости XY, Z не учитывается)
    Point3_ unitXY() const;
    //! единичный вектор
    Point3_ unit() const;
    //! скалярное произведение
    T dot(Point3_ const &p) const;
    //! Скалярное произведение двойной точности
    double ddot(Point3_ const &p) const;
    //! векторное произведение
    Point3_ cross(Point3_ const &p) const;
    //! проверка на пустоту (x == y == z == 0)
    bool isNull() const;

    T x, y, z; // Координаты точки
};

DeclareListTypesExt(Point3_, , , Point3)


/*!
  Шаблон Размера.
*/
template<typename T> class Size_
{
public:
    typedef T value_type;

    //! various constructors
    Size_();
    Size_(T _width, T _height);
    template<typename U> Size_(Size_<U> const &s);
    template<typename U> Size_(Point_<U> const &p);
    template<typename U> Size_(Point3_<U> const &p);
    Size_(QSize const &s); // для Qt
    Size_(QSizeF const &s); // для Qt

    Size_& operator = (Size_ const &s);
    template<typename U> typename __Enable_If<__HasEl<U>::is_size, Size_>::type &operator = (U const &s);
    template<typename U> typename __Enable_If<__HasEl<U>::is_size_f, Size_>::type &operator = (U const &s);

    template<typename U> operator U() const;

    // формат "<width>х<height>" - например "123x456"
    bool fromString(QString const &text);
    QString toString() const;

    //! площадь (width*height)
    T area() const;

    T width, height; // ширина и высота
};

DeclareListTypesExt(Size_, , ,Size2)
typedef Size2i Size;


/*!
  Шаблон прямоугольника.
*/
template<typename T> class Rect_
{
public:
    typedef T value_type;

    //! various constructors
    Rect_();
    Rect_(T _x, T _y, T _width, T _height);
    Rect_(Rect_ const &r);
    Rect_(Point_<T> const &pos, Size_<T> const &size);
    Rect_(Point_<T> const &p1, Point_<T> const &p2);
    Rect_(QRect const &r); // для Qt
    Rect_(QRectF const &r); // для Qt

    Rect_& operator = (Rect_ const &r);
    template<typename U> typename __Enable_If<__HasEl<U>::is_rect, Rect_>::type &operator = (U const &r);
    template<typename U> typename __Enable_If<__HasEl<U>::is_rect_f, Rect_>::type &operator = (U const &r);

    template<typename U> operator U() const;

    // формат "<x>,<y> <width>х<height>" - например "50,100 123x456"
    bool fromString(QString const &text);
    QString toString() const;

    //! верхний левый угол
    Point_<T> tl() const;
    //! правый нижний угол
    Point_<T> br() const;
    //! центр
    Point_<T> center() const;

    //! размер (ширина, высота) прямоугольника
    Size_<T> size() const;
    //! площадь (ширина*высота) прямоугольника
    T area() const;

    //! проверить на вхождение точки
    bool contains(Point_<T> const &p) const;

    T x, y, width, height; // левый верхний угол и ширина, высота прямоугольника
};

DeclareListTypesExt(Rect_, , , Rect2)
typedef Rect2i Rect;


/*!
  Шаблон массива.
*/
template<typename T> class Line_ : public std::vector<T>
{
public:
    typedef T                                               value_type;
    typedef typename std::vector<T>::pointer                pointer;
    typedef typename std::vector<T>::const_pointer          const_pointer;
    typedef typename std::vector<T>::reference              reference;
    typedef typename std::vector<T>::const_reference        const_reference;
    typedef typename std::vector<T>::iterator               iterator;
    typedef typename std::vector<T>::const_iterator         const_iterator;
    typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
    typedef typename std::vector<T>::reverse_iterator		reverse_iterator;
    typedef typename std::vector<T>::size_type              size_type;
    typedef typename std::vector<T>::difference_type		difference_type;
    typedef typename std::vector<T>::allocator_type         allocator_type;

    Line_();
    Line_(T const &a0);
    Line_(T const &a0, T const &a1);
    Line_(T const &a0, T const &a1, T const &a2);
    Line_(T const &a0, T const &a1, T const &a2, T const &a3);

    Line_(QPoint const &p); // для Qt
    Line_(QPointF const &p); // для Qt
    Line_(QPolygon const &l); // для Qt
    Line_(QPolygonF const &l); // для Qt

    template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Line_>::type &operator = (U const &e);
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, Line_>::type &operator = (U const &l);

    template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Line_>::type &append(U const &e);
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, Line_>::type &append(U const  &l);

    template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Line_>::type &operator << (U const &e);
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, Line_>::type &operator << (U const  &l);

    // формат "<a0>,<a1>,...<an>" - например "50,100,150,200"
    bool fromString(QString const &text);
    QString toString() const;

    // описывающий прямоугольник
    Rect_<typename T::value_type> boundingRect() const;

    // доступ к подметрике. отрабатывает без ошибок при выходе за границу размера
    const_reference atE(size_type n) const;
    reference atE(size_type n);

    // преобразовать в другой контейнер.
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, U>::type array() const;
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, U>::type array(U &) const;

#if defined( GCC_11 )
    static_assert(
            __HasEl<T>::is_point_f,
            "Template argument T must be a point type in class template Line_"
            );
#endif
};

DeclareListTypesExt(Line_, <Point_,  >, Line2)
DeclareListTypesExt(Line_, <Point3_, >, Line3)
typedef Line2i Line;


/*!
  Шаблон Метрики.
*/

template<typename T> class Metric_ : public std::vector<T>
{
public:
    typedef T                                               value_type;
    typedef typename std::vector<T>::pointer                pointer;
    typedef typename std::vector<T>::const_pointer          const_pointer;
    typedef typename std::vector<T>::reference              reference;
    typedef typename std::vector<T>::const_reference        const_reference;
    typedef typename std::vector<T>::iterator               iterator;
    typedef typename std::vector<T>::const_iterator         const_iterator;
    typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
    typedef typename std::vector<T>::reverse_iterator		reverse_iterator;
    typedef typename std::vector<T>::size_type              size_type;
    typedef typename std::vector<T>::difference_type		difference_type;
    typedef typename std::vector<T>::allocator_type         allocator_type;

    Metric_();
    Metric_(T const &l);
    Metric_(typename T::value_type const &e);
    Metric_(QString const &text);
    explicit Metric_(QByteArray const &data);

    Metric_(QPoint const &p); // для Qt
    Metric_(QPointF const &p); // для Qt
    Metric_(QPolygon const &l); // для Qt
    Metric_(QPolygonF const &l); // для Qt

    template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Metric_>::type &operator = (U const &p); // точка
    template<typename U> typename __Enable_If<__Bool_And<__HasEl<U>::is_container, __HasEl<typename U::value_type>::is_point_>::value, Metric_>::type &operator = (U const &l); // подметрика
    template<typename U> typename __Enable_If<__HasEl<typename U::value_type>::is_container, Metric_>::type &operator = (U const &v); // метрика

    // Добавить подметрику
    template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Metric_>::type &append(U const &p); // точка
    template<typename U> typename __Enable_If<__HasEl<typename U::value_type>::is_point_, Metric_>::type &append(U const  &l); // подметрика
    template<typename U> typename __Enable_If<__HasEl<typename U::value_type>::is_container, Metric_>::type &append(U const  &v); // метрика

    template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Metric_>::type &operator << (U const  &p); // точка
    template<typename U> typename __Enable_If<__HasEl<typename U::value_type>::is_point_, Metric_>::type &operator << (U const  &l); // подметрика
    template<typename U> typename __Enable_If<__HasEl<typename U::value_type>::is_container, Metric_>::type &operator << (U const  &v); // метрика

    // есть хоть одна не пустая подметрика
    bool isNotEmpty() const;

    void fromByteArray(QByteArray const &data);
    QByteArray toByteArray() const;

    // формат строки:  x1 y1 z1, x2 y2 z2; x3 y3 z3
    // т.е. подметрики разделяются - ";" (точка с запятой)
    // точки разделяются - "," (запятая)
    // а значения по осям - " " (пробел)
    // лишние украшательские пробелы - игнорируются
    bool fromString(QString const &text);
    QString toString() const;

    // объединить все подметрики в одну
    T join() const;

    // доступ к подметрике. отрабатывает без ошибок при выходе за границу
    const_reference atE(size_type n) const;
    reference atE(size_type n);

    // описывающий прямоугольник
    Rect_<typename LiteraDataType<T>::type> boundingRect() const;

    // вернуть точку. параметры: номер точки, номер метрики
    typename T::value_type const &point(size_type pos = 0, typename T::size_type metric = 0) const;
    // задать точку. параметры: номер точки, номер метрики
    template<typename U> typename __Enable_If<__HasEl<U>::is_point_>::type setPoint(U const &p, size_type pos = 0, typename T::size_type metric = 0);

    // вернуть точку. аналог point но не корится в случае выхода за границы метрик
    typename T::value_type const &pointE(size_type pos = 0, typename T::size_type metric = 0) const;
    // задать точку. аналог setPoint но не корится в случае выхода за границы метрик
    template<typename U> typename __Enable_If<__HasEl<U>::is_point_>::type setPointE(U const &p, size_type pos = 0, typename T::size_type metric = 0);

    // узнать угол метрики в радианах
    // параметры: первый номер метрики, первый номер точки, второй номер метрики, второй номер точки
    template<int M1, int P1, int M2, int P2>
    double angle() const;
    // задать угол метрики в радианах
    // параметры: первый номер метрики, первый номер точки, второй номер метрики, второй номер точки
    template<int M1, int P1, int M2, int P2>
    void setAngle(double alpha);
    // эквивалент angle<0,0, 1,0>
    double angle() const;
    // эквивалент setAngle<0,0, 1,0>
    void setAngle(double alpha);

    // узнать угол метрики в радианах
    // аналог angle<> но не корится в случае выхода за границы метрик
    template<int M1, int P1, int M2, int P2>
    double angleE() const;
    // задать угол метрики в радианах
    // аналог setAngle<> но не корится в случае выхода за границы метрик
    template<int M1, int P1, int M2, int P2>
    void setAngleE(double alpha);
    // эквивалент angleE<0,0, 1,0>
    double angleE() const;
    // эквивалент setAngleE<0,0, 1,0>
    void setAngleE(double alpha);

    // перевод метрики в полигон. метрики записываются в полигон по порядку друг за другом
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, U>::type array() const;
    template<typename U> typename __Enable_If<__HasEl<U>::is_container, U>::type array(U &) const;

#if defined( GCC_11 )
    static_assert(
            __HasEl<T>::is_container,
            "Template argument T must be a container type in class template Metric_"
            );
#endif
};

DeclareListTypesExt(Metric_ < Line_, <Point_,  > >, Metric2)
DeclareListTypesExt(Metric_ < Line_, <Point3_, > >, Metric3)
typedef Metric2i Metric;


#undef DeclareListTypesExt

//////////////////////////////// 2D Point ////////////////////////////////

template<typename T> inline Point_<T>::Point_() : x(FunctorDataType<T>::getZero()), y(FunctorDataType<T>::getZero()) {}
template<typename T> inline Point_<T>::Point_(T _x, T _y) : x(_x), y(_y) {}

template<typename T> template<typename U> inline Point_<T>::Point_(Point_<U> const &p) : x(p.x), y(p.y) {}
template<typename T> template<typename U> inline Point_<T>::Point_(Point3_<U> const &p) : x(p.x), y(p.y) {}
//template<typename T> template<typename TP> inline Point_<T>::Point_(const std::vector<TP> &pt) { x = pt[0]; y = pt[1]; }
template<typename T> inline Point_<T>::Point_(QPoint const &p) : x(p.x()), y(p.y()) { }
template<typename T> inline Point_<T>::Point_(QPointF const &p) : x(p.x()), y(p.y()) { }


template<typename T> template<typename U> inline Point_<T>& Point_<T>::operator = (Point_<U> const &p)
{ x = p.x; y = p.y; return *this; }
template<typename T> template<typename U> inline Point_<T> &Point_<T>::operator = (Point3_<U> const &p)
{ x = p.x; y = p.y; return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point, Point_<T> >::type  &Point_<T>::operator = (U const &p)
{ x = p.x(); y = p.y(); return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_f, Point_<T> >::type  &Point_<T>::operator = (U const &p)
{ x = p.x; y = p.y; return *this; }

template<typename T> template<typename U> inline Point_<T>::operator Point_<U>() const { return Point_<U>(x, y); }
template<typename T> template<typename U> inline Point_<T>::operator U() const { return U(x, y); }

template<typename T> inline bool Point_<T>::fromString(QString const &text) {
    QStringList sl = text.split(" ", QString::SkipEmptyParts);
    //Q_ASSERT(sl.size() >= 2);
    if (sl.size() < 2) return false;
    bool okx, oky;
    x = FunctorDataType<T>::fromString(sl.at(0), &okx);
    y = FunctorDataType<T>::fromString(sl.at(1), &oky);
    return okx && oky;
}

template<typename T> inline QString Point_<T>::toString() const {
    return StringArg<T>(QString("%1 %2"))(x)(y)._;
}

template<typename T> inline double Point_<T>::length() const
{ return sqrt((double)x*x + (double)y*y); }

template<typename T> inline Point_<T> Point_<T>::normal() const
{ return Point_<T>(y, -x); }

template<typename T> inline Point_<T> Point_<T>::unit() const
{   double l = length();
    if (qFuzzyIsNull(l)) return Point_<T>();
    return Point_<T>(double(x)/l, double(y)/l); }

template<typename T> inline T Point_<T>::dot(Point_ const &p) const
{ return T(x*p.x + y*p.y); }
template<typename T> inline double Point_<T>::ddot(Point_ const &p) const
{ return (double)x*p.x + (double)y*p.y; }

template<typename T> inline double Point_<T>::cross(Point_ const &p) const
{ return (double)x*p.y - (double)y*p.x; }

template<typename T> inline bool Point_<T>::isNull() const
{ return FunctorDataType<T>::isNull(x) && FunctorDataType<T>::isNull(y); }

template<typename T> static inline Point_<T> &operator += (Point_<T> &a, Point_<T> const &b)
{ a.x += b.x; a.y += b.y; return a; }

template<typename T> static inline Point_<T> &operator -= (Point_<T> &a, Point_<T> const &b)
{ a.x += b.x; a.y += b.y; return a; }

template<typename T> static inline Point_<T> &operator += (Point_<T> &a, Point3_<T> const &b)
{ a.x += b.x; a.y += b.y; return a; }

template<typename T> static inline Point_<T> &operator -= (Point_<T> &a, Point3_<T> const &b)
{ a.x += b.x; a.y += b.y; return a; }

template<typename T, typename S>
static inline Point_<T> &operator *= (Point_<T> &a, S b)
{ a.x *= b; a.y *= b; return a; }

template<typename T> static inline bool operator == (Point_<T> const &a, Point_<T> const &b)
{ return a.x == b.x && a.y == b.y; }

template<typename T> static inline bool operator != (Point_<T> const &a, Point_<T> const &b)
{ return a.x != b.x || a.y != b.y; }

template<typename T> static inline Point_<T> operator + (Point_<T> const &a, Point_<T> const &b)
{ return Point_<T>(a.x + b.x, a.y + b.y); }

template<typename T> static inline Point_<T> operator - (Point_<T> const &a, Point_<T> const &b)
{ return Point_<T>(a.x - b.x, a.y - b.y); }

template<typename T> static inline Point_<T> operator - (Point_<T> const &a)
{ return Point_<T>(-a.x, -a.y); }

template<typename T, typename S>
static inline Point_<T> operator * (Point_<T> const &a, S b)
{ return Point_<T>(a.x * b, a.y * b); }

template<typename T, typename S>
static inline Point_<T> operator * (S a, const Point_<T> &b)
{ return Point_<T>(b.x * a, b.y * a); }

template<typename T> inline bool Point_<T>::inside(Rect_<T> const &r) const
{ return r.contains(*this); }


//////////////////////////////// 3D Point ////////////////////////////////

template<typename T> inline Point3_<T>::Point3_() : x(FunctorDataType<T>::getZero()), y(FunctorDataType<T>::getZero()), z(FunctorDataType<T>::getNan()) {}
template<typename T> inline Point3_<T>::Point3_(T _x, T _y) : x(_x), y(_y) { z = FunctorDataType<T>::getNan(); }
template<typename T> inline Point3_<T>::Point3_(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
template<typename T> template<typename U> inline Point3_<T>::Point3_(Point3_<U> const &p) : x(p.x), y(p.y), z(p.z) {}
template<typename T> template<typename U> inline Point3_<T>::Point3_(Point_<U> const &p) : x(p.x), y(p.y) { z = FunctorDataType<T>::getNan(); }
//template<typename T> template<typename TP> inline Point3_<T>::Point3_(const std::vector<TP> &pt) { x = pt[0]; y = pt[1]; z = pt[2]; }
template<typename T> inline Point3_<T>::Point3_(QPoint const &p) : x(p.x()), y(p.y()) { z = FunctorDataType<T>::getNan(); }
template<typename T> inline Point3_<T>::Point3_(QPointF const &p) : x(p.x()), y(p.y()) { z = FunctorDataType<T>::getNan(); }

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_s, Point3_<T> >::type &Point3_<T>::operator = (U const &p)
{ x = p.x(); y = p.y(); z = FunctorDataType<T>::getNan(); return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_f_s, Point3_<T> >::type &Point3_<T>::operator = (U const &p)
{ x = p.x; y = p.y; z = FunctorDataType<T>::getNan(); return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point3, Point3_<T> >::type &Point3_<T>::operator = (U const &p)
{ x = p.x(); y = p.y(); z = p.z(); return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point3_f, Point3_<T> >::type &Point3_<T>::operator = (U const &p)
{ x = p.x; y = p.y; z = p.z; return *this; }

template<typename T> template<typename U> inline Point3_<T>::operator Point3_<U>() const
{ return Point3_<U>(x, y, z); }
template<typename T> template<typename U> inline Point3_<T>::operator U() const { return U(x, y); }

template<typename T> inline bool Point3_<T>::fromString(QString const &text) {
    QStringList sl = text.split(" ", QString::SkipEmptyParts);
    //Q_ASSERT(sl.size() >= 3);
    if (sl.size() < 3) return false;
    bool okx, oky, okz;
    x = FunctorDataType<T>::fromString(sl.at(0), &okx);
    y = FunctorDataType<T>::fromString(sl.at(1), &oky);
    z = FunctorDataType<T>::fromString(sl.at(2), &okz);
    return okx && oky && okz;
}

template<typename T> inline QString Point3_<T>::toString() const {
    return StringArg<T>(QString("%1 %2 %3"))(x)(y)(z)._;
}

template<typename T> inline double Point3_<T>::length() const
{
    if (FunctorDataType<T>::isNan(z))
        return sqrt((double)x*x + (double)y*y);
    return sqrt((double)x*x + (double)y*y + (double)z*z); }

template<typename T> inline Point3_<T> Point3_<T>::normalXY() const
{ return Point3_<T>(y, -x, z); }

template<typename T> inline Point3_<T> Point3_<T>::unitXY() const
{   double l = sqrt((double)x*x + (double)y*y);
    if (qFuzzyIsNull(l)) return Point3_<T>();
    return Point3_<T>(double(x)/l, double(y)/l, z); }

template<typename T> inline Point3_<T> Point3_<T>::unit() const
{   double l = length();
    if (qFuzzyIsNull(l)) return Point3_<T>();
    return Point3_<T>(double(x)/l, double(y)/l, double(z)/l); }

template<typename T> inline T Point3_<T>::dot(Point3_ const &p) const
{ return T(x*p.x + y*p.y + z*p.z); }
template<typename T> inline double Point3_<T>::ddot(Point3_ const &p) const
{ return (double)x*p.x + (double)y*p.y + (double)z*p.z; }

template<typename T> inline Point3_<T> Point3_<T>::cross(Point3_<T> const &p) const
{ return Point3_<T>(y*p.z - z*p.y, z*p.x - x*p.z, x*p.y - y*p.x); }

template<typename T> inline bool Point3_<T>::isNull() const
{ return FunctorDataType<T>::isNull(x) && FunctorDataType<T>::isNull(y) && FunctorDataType<T>::isNull(z); }

template<typename T> static inline Point3_<T> &operator += (Point3_<T> &a, const Point3_<T> &b)
{ a.x += b.x; a.y += b.y; a.z += b.z; return a; }

template<typename T> static inline Point3_<T> &operator += (Point3_<T> &a, const Point_<T> &b)
{ a.x += b.x; a.y += b.y; return a; }

template<typename T> static inline Point3_<T> &operator -= (Point3_<T> &a, const Point3_<T> &b)
{ a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }

template<typename T> static inline Point3_<T> &operator -= (Point3_<T> &a, const Point_<T> &b)
{ a.x -= b.x; a.y -= b.y; return a; }

template<typename T, typename S>
static inline Point3_<T> &operator *= (Point3_<T> &a, S b)
{ a.x = a.x*b; a.y = a.y*b; a.z = a.z*b; return a; }

template<typename T> static inline double norm(Point3_<T> const &p)
{ return sqrt((double)p.x*p.x + (double)p.y*p.y + (double)p.z*p.z); }

template<typename T> static inline bool operator == (Point3_<T> const &a, Point3_<T> const &b)
{ return a.x == b.x && a.y == b.y && a.z == b.z; }

template<typename T> static inline bool operator != (Point3_<T> const &a, Point3_<T> const &b)
{ return a.x != b.x || a.y != b.y || a.z != b.z; }

template<typename T> static inline Point3_<T> operator + (Point3_<T> const &a, Point3_<T> const &b)
{ return Point3_<T>(a.x + b.x, a.y + b.y, a.z + b.z); }

template<typename T> static inline Point3_<T> operator + (Point3_<T> const &a, Point_<T> const &b)
{ return Point3_<T>(a.x + b.x, a.y + b.y, a.z); }

template<typename T> static inline Point3_<T> operator + (Point_<T> const &a, Point3_<T> const &b)
{ return Point3_<T>(a.x + b.x, a.y + b.y, b.z); }

template<typename T> static inline Point3_<T> operator - (Point3_<T> const &a, Point3_<T> const &b)
{ return Point3_<T>(a.x - b.x, a.y - b.y, a.z - b.z); }

template<typename T> static inline Point3_<T> operator - (Point3_<T> const &a, Point_<T> const &b)
{ return Point3_<T>(a.x - b.x, a.y - b.y, a.z); }

template<typename T> static inline Point3_<T> operator - (Point_<T> const &a, Point3_<T> const &b)
{ return Point3_<T>(a.x - b.x, a.y - b.y, - b.z); }

template<typename T> static inline Point3_<T> operator - (Point3_<T> const &a)
{ return Point3_<T>(-a.x, -a.y, -a.z); }

template<typename T, typename S>
static inline Point3_<T> operator * (Point3_<T> const &a, S b)
{ return Point3_<T>(a.x * b, a.y * b, a.z * b); }

template<typename T, typename S>
static inline Point3_<T> operator * (S a, Point3_<T> const &b)
{ return Point3_<T>(b.x * a, b.y * a, b.z * a); }


//////////////////////////////// Size ////////////////////////////////

template<typename T> inline Size_<T>::Size_()
    : width(0), height(0) {}
template<typename T> inline Size_<T>::Size_(T _width, T _height)
    : width(_width), height(_height) {}

template<typename T> template<typename U> inline Size_<T>::Size_(Size_<U> const &s)
    : width(s.width), height(s.height) {}

template<typename T> template<typename U> inline Size_<T>::Size_(Point_<U> const &p)
    : width(p.x), height(p.y) {}

template<typename T> template<typename U> inline Size_<T>::Size_(Point3_<U> const &p)
    : width(p.x), height(p.y) {}
template<typename T> inline Size_<T>::Size_(QSize const &s)
    : width(s.width()), height(s.height()) {}
template<typename T> inline Size_<T>::Size_(QSizeF const &s)
    : width(s.width()), height(s.height()) {}

template<typename T> inline Size_<T> &Size_<T>::operator = (Size_<T> const &s)
{ width = s.width; height = s.height; return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_size, Size_<T> >::type &Size_<T>::operator = (U const &s)
{ width = s.width(); height = s.height(); return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_size_f, Size_<T> >::type &Size_<T>::operator = (U const &s)
{ width = s.width; height = s.height; return *this; }

template<typename T> template<typename U> inline Size_<T>::operator U() const
{ return U(width, height); }

template<typename T> inline bool Size_<T>::fromString(QString const &text) {
    QStringList sl = text.split("x", QString::SkipEmptyParts);
    //Q_ASSERT(sl.size() >= 2);
    if (sl.size() < 2) return false;
    bool okw, okh;
    width  = FunctorDataType<T>::fromString(sl.at(0), &okw);
    height = FunctorDataType<T>::fromString(sl.at(1), &okh);
    return okw && okh;
}

template<typename T> inline QString Size_<T>::toString() const {
    return StringArg<T>(QString("%1x%2"))(width)(height)._;
}

template<typename T> inline T Size_<T>::area() const { return width * height; }

template<typename T> static inline Size_<T> operator * (Size_<T> const &a, T b)
{ return Size_<T>(a.width * b, a.height * b); }
template<typename T> static inline Size_<T> operator + (Size_<T> const &a, Size_<T> const &b)
{ return Size_<T>(a.width + b.width, a.height + b.height); }
template<typename T> static inline Size_<T> operator - (Size_<T> const &a, Size_<T> const &b)
{ return Size_<T>(a.width - b.width, a.height - b.height); }

template<typename T> static inline Size_<T>& operator *= (Size_<T> &a, T b)
{ a.width *= b; a.height *= b; return a; }
template<typename T> static inline Size_<T>& operator += (Size_<T> &a, Size_<T> const &b)
{ a.width += b.width; a.height += b.height; return a; }
template<typename T> static inline Size_<T>& operator -= (Size_<T> &a, Size_<T> const &b)
{ a.width -= b.width; a.height -= b.height; return a; }

template<typename T> static inline bool operator == (Size_<T> const &a, Size_<T> const &b)
{ return a.width == b.width && a.height == b.height; }
template<typename T> static inline bool operator != (Size_<T> const &a, Size_<T> const &b)
{ return a.width != b.width || a.height != b.height; }


//////////////////////////////// Rect ////////////////////////////////

template<typename T> inline Rect_<T>::Rect_() : x(0), y(0), width(0), height(0) {}
template<typename T> inline Rect_<T>::Rect_(T _x, T _y, T _width, T _height) : x(_x), y(_y), width(_width), height(_height) {}
template<typename T> inline Rect_<T>::Rect_(Rect_<T> const &r) : x(r.x), y(r.y), width(r.width), height(r.height) {}

template<typename T> inline Rect_<T>::Rect_(Point_<T> const &pos, Size_<T> const &size)
    : x(pos.x), y(pos.y), width(size.width), height(size.height) {}
template<typename T> inline Rect_<T>::Rect_(Point_<T> const &p1, Point_<T> const &p2)
{
    x = std::min(p1.x, p2.x); y = std::min(p1.y, p2.y);
    width = std::max(p1.x, p2.x) - x; height = std::max(p1.y, p2.y) - y;
}
template<typename T> inline Rect_<T>::Rect_(QRect const &r) : x(r.x()), y(r.y()), width(r.width()), height(r.height()) {}
template<typename T> inline Rect_<T>::Rect_(QRectF const &r) : x(r.x()), y(r.y()), width(r.width()), height(r.height()) {}

template<typename T> inline Rect_<T> &Rect_<T>::operator = (Rect_<T> const &r)
{ x = r.x; y = r.y; width = r.width; height = r.height; return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_rect, Rect_<T> >::type &Rect_<T>::operator = (U const &r)
{ x = r.x(); y = r.y(); width = r.width(); height = r.height(); return *this; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_rect_f, Rect_<T> >::type &Rect_<T>::operator = (U const &r)
{ x = r.x; y = r.y; width = r.width; height = r.height; return *this; }

template<typename T> template<typename U> inline Rect_<T>::operator U() const
{ return U(x, y, width, height); }

template<typename T> inline bool Rect_<T>::fromString(QString const &text) {
    QStringList sl = text.split(QRegExp("[, xX]"), QString::SkipEmptyParts);
    //Q_ASSERT(sl.size() >= 4);
    if (sl.size() < 4) return false;
    bool okx, oky, okw, okh;
    x      = FunctorDataType<T>::fromString(sl.at(0), &okx);
    y      = FunctorDataType<T>::fromString(sl.at(1), &oky);
    width  = FunctorDataType<T>::fromString(sl.at(2), &okw);
    height = FunctorDataType<T>::fromString(sl.at(3), &okh);
    return okx && oky && okw && okh;
}

template<typename T> inline QString Rect_<T>::toString() const {
    return StringArg<T>(QString("%1,%2 %2x%3"))(x)(y)(width)(height)._;
}

template<typename T> inline Point_<T> Rect_<T>::tl() const { return Point_<T>(x, y); }
template<typename T> inline Point_<T> Rect_<T>::br() const { return Point_<T>(x+width, y+height); }
template<typename T> inline Point_<T> Rect_<T>::center() const { return Point_<T>(x+width/2.0, y+height/2.0); }

template<typename T> inline Size_<T> Rect_<T>::size() const { return Size_<T>(width, height); }
template<typename T> inline T Rect_<T>::area() const { return width*height; }

template<typename T> static inline Rect_<T> &operator += (Rect_<T> &a, Point_<T> const &b)
{ a.x += b.x; a.y += b.y; return a; }
template<typename T> static inline Rect_<T> &operator -= (Rect_<T> &a, Point_<T> const &b)
{ a.x -= b.x; a.y -= b.y; return a; }
template<typename T> static inline Rect_<T> &operator += (Rect_<T> &a, Size_<T> const &b)
{ a.width += b.width; a.height += b.height; return a; }
template<typename T> static inline Rect_<T> &operator -= (Rect_<T> &a, Size_<T> const &b)
{ a.width -= b.width; a.height -= b.height; return a; }

template<typename T> static inline Rect_<T> &operator &= (Rect_<T> &a, Rect_<T> const &b)
{
    T x1 = std::max(a.x, b.x), y1 = std::max(a.y, b.y);
    a.width  = std::min(a.x + a.width,  b.x + b.width)  - x1;
    a.height = std::min(a.y + a.height, b.y + b.height) - y1;
    a.x = x1; a.y = y1;
    if( a.width <= 0 || a.height <= 0 )
        a = Rect_<T>();
    return a;
}
template<typename T> static inline Rect_<T> &operator |= (Rect_<T> &a, Rect_<T> const &b)
{
    T x1 = std::min(a.x, b.x), y1 = std::min(a.y, b.y);
    a.width  = std::max(a.x + a.width,  b.x + b.width)  - x1;
    a.height = std::max(a.y + a.height, b.y + b.height) - y1;
    a.x = x1; a.y = y1;
    return a;
}

template<typename T> inline bool Rect_<T>::contains(Point_<T> const &p) const
{ return x <= p.x && p.x < x + width && y <= p.y && p.y < y + height; }

template<typename T> static inline bool operator == (Rect_<T> const &a, Rect_<T> const &b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

template<typename T> static inline bool operator != (Rect_<T> const &a, Rect_<T> const &b)
{
    return a.x != b.x || a.y != b.y || a.width != b.width || a.height != b.height;
}

template<typename T> static inline Rect_<T> operator + (Rect_<T> const &a, Point_<T> const &b)
{
    return Rect_<T>( a.x + b.x, a.y + b.y, a.width, a.height );
}

template<typename T> static inline Rect_<T> operator - (Rect_<T> const &a, Point_<T> const &b)
{
    return Rect_<T>( a.x - b.x, a.y - b.y, a.width, a.height );
}

template<typename T> static inline Rect_<T> operator + (Rect_<T> const &a, Size_<T> const &b)
{
    return Rect_<T>( a.x, a.y, a.width + b.width, a.height + b.height );
}

template<typename T> static inline Rect_<T> operator & (Rect_<T> const &a, Rect_<T> const &b)
{
    Rect_<T> c = a;
    return c &= b;
}

template<typename T> static inline Rect_<T> operator | (Rect_<T> const &a, Rect_<T> const &b)
{
    Rect_<T> c = a;
    return c |= b;
}


//////////////////////////////// Line ////////////////////////////////

template<typename T> inline Line_<T>::Line_() {}
template<typename T> inline Line_<T>::Line_(T const &a0) { this->push_back(a0); }
template<typename T> inline Line_<T>::Line_(T const &a0, T const &a1) { this->push_back(a0); this->push_back(a1); }
template<typename T> inline Line_<T>::Line_(T const &a0, T const &a1, T const &a2) { this->push_back(a0); this->push_back(a1); this->push_back(a2); }
template<typename T> inline Line_<T>::Line_(T const &a0, T const &a1, T const &a2, T const &a3) { this->push_back(a0); this->push_back(a1); this->push_back(a2); this->push_back(a3); }

template<typename T> inline Line_<T>::Line_(QPoint const &p) { this->push_back(p); }
template<typename T> inline Line_<T>::Line_(QPointF const &p) { this->push_back(p); }
template<typename T> inline Line_<T>::Line_(QPolygon const &l) {
    this->resize(l.size());
    std::copy(l.begin(), l.end(), this->begin());
}
template<typename T> inline Line_<T>::Line_(QPolygonF const &l) {
    this->resize(l.size());
    std::copy(l.begin(), l.end(), this->begin());
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_, Line_<T> >::type &Line_<T>::operator = (U const &e) {
    this->clear();
    this->push_back(e);
    return *this;
}
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, Line_<T> >::type &Line_<T>::operator = (U const &v) {
    this->clear();
    this->resize(v.size());
    std::copy(v.begin(), v.end(), this->begin());
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_, Line_<T> >::type &Line_<T>::append(U const &e) {
    push_back(e);
    return *this;
}
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, Line_<T> >::type &Line_<T>::append(U const &l) {
    int n = this->size();
    this->resize(n + l.size());
    std::copy(l.begin(), l.end(), this->begin() + n);
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_, Line_<T> >::type &Line_<T>::operator << (U const &e) { return this->append(e); }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, Line_<T> >::type &Line_<T>::operator << (U const &l) { return this->append(l); }

template<typename T> inline bool Line_<T>::fromString(QString const &text) {
#if defined( GCC_11 )
    static_assert(__HasEl<T>::method_fromString, "Template argument T must contains toString function");
#endif
    QStringList sl = text.split(",", QString::SkipEmptyParts);
    this->clear();
    this->reserve(sl.size());
    T item;
    for (QStringList::const_iterator it = sl.begin(), itEnd = sl.end(); it != itEnd; ++it) {
        item.fromString(*it);
        this->push_back(item);
    }
    return true;
}

template<typename T> inline QString Line_<T>::toString() const {
#if defined( GCC_11 )
    static_assert(__HasEl<T>::method_toString, "Template argument T must contains toString function");
#endif
    QStringList result;
    result.reserve(this->size());
    for (const_iterator it = this->begin(), itEnd = this->end(); it != itEnd; ++it) {
        result.push_back(it->toString());
    }
    return result.join(", ");
}

template<typename T> inline Rect_<typename T::value_type> Line_<T>::boundingRect() const
{
    typedef typename T::value_type V;
    if (this->empty())
        return Rect_<V>();
    V minx, maxx, miny, maxy;
    const_iterator it    = this->begin();
    const_iterator itEnd = this->end();
    minx = maxx = it->x;
    miny = maxy = it->y;
    for (++it; it != itEnd; ++it) {
        if (it->x < minx)      minx = it->x;
        else if (it->x > maxx) maxx = it->x;
        if (it->y < miny)      miny = it->y;
        else if (it->y > maxy) maxy = it->y;
    }
    return Rect_<V>(Point_<V>(minx, miny), Point_<V>(maxx, maxy));
}

template<typename T> inline typename Line_<T>::const_reference Line_<T>::atE(typename Line_<T>::size_type n) const {
    static T _;
    if (n >= this->size() || n < 0)
        return _;
    return (*this)[n];
}

template<typename T> inline typename Line_<T>::reference Line_<T>::atE(typename Line_<T>::size_type n)  {
    static T _;
    if (n >= this->size() || n < 0)
        return _;
    return (*this)[n];
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, U>::type Line_<T>::array() const {
    U result;
    array(result);
    return result;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, U>::type Line_<T>::array(U &result) const {
#if defined( GCC_11 )
    static_assert(__HasEl<U>::is_container, "Template argument T must by container type");
#endif
    size_type n = result.size();
    result.resize(n + this->size());
    std::copy(this->begin(), this->end(), result.begin() + n);
    return result;
}

template<typename T> static inline bool operator == (Line_<T> const &a, Line_<T> const &b) {
    if (a.size() != b.size())
        return false;
    typename Line_<T>::const_iterator ab = a.begin();
    typename Line_<T>::const_iterator ae = a.end();
    typename Line_<T>::const_iterator bb = b.begin();
    for (; ab != ae; ++ab, ++bb)
        if (*ab != *bb)
            return false;
    return true;
}

template<typename T> static inline bool operator != (Line_<T> const &a, Line_<T> const &b)
{ return !(a == b); }

template<typename T> static inline Line_<T> operator + (Line_<T> const &a, Point_<T> const &b)
{
    Line_<T> result;
    result.reserve(a.size());
    for (typename Line_<T>::const_iterator it = a.begin(), itEnd = a.end(); it != itEnd; ++it)
        result.push_back(*it + b);
    return result;
}

template<typename T> static inline Line_<T> operator + (Line_<T> const &a, Point3_<T> const &b)
{
    Line_<T> result;
    result.reserve(a.size());
    for (typename Line_<T>::const_iterator it = a.begin(), itEnd = a.end(); it != itEnd; ++it)
        result.push_back(*it + b);
    return result;
}

template<typename T> static inline Line_<T> operator - (Line_<T> const &a, Point_<T> const &b)
{
    Line_<T> result;
    result.reserve(a.size());
    for (typename Line_<T>::const_iterator it = a.begin(), itEnd = a.end(); it != itEnd; ++it)
        result.push_back(*it - b);
    return result;
}

template<typename T> static inline Line_<T> operator - (Line_<T> const &a, Point3_<T> const &b)
{
    Line_<T> result;
    result.reserve(a.size());
    for (typename Line_<T>::const_iterator it = a.begin(), itEnd = a.end(); it != itEnd; ++it)
        result.push_back(*it - b);
    return result;
}


//////////////////////////////// Metric ////////////////////////////////

template<typename T> inline Metric_<T>::Metric_() {}
template<typename T> inline Metric_<T>::Metric_(T const &e) { this->push_back(e); }
template<typename T> inline Metric_<T>::Metric_(typename T::value_type const &e) { this->resize(1); this->begin()->push_back(e); }
template<typename T> inline Metric_<T>::Metric_(QString const&text) { fromString(text); }
template<typename T> inline Metric_<T>::Metric_(QByteArray const &data) { fromByteArray(data); }

template<typename T> inline Metric_<T>::Metric_(QPoint const &p) { this->resize(1); this->begin()->push_back(p); }
template<typename T> inline Metric_<T>::Metric_(QPointF const &p) { this->resize(1); this->begin()->push_back(p); }
template<typename T> inline Metric_<T>::Metric_(QPolygon const &l) { this->push_back(l); }
template<typename T> inline Metric_<T>::Metric_(QPolygonF const &l) { this->push_back(l); }


template<typename T> template<typename U> typename __Enable_If<__HasEl<U>::is_point_, Metric_<T> >::type &Metric_<T>::operator = (U const &p)
{
    this->resize(1);
    iterator it = this->begin();
    it->clear();
    it->push_back(p);
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__Bool_And<__HasEl<U>::is_container, __HasEl<typename U::value_type>::is_point_>::value, Metric_<T> >::type &Metric_<T>::operator = (U const &l) {
    this->clear();
    this->push_back(l);
    return *this;
}
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<typename U::value_type>::is_container, Metric_<T> >::type &Metric_<T>::operator = (U const &v) {
    this->clear();
    this->resize(v.size());
    std::copy(v.begin(), v.end(), this->begin());
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_, Metric_<T> >::type &Metric_<T>::append(U const &p)
{
    this->push_back(typename T::value_type() = p);
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<typename U::value_type>::is_point_, Metric_<T> >::type &Metric_<T>::append(U const &l)
{
    this->push_back(l);
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<typename U::value_type>::is_container, Metric_<T> >::type &Metric_<T>::append(U const &v)
{
    size_type n = this->size();
    this->resize(v.size() + n);
    std::copy(v.begin(), v.end(), this->begin() + n);
    return *this;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_, Metric_<T> >::type &Metric_<T>::operator << (U const  &p) { return append(p); }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<typename U::value_type>::is_point_, Metric_<T> >::type &Metric_<T>::operator << (U const  &l) { return append(l); }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<typename U::value_type>::is_container, Metric_<T> >::type &Metric_<T>::operator << (U const  &v) { return append(v); }

template<typename T> inline bool Metric_<T>::isNotEmpty() const
{
    for (const_iterator it = this->begin(), itEnd = this->end(); it != itEnd; ++it)
        if (!it->empty())
            return true;
    return false;
}

template<typename T> inline void Metric_<T>::fromByteArray(QByteArray const &data)
{
    QDataStream ds(data);
    ds.setVersion(QDataStream::Qt_4_6);
    ds.setByteOrder(QDataStream::BigEndian);
    ds >> *this;
}

template<typename T> inline QByteArray Metric_<T>::toByteArray() const
{
    QByteArray resultData;
    QDataStream ds(&resultData, QIODevice::WriteOnly);
    ds.setVersion(QDataStream::Qt_4_6);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << *this;
    return resultData;
}

template<typename T> inline bool Metric_<T>::fromString(QString const &text) {
#if defined( GCC_11 )
    static_assert(__HasEl<T>::method_fromString, "Template argument T must contains toString function");
#endif
    QStringList sl = text.split(";", QString::SkipEmptyParts);
    this->clear();
    this->reserve(sl.size());
    T item;
    for (QStringList::const_iterator it = sl.begin(), itEnd = sl.end(); it != itEnd; ++it) {
        item.fromString(*it);
        this->push_back(item);
    }
    return true;
}

template<typename T> inline QString Metric_<T>::toString() const {
#if defined( GCC_11 )
    static_assert(__HasEl<T>::method_toString, "Template argument T must contains toString function");
#endif
    QStringList result;
    result.reserve(this->size());
    for (const_iterator it = this->begin(), itEnd = this->end(); it != itEnd; ++it) {
        result.push_back(it->toString());
    }
    return result.join("; ");
}

template<typename T> inline T Metric_<T>::join() const {
    T result;
    for (const_iterator it = this->begin(), itEnd = this->end(); it != itEnd; ++it) {
        result.reserve(result->size() + it->size());
        std::copy(it->begin(), it->end(), std::back_inserter(result));
    }
    return result;
}

template<typename T> inline typename Metric_<T>::const_reference Metric_<T>::atE(typename Metric_<T>::size_type n) const {
    static T _;
    if (n >= this->size() || n < 0)
        return _;
    return (*this)[n];
}

template<typename T> inline typename Metric_<T>::reference Metric_<T>::atE(typename Metric_<T>::size_type n) {
    static T _;
    if (n >= this->size() || n < 0)
        return _;
    return (*this)[n];
}

template<typename T> inline Rect_<typename LiteraDataType<T>::type> Metric_<T>::boundingRect() const
{
    typedef typename T::value_type::value_type V;
    if (this->empty())
        return Rect_<V>();
    const_iterator it    = this->begin();
    const_iterator itEnd = this->end();
    Rect_<V> res = this->begin()->boundingRect();
    for (++it; it != itEnd; ++it)
        res |= it->boundingRect();
    return res;
}

template<typename T> inline typename T::value_type const &Metric_<T>::point(size_type pos, typename T::size_type metric) const { return (*this)[metric][pos]; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_>::type Metric_<T>::setPoint(U const &p, size_type pos, typename T::size_type metric) { (*this)[metric][pos] = p; }
template<typename T> inline typename T::value_type const &Metric_<T>::pointE(size_type pos, typename T::size_type metric) const {
    static typename T::value_type _;
    if (metric < this->size() && pos < (*this)[metric].size() && metric >= 0 && pos >= 0)
        return (*this)[metric][pos]; return _; }
template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_point_>::type Metric_<T>::setPointE(U const &p, size_type pos, typename T::size_type metric) {
    if (metric < this->size() && pos < (*this)[metric].size() && metric >= 0 && pos >= 0) (*this)[metric][pos] = p; }

template<typename T> template<int M1, int P1, int M2, int P2>
inline double Metric_<T>::angle() const { typename T::value_type p = ((*this)[M2][P2] - (*this)[M1][P1]); return atan2(p.y, p.x); }
template<typename T> template<int M1, int P1, int M2, int P2>
inline void Metric_<T>::setAngle(double alpha) { const double R = 100.; (*this)[M2][P2] = (*this)[M1][P1] + typename T::value_type(R * cos(alpha), R * sin(alpha)); }
template<typename T> inline double Metric_<T>::angle() const { return angle<0,0, 1,0>(); }
template<typename T> inline void Metric_<T>::setAngle(double alpha) { setAngle<0,0, 1,0>(alpha); }

template<typename T> template<int M1, int P1, int M2, int P2>
inline double Metric_<T>::angleE() const {
    if (M1 < this->size() && P1 < (*this)[M1].size() && M2 < this->size() && P2 < (*this)[M2].size() && M1 >= 0  && P1 >= 0 && M2 >= 0 && P2 >= 0) {
        typename T::value_type p = ((*this)[M2][P2] - (*this)[M1][P1]); return atan2(p.y, p.x); } return 0; }
template<typename T> template<int M1, int P1, int M2, int P2>
inline void Metric_<T>::setAngleE(double alpha) {
    if (M1 >= this->size()) this->resize(M1 + 1); if (P1 >= (*this)[M1].size()) (*this)[M1].resize(P1 + 1);
    if (M2 >= this->size()) this->resize(M2 + 1); if (P2 >= (*this)[M2].size()) (*this)[M2].resize(P2 + 1);
    const double R = 100.; (*this)[M2][P2] = (*this)[M1][P1] + typename T::value_type(R * cos(alpha), R * sin(alpha));
}
template<typename T> inline double Metric_<T>::angleE() const { return angleE<0,0, 1,0>(); }
template<typename T> inline void Metric_<T>::setAngleE(double alpha) { setAngleE<0,0, 1,0>(alpha); }

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, U>::type Metric_<T>::array() const {
    U result;
    array(result);
    return result;
}

template<typename T> template<typename U> inline typename __Enable_If<__HasEl<U>::is_container, U>::type Metric_<T>::array(U &result) const {
#if defined( GCC_11 )
    static_assert(__HasEl<U>::is_container, "Template argument T must by container type");
#endif
    for (const_iterator it = this->begin(), itEnd = this->end(); it != itEnd; ++it) {
        it->array(result);
    }
    return result;
}

template<typename T> static inline bool operator == (Metric_<T> const &a, Metric_<T> const &b) {
    if (a.size() != b.size())
        return false;
    typename Metric_<T>::const_iterator ab = a.begin();
    typename Metric_<T>::const_iterator ae = a.end();
    typename Metric_<T>::const_iterator bb = b.begin();
    for (; ab != ae; ++ab, ++bb)
        if (*ab != *bb)
            return false;
    return true;
}

template<typename T> static inline bool operator != (Metric_<T> const &a, Metric_<T> const &b)
{ return !(a == b); }


//////////////////////////////// Helpers ////////////////////////////////


template <typename M> struct __datastream_inout_metric
{
    enum { MAGIC_BYTE = 54, DS_Version = 1 };

    template <typename MT>
    inline static void readDataFromStream(QDataStream &stream, MT &v) {
        v.clear();
        quint32 n;
        stream >> n;
        v.resize(n);
        for (typename MT::iterator it = v.begin(), itEnd = v.end(); it != itEnd; ++it)
            stream >> *it;
    }

    template <typename MT>
    inline static void writeDataToStream(QDataStream &stream, MT const &v) {
        stream << quint32(v.size());
        for (typename MT::const_iterator it = v.begin(), itEnd = v.end(); it != itEnd; ++it)
            stream << *it;
    }


    static inline void in(QDataStream &stream, M &m) {
        typedef typename M::value_type::value_type P;

        qint8 _mb, _v, _sz, _tp;
        stream >> _mb >> _v >> _sz >> _tp;
        if (MAGIC_BYTE != _mb)
            return;

        if (_v == 1) {
            if (coord::PointDataType<P>::num == CMK_NUM(_sz, _tp)) {
                readDataFromStream(stream, m);
            } else {
                #define CASE(N, T, M) case CMK_NUM(N, T): { M sm; readDataFromStream(stream, sm); m = sm; break; }

                switch (CMK_NUM(_sz, _tp)) {
                // TODO: сделать через декларацию типов данных
                CASE('2', 'c', Metric2c)
                CASE('2', 'C', Metric2C)
                CASE('2', 's', Metric2s)
                CASE('2', 'S', Metric2S)
                CASE('2', 'i', Metric2i)
                CASE('2', 'I', Metric2I)
//                CASE('2', 'l', Metric2l)
//                CASE('2', 'L', Metric2L)
                CASE('2', 'u', Metric2u)
                CASE('2', 'U', Metric2U)
                CASE('2', 'f', Metric2f)
                CASE('2', 'd', Metric2d)
#ifndef __NO_LONG_DOUBLE_MATH
//                CASE('2', 'D', Metric2D)
#endif
                CASE('3', 'c', Metric3c)
                CASE('3', 'C', Metric3C)
                CASE('3', 's', Metric3s)
                CASE('3', 'S', Metric3S)
                CASE('3', 'i', Metric3i)
                CASE('3', 'I', Metric3I)
//                CASE('3', 'l', Metric3l)
//                CASE('3', 'L', Metric3L)
                CASE('3', 'u', Metric3u)
                CASE('3', 'U', Metric3U)
                CASE('3', 'f', Metric3f)
                CASE('3', 'd', Metric3d)
#ifndef __NO_LONG_DOUBLE_MATH
//                CASE('3', 'D', Metric3D)
#endif
                default:
                    qWarning() << "Unknown metric type";
                    return;
                }
                #undef CASE
            }
            return;
        }
        qWarning() << "Unknown metric stream version";
    }
    static inline void out(QDataStream &stream, M const &m) {
        typedef typename M::value_type::value_type P;
        stream << qint8(MAGIC_BYTE) << qint8(DS_Version) << qint8(coord::PointDataType<P>::size) << qint8(coord::PointDataType<P>::type);
        writeDataToStream(stream, m);
    }
};

#undef CMK_NUM

} // namespace coord

//////////////////////////////// Stream ////////////////////////////////

template <typename T> inline QDataStream &operator << (QDataStream &stream, coord::Point_<T> const &p)
{ stream << p.x << p.y; return stream; }
template <typename T> inline QDataStream &operator >> (QDataStream &stream, coord::Point_<T> &p)
{ stream >> p.x >> p.y; return stream; }

template <typename T> inline QDataStream &operator << (QDataStream &stream, coord::Point3_<T> const &p)
{ stream << p.x << p.y << p.z; return stream; }
template <typename T> inline QDataStream &operator >> (QDataStream &stream, coord::Point3_<T> &p)
{ stream >> p.x >> p.y >> p.z; return stream; }

template <typename T> inline QDataStream &operator << (QDataStream &stream, coord::Size_<T> const &s)
{ stream << s.width << s.height; return stream; }
template <typename T> inline QDataStream &operator >> (QDataStream &stream, coord::Size_<T> &s)
{ stream >> s.width >> s.height; return stream; }

template <typename T> inline QDataStream &operator << (QDataStream &stream, coord::Rect_<T> const &r)
{ stream << r.x << r.y << r.width << r.height; return stream; }
template <typename T> inline QDataStream &operator >> (QDataStream &stream, coord::Rect_<T> &r)
{ stream >> r.x >> r.y >> r.width >> r.height; return stream; }

template <typename T> inline QDataStream &operator << (QDataStream &stream, coord::Line_<T> const &l) {
    stream << quint32(l.size());
    for (typename coord::Line_<T>::const_iterator it = l.begin(), itEnd = l.end(); it != itEnd; ++it)
        stream << *it;
    return stream;
}
template <typename T> inline QDataStream &operator >> (QDataStream &stream, coord::Line_<T> &l) {
    quint32 n;
    stream >> n;
    l.resize(n);
    for (typename coord::Line_<T>::iterator it = l.begin(), itEnd = l.end(); it != itEnd; ++it)
        stream >> *it;
    return stream;
}

template <typename T> inline QDataStream &operator<< (QDataStream &stream, coord::Metric_<T> const &data)
{ coord::__datastream_inout_metric< coord::Metric_<T> >::out(stream, data); return stream; }
template <typename T> inline QDataStream &operator>> (QDataStream &stream, coord::Metric_<T> &data)
{ coord::__datastream_inout_metric< coord::Metric_<T> >::in(stream, data); return stream; }

//////////////////////////////// Debug ////////////////////////////////

#ifndef QT_NO_DEBUG_STREAM

template <class T> inline QDebug operator<<(QDebug debug, coord::Point_<T> const &p)
{
#if defined( QT_5 )
    const bool oldSetting = debug.autoInsertSpaces();
#endif
    debug.nospace() << "Point2" << (char)coord::LiteraDataType<T>::litera << '(' << p.x << ", " << p.y << ')';
#if defined( QT_5 )
    debug.setAutoInsertSpaces(oldSetting);
#endif
//    return debug.maybeSpace();
    return debug.space();
}

template <class T> inline QDebug operator<<(QDebug debug, coord::Point3_<T> const &p)
{
#if defined( QT_5 )
    const bool oldSetting = debug.autoInsertSpaces();
#endif
    debug.nospace() << "Point3" << (char)coord::LiteraDataType<T>::litera << '(' << p.x << ", " << p.y << ", " << p.z << ')';
#if defined( QT_5 )
    debug.setAutoInsertSpaces(oldSetting);
#endif
//    return debug.maybeSpace();
    return debug.space();
}

template <class T> inline QDebug operator<<(QDebug debug, coord::Size_<T> const &s)
{
#if defined( QT_5 )
    const bool oldSetting = debug.autoInsertSpaces();
#endif
    debug.nospace() << "Size2" << (char)coord::LiteraDataType<T>::litera << '(' << s.width << ", " << s.height << ')';
#if defined( QT_5 )
    debug.setAutoInsertSpaces(oldSetting);
#endif
    return debug.maybeSpace();
}

template <class T> inline QDebug operator<<(QDebug debug, coord::Rect_<T> const &r)
{
#if defined( QT_5 )
    const bool oldSetting = debug.autoInsertSpaces();
#endif
    debug.nospace() << "Rect2" << (char)coord::LiteraDataType<T>::litera << '(' << r.x << "," << r.y << " " << r.width << "x" << r.height << ')';
#if defined( QT_5 )
    debug.setAutoInsertSpaces(oldSetting);
#endif
//    return debug.maybeSpace();
    return debug.space();
}

template <class T> inline QDebug operator<<(QDebug debug, coord::Line_<T> const &l)
{
#if defined( QT_5 )
    const bool oldSetting = debug.autoInsertSpaces();
#endif
    debug.nospace() << "Line" << (char)coord::PointDataType<T>::size << (char)coord::PointDataType<T>::type << '[';
    for (typename coord::Line_<T>::size_type i = 0; i < l.size(); ++i) {
        if (i)
            debug.nospace() << ", ";
        debug << l.at(i);
    }
    debug << ']';
#if defined( QT_5 )
    debug.setAutoInsertSpaces(oldSetting);
#endif
//    return debug.maybeSpace();
    return debug.space();
}

template <class T> inline QDebug operator<<(QDebug debug, coord::Metric_<T> const &m)
{
#if defined( QT_5 )
    const bool oldSetting = debug.autoInsertSpaces();
#endif
    debug.nospace() << "Metric" << (char)coord::PointDataType<T>::size << (char)coord::PointDataType<T>::type << '{';
    for (typename coord::Metric_<T>::size_type i = 0; i < m.size(); ++i) {
        if (i)
            debug << ", ";
        debug << m.at(i);
    }
    debug << '}';
#if defined( QT_5 )
    debug.setAutoInsertSpaces(oldSetting);
#endif
    //return debug.maybeSpace();
    return debug.space();
}

#endif // QT_NO_DEBUG_STREAM

#endif // __cplusplus

#endif // COORD_HPP
