#ifndef MAPOBJECTMETRIC_H
#define MAPOBJECTMETRIC_H

#include <QObject>
#include <QPolygonF>
#include <QMap>
#include <QVariant>
#include <QSharedData>

#include <math.h>

#include <common/coord.hpp>

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

enum MetricType {
    Mercator  = 10,
    WGS84_geo = 20,
    SK42_flat = 30
};
typedef QPolygonF MetricItem;
typedef QVector<MetricItem> MetricData;
class MetricShareData;

// -------------------------------------------------------
// Данные метрики
class MetricShareData : public QSharedData
{
public:
    MetricShareData() {}
    MetricShareData(const MetricShareData &other) : QSharedData(other) {
        metric   = other.metric;
        baseType = other.baseType;
    }
    ~MetricShareData() {}

    mutable QHash<MetricType, MetricData > metric;
    MetricType baseType;
};

// TODO: перевести на использование coord::Metric_ с добавлением типа СК

// Класс метрика графического объекта
class Metric : public QObject
{
    Q_OBJECT
    Q_ENUMS(MetricType)

public:
    Metric(MetricType mt = WGS84_geo);
    Metric(MetricData m, MetricType mt = SK42_flat);
    Metric(MetricItem m, MetricType mt = SK42_flat);
    Metric(QPointF m, MetricType mt = SK42_flat);
    template<typename P> Metric(coord::Metric_<P> m, MetricType t = SK42_flat);
    virtual ~Metric();

    Metric(const Metric &other) : QObject(), d_ptr(other.d_ptr) {}
    Metric &operator=(Metric const &other) {
        if (&other != this)
            d_ptr = other.d_ptr;
        return *this;
    }

    // очистить метрику
    Q_INVOKABLE void clear();

    // установить новую метрику
    Q_INVOKABLE void setMetric(MetricData m, MetricType t = SK42_flat);
    Q_INVOKABLE void setMetric(MetricItem m, MetricType t = SK42_flat);
    Q_INVOKABLE void setMetric(MetricItem m, int ind, MetricType t = SK42_flat);
    template<typename P> void setMetric(coord::Metric_<P> m, MetricType t = SK42_flat);

    // изменить часть метрики
    Q_INVOKABLE void setMetric(Metric m, int ind);
    Q_INVOKABLE void setMetric(QPointF m, MetricType t = SK42_flat);
    Q_INVOKABLE void setMetric(QPointF m, int ind, MetricType t = SK42_flat);

    // вернуть метрику в нужной системе координат
    Q_INVOKABLE MetricData toType(MetricType t) const;
    Q_INVOKABLE inline MetricData toMercator() const { return toType(Mercator);  }
    Q_INVOKABLE inline MetricData toWGS84Geo() const { return toType(WGS84_geo); }
    Q_INVOKABLE inline MetricData toSK42Flat() const { return toType(SK42_flat); }
    Q_INVOKABLE inline MetricData toBaseType() const { return d_ptr->metric.value(d_ptr->baseType); }
    template<typename P> inline P toMetric(MetricType t = SK42_flat) const;

    // вернуть часть метрики в нужной системе координат
    Q_INVOKABLE MetricItem toType(int ind, MetricType t) const;
    Q_INVOKABLE inline MetricItem toMercator(int ind) const { return toType(ind, Mercator);  }
    Q_INVOKABLE inline MetricItem toWGS84Geo(int ind) const { return toType(ind, WGS84_geo); }
    Q_INVOKABLE inline MetricItem toSK42Flat(int ind) const { return toType(ind, SK42_flat); }
    Q_INVOKABLE inline MetricItem toBaseType(int ind) const { return d_ptr->metric.value(d_ptr->baseType).at(ind);  }

    // вернуть часть метрики в нужной системе координат. Не выдаёт ошибки при выходе за границы
    Q_INVOKABLE MetricItem toTypeE(int ind, MetricType t) const;
    Q_INVOKABLE inline MetricItem toMercatorE(int ind) const { return toTypeE(ind, Mercator);  }
    Q_INVOKABLE inline MetricItem toWGS84GeoE(int ind) const { return toTypeE(ind, WGS84_geo); }
    Q_INVOKABLE inline MetricItem toSK42FlatE(int ind) const { return toTypeE(ind, SK42_flat); }
    Q_INVOKABLE inline MetricItem toBaseTypeE(int ind) const {
        MetricData const &md = toType(d_ptr->baseType); if (ind < 0 || md.size() <= ind)
        return MetricItem(); return md.at(ind);
    }

    Q_INVOKABLE QPointF toTypeE(int ind, int pos, MetricType t) const;
    Q_INVOKABLE inline QPointF toMercatorE(int ind, int pos) const { return toTypeE(ind, pos, Mercator);  }
    Q_INVOKABLE inline QPointF toWGS84GeoE(int ind, int pos) const { return toTypeE(ind, pos, WGS84_geo); }
    Q_INVOKABLE inline QPointF toSK42FlatE(int ind, int pos) const { return toTypeE(ind, pos, SK42_flat); }
    Q_INVOKABLE inline QPointF toBaseTypeE(int ind, int pos) const {
        MetricData const &md = toType(d_ptr->baseType);
        if (ind < 0 || md.size() <= ind) return QPointF();
        MetricItem const &mi = md.at(ind);
        if (pos < 0 || pos >= mi.size()) return QPointF();
        return mi.at(pos);
    }

    Q_INVOKABLE MetricType type() const;
    Q_INVOKABLE int size() const;

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

    // из-за округления чисел с плавающей точкой после преобразования координат сравнениваются только однотипные метрики
    Q_INVOKABLE bool operator==(Metric const &other) const;
    Q_INVOKABLE bool operator!=(Metric const &other) const;

#ifndef QT_NO_DEBUG_STREAM
    void debug(QDebug) const;
#endif

private:

    typedef QPointF(*PointToPoint)(const QPointF&);
    static QPointF noopTransform(const QPointF &p) { return p; }

    PointToPoint findConvertFunction(MetricType from, MetricType to) const;
    MetricItem convertToType(const MetricItem &m, PointToPoint f) const;
    MetricData convertToType(const MetricData &m, PointToPoint f) const;

    QSharedDataPointer<MetricShareData> d_ptr;
};

// -------------------------------------------------------

template<typename P> Metric::Metric(coord::Metric_<P> m, MetricType t)
    : QObject(), d_ptr(new MetricShareData)
{ setMetric(m, t); }

// -------------------------------------------------------

template<typename P> void Metric::setMetric(coord::Metric_<P> m, MetricType t) {
    typedef coord::Metric_<P> MMT;
    d_ptr->metric.clear();
    d_ptr->baseType = t;
    MetricData &md = d_ptr->metric[d_ptr->baseType];

    md.resize(m.size());
    for (typename MMT::size_type i = 0; i < m.size(); ++i) {
        MetricItem &mi = md[i];
        typename MMT::value_type &mp3 = m[i];
        mi.resize(mp3.size());
        std::copy(mp3.begin(), mp3.end(), mi.begin());
    }
}
// -------------------------------------------------------

template<typename P> inline P Metric::toMetric(MetricType t) const {
    P res;
    minigis::MetricData md = toType(t);
    res.clear();
    res.resize(md.size());
    for (int i = 0; i < md.size(); ++i) {
        typename P::value_type &sm = res[i];
        const minigis::MetricItem &mi = md[i];
        sm.resize(mi.size());
        std::copy(mi.begin(), mi.end(), sm.begin());
    }
    return res;
}

// -------------------------------------------------------

template<int M1, int P1, int M2, int P2>
inline double Metric::angle() const
{
    MetricData &md = d_ptr->metric[d_ptr->baseType];
    QPointF p = (md[M2][P2] - md[M1][P1]);
    return atan2(p.y(), p.x());
}
template<int M1, int P1, int M2, int P2>
inline void Metric::setAngle(double alpha)
{
    MetricData &md = d_ptr->metric[d_ptr->baseType];
    const double R = 100.;
    md[M2][P2] = md[M1][P1] + QPointF(R * cos(alpha), R * sin(alpha));
}
inline double Metric::angle() const
{
    return angle<0,0, 1,0>();
}
inline void Metric::setAngle(double alpha)
{
    setAngle<0,0, 1,0>(alpha);
}

template<int M1, int P1, int M2, int P2>
inline double Metric::angleE() const
{
    MetricData &md = d_ptr->metric[d_ptr->baseType];
    if (M1 < this->size() && P1 < md[M1].size() && M2 < md.size() && P2 < md[M2].size() && M1 >= 0  && P1 >= 0 && M2 >= 0 && P2 >= 0) {
        QPointF p = (md[M2][P2] - md[M1][P1]);
        return atan2(p.y(), p.x());
    }
    return 0;
}
template<int M1, int P1, int M2, int P2>
inline void Metric::setAngleE(double alpha)
{
    MetricData &md = d_ptr->metric[d_ptr->baseType];
    if (M1 >= md.size())
        md.resize(M1 + 1);
    if (P1 >= md[M1].size())
        md[M1].resize(P1 + 1);
    if (M2 >= md.size())
        md.resize(M2 + 1);
    if (P2 >= md[M2].size())
        md[M2].resize(P2 + 1);
    const double R = 100.;
    md[M2][P2] = md[M1][P1] + QPointF(R * cos(alpha), R * sin(alpha));
}
inline double Metric::angleE() const
{
    return angleE<0,0, 1,0>();
}
inline void Metric::setAngleE(double alpha)
{
    setAngleE<0,0, 1,0>(alpha);
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug dbg, minigis::Metric const &m)
{
    m.debug(dbg);
    return dbg;
}
#endif // QT_NO_DEBUG_STREAM

// -------------------------------------------------------

Q_DECLARE_METATYPE(minigis::Metric)

#endif // MAPOBJECTMETRIC_H
