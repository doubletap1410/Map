#ifndef MAPTEMPLATES_H
#define MAPTEMPLATES_H

#include <QDebug>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

// check if T can convert to U
template<class T, class U>
class Conversion {
    typedef char Small;
    class Big { char dummy[2]; };
    static Small Test(const U&);
    static Big Test(...);
    static T MakeT();
public:
    enum { check = sizeof(Test(MakeT())) == sizeof(Small) };
};

// Check if CLS is SubClass of Base
template<class BASE, class CLS>
struct SubClass {
   enum { check = Conversion<const CLS*, const BASE*>::check};
};

// -------------------------------------------------------

// primary template: yield second or third argument depending on first argument
template <bool C, typename Ta, typename Tb>
struct IfThenElse;

// partial specialization: true yields second argument
template <typename Ta, typename Tb>
struct IfThenElse<true, Ta, Tb> {
    typedef Ta ResultT;
};

// partial specialization: false yields third argument
template <typename Ta, typename Tb>
struct IfThenElse<false, Ta, Tb> {
    typedef Tb ResultT;
};

// -------------------------------------------------------

// primary template
template <typename T>
class TypeOp { // primary template
public:
    typedef T         Arg;
    typedef T         Bare;
    typedef T const   Const;
    typedef T &       Ref;
    typedef T &       RefBare;
    typedef T const & RefConst;
};

template <typename T>
class TypeOp <T const> { // partial specialization for const types
public:
    typedef T const   Arg;
    typedef T         Bare;
    typedef T const   Const;
    typedef T const & Ref;
    typedef T &       RefBar;
    typedef T const & RefConst;
};

template <typename T>
class TypeOp <T &> { // partial specialization for references
public:
    typedef T &                        Arg;
    typedef typename TypeOp<T>::Bare   Bare;
    typedef T const                    Const;
    typedef T &                        Ref;
    typedef typename TypeOp<T>::Bare & RefBare;
    typedef T const &                  RefConst;
};

template<>
class TypeOp <void> { // full specialization for void
public:
    typedef void       Arg;
    typedef void       Bare;
    typedef void const Const;
    typedef void       Ref;
    typedef void       RefBare;
    typedef void       RefConst;
};


// -------------------------------------------------------


// primary template: in general T is no fundamental type
template <typename T>
class IsFunda {
public:
    enum { Yes = 0, No = 1};
};

// macro to specialize for fundamental types
#define MK_FUNDA_TYPE(T)           \
    template<> class IsFunda<T> { \
    public:                        \
    enum { Yes = 1, No = 0 };      \
    };

MK_FUNDA_TYPE(void)

MK_FUNDA_TYPE(bool)
MK_FUNDA_TYPE(char)
MK_FUNDA_TYPE(signed char)
MK_FUNDA_TYPE(unsigned char)
MK_FUNDA_TYPE(wchar_t)

MK_FUNDA_TYPE(signed short)
MK_FUNDA_TYPE(unsigned short)
MK_FUNDA_TYPE(signed int)
MK_FUNDA_TYPE(unsigned int)
MK_FUNDA_TYPE(signed long)
MK_FUNDA_TYPE(unsigned long)
#if LONGLONG_EXISTS
MK_FUNDA_TYPE(signed long long)
MK_FUNDA_TYPE(unsigned long long)
#endif  // LONGLONG_EXISTS

MK_FUNDA_TYPE(float)
MK_FUNDA_TYPE(double)
MK_FUNDA_TYPE(long double)

#undef MK_FUNDA_TYPE

template<typename T>
class IsFunction {
private:
    typedef char One;
    typedef struct { char a[2]; } Two;
    template<typename U> static One test(...);
    template<typename U> static Two test(U (*)[1]);
public:
    enum { Yes = sizeof(IsFunction<T>::test<T>(0)) == sizeof(One) };
    enum { No = !Yes };
};

template<typename T>
class IsFunction<T&> {
public:
    enum { Yes = 0 };
    enum { No = !Yes };
};

template<>
class IsFunction<void> {
public:
    enum { Yes = 0 };
    enum { No = !Yes };
};

template<>
class IsFunction<void const> {
public:
    enum { Yes = 0 };
    enum { No = !Yes };
};

// same for void volatile and void const volatile
//...

template<typename T>
class Compound {           // primary template
public:
    enum {
        IsPtr    = 0,
        IsRef    = 0,
        IsArray  = 0,
        IsFunc   = IsFunction<T>::Yes,
        IsPtrMem = 0
    };
    typedef T BaseT;
    typedef T BottomT;
    typedef Compound<void> ClassT;
};

template<typename T>
class Compound<T&> {       // partial specialization for references
public:
    enum {
        IsPtr    = 0,
        IsRef    = 1,
        IsArray  = 0,
        IsFunc   = 0,
        IsPtrMem = 0
    };
    typedef T BaseT;
    typedef typename Compound<T>::BottomT BottomT;
    typedef Compound<void> ClassT;
};

template<typename T>
class Compound<T*> {       // partial specialization for pointers
public:
    enum {
        IsPtr    = 1,
        IsRef    = 0,
        IsArray  = 0,
        IsFunc   = 0,
        IsPtrMem = 0
    };
    typedef T BaseT;
    typedef typename Compound<T>::BottomT BottomT;
    typedef Compound<void> ClassT;
};

template<typename T, size_t N>
class Compound <T[N]> {    // partial specialization for arrays
public:
    enum {
        IsPtr    = 0,
        IsRef    = 0,
        IsArray  = 1,
        IsFunc   = 0,
        IsPtrMem = 0
    };
    typedef T BaseT;
    typedef typename Compound<T>::BottomT BottomT;
    typedef Compound<void> ClassT;
};

template<typename T>
class Compound <T[]> {    // partial specialization for empty arrays
public:
    enum {
        IsPtr    = 0,
        IsRef    = 0,
        IsArray  = 1,
        IsFunc   = 0,
        IsPtrMem = 0
    };
    typedef T BaseT;
    typedef typename Compound<T>::BottomT BottomT;
    typedef Compound<void> ClassT;
};

template<typename T, typename C>
class Compound <T C::*> {  // partial specialization for pointer-to-members
public:
    enum {
        IsPtr    = 0,
        IsRef    = 0,
        IsArray  = 0,
        IsFunc   = 0,
        IsPtrMem = 1
    };
    typedef T BaseT;
    typedef typename Compound<T>::BottomT BottomT;
    typedef C ClassT;
};

template<typename R>
class Compound<R()> {
    public:
    enum {
        IsPtr = 0,
        IsRef = 0,
        IsArray = 0,
        IsFunc = 1,
        IsPtrMem = 0
    };
    typedef R BaseT();
    typedef R BottomT();
    typedef Compound<void> ClassT;
};

template<typename R, typename P1>
class Compound<R(P1)> {
    public:
    enum {
        IsPtr    = 0,
        IsRef    = 0,
        IsArray  = 0,
        IsFunc   = 1,
        IsPtrMem = 0
    };
    typedef R BaseT(P1);
    typedef R BottomT(P1);
    typedef Compound<void> ClassT;
};

template<typename R, typename P1>
class Compound<R(P1, ...)> {
    public:
    enum {
        IsPtr    = 0,
        IsRef    = 0,
        IsArray  = 0,
        IsFunc   = 1,
        IsPtrMem = 0
    };
    typedef R BaseT(P1);
    typedef R BottomT(P1);
    typedef Compound<void> ClassT;
};

struct SizeOverOne { char c[2]; };

template<typename T,
         bool convert_possible = !Compound<T>::IsFunc && !Compound<T>::IsArray>
class ConsumeUDC {
public:
    operator T() const;
};

// conversion to function and array types is not possible
template <typename T>
class ConsumeUDC<T, false> {
};

// conversion to void type is not possible
template <bool convert_possible>
class ConsumeUDC<void, convert_possible> {
};

char enum_check(bool);
char enum_check(char);
char enum_check(signed char);
char enum_check(unsigned char);
char enum_check(wchar_t);

char enum_check(signed short);
char enum_check(unsigned short);
char enum_check(signed int);
char enum_check(unsigned int);
char enum_check(signed long);
char enum_check(unsigned long);
#if LONGLONG_EXISTS
char enum_check(signed long long);
char enum_check(unsigned long long);
#endif  // LONGLONG_EXISTS

// avoid accidental conversions from float to int
char enum_check(float);
char enum_check(double);
char enum_check(long double);

SizeOverOne enum_check(...);    // catch all

template<typename T>
class IsEnum {
public:
    enum {
        Yes = IsFunda<T>::No &&
        !Compound<T>::IsRef &&
        !Compound<T>::IsPtr &&
        !Compound<T>::IsPtrMem &&
        sizeof(enum_check(ConsumeUDC<T>())) == 1
    };
    enum { No = !Yes };
};

template<typename T>
class IsClass {
public:
    enum {
        Yes = IsFunda<T>::No &&
        IsEnum<T>::No &&
        !Compound<T>::IsPtr &&
        !Compound<T>::IsRef &&
        !Compound<T>::IsArray &&
        !Compound<T>::IsPtrMem &&
        !Compound<T>::IsFunc
    };
    enum { No = !Yes };
};

// define template that handles all in one style
template <typename T>
class TypeT {
public:
    enum {
        IsFunda  = IsFunda<T>::Yes,
        IsPtr    = Compound<T>::IsPtr,
        IsRef    = Compound<T>::IsRef,
        IsArray  = Compound<T>::IsArray,
        IsFunc   = Compound<T>::IsFunc,
        IsPtrMem = Compound<T>::IsPtrMem,
        IsEnum   = IsEnum<T>::Yes,
        IsClass  = IsClass<T>::Yes
    };
};

// check by passing type as template argument
template <typename T>
void check()
{
    if (TypeT<T>::IsFunda) {
        qDebug() << " IsFunda ";
    }
    if (TypeT<T>::IsPtr) {
        qDebug() << " IsPtr ";
    }
    if (TypeT<T>::IsRef) {
        qDebug() << " IsRef ";
    }
    if (TypeT<T>::IsArray) {
        qDebug() << " IsArray ";
    }
    if (TypeT<T>::IsFunc) {
        qDebug() << " IsFunc ";
    }
    if (TypeT<T>::IsPtrMem) {
        qDebug() << " IsPtrMem ";
    }
    if (TypeT<T>::IsEnum) {
        qDebug() << " IsEnum ";
    }
    if (TypeT<T>::IsClass) {
        qDebug() << " IsClass ";
    }
    qDebug();
}

// check by passing type as function call argument
template <typename T>
void checkT (T)
{
    check<T>();

    // for pointer types check type of what they refer to
    if (TypeT<T>::IsPtr || TypeT<T>::IsPtrMem) {
        check<typename Compound<T>::BaseT>();
    }
}


// -------------------------------------------------------

//! класс компаратор двух объектов
template <typename Result, typename C, template <typename T> class BinaryPred>
struct const_comparator_t : public std::binary_function<C const&, C const&, bool>
{
    typedef Result (C::*Fnctype)() const;

public:
    explicit const_comparator_t(Fnctype _pfn) : pfnc(_pfn) {}
    bool operator()(C const& left, C const& right) { return pred((left.*pfnc)(), (right.*pfnc)()); }
    bool operator()(C *const& left, C *const& right) { return pred((left->*pfnc)(), (right->*pfnc)()); }
    template <typename Share>
    bool operator()(Share const& left, Share const& right) { return pred(((*left).*pfnc)(), ((*right).*pfnc)()); }

private:
    Fnctype pfnc;
    BinaryPred<Result> pred;
};

//! класс компаратор двух объектов
template <typename Result, typename C, template <typename T> class BinaryPred>
struct const_comparator_c : public std::binary_function<C const&, C const&, bool>
{
    typedef Result (C::*Fnctype)();

public:
    explicit const_comparator_c(Fnctype _pfn) : pfnc(_pfn) {}
    bool operator()(C const& left, C const& right) { return pred((left.*pfnc)(), (right.*pfnc)()); }
    bool operator()(C *const& left, C *const& right) { return pred((left->*pfnc)(), (right->*pfnc)()); }
    template <typename Share>
    bool operator()(Share const& left, Share const& right) { return pred(((*left).*pfnc)(), ((*right).*pfnc)()); }

private:
    Fnctype pfnc;
    BinaryPred<Result> pred;
};

// -------------------------------------------------------

//! вспомогательная функция компаратора для вывода аргументов шаблона
template <template <typename T> class BinaryPred, typename C, typename Result>
// inline
const_comparator_t<Result, C, BinaryPred> compare(Result (C::*pfn)() const)
{
    return const_comparator_t<Result, C, BinaryPred>(pfn);
}

//! вспомогательная функция компаратора для вывода аргументов шаблона
template <template <typename T> class BinaryPred, typename C, typename Result>
// inline
const_comparator_t<Result, C, BinaryPred> compare(Result (C::*pfn)())
{
    return const_comparator_t<Result, C, BinaryPred>(pfn);
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#endif // MAPTEMPLATES_H
