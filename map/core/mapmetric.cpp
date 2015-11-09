
#include "coord/mapcoords.h"
#include "core/mapmetric.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

Metric::Metric(MetricType mt)
    : QObject(), d_ptr(new MetricShareData)
{
    d_ptr->baseType = mt;
}

Metric::Metric(MetricData m, MetricType mt)
    : QObject(), d_ptr(new MetricShareData)
{
    d_ptr->baseType = mt;
    d_ptr->metric[mt] = m;
}

Metric::Metric(MetricItem m, MetricType mt)
    : QObject(), d_ptr(new MetricShareData)
{
    d_ptr->baseType = mt;
    d_ptr->metric[mt] = MetricData() << m;
}

Metric::Metric(QPointF m, MetricType mt)
    : QObject(), d_ptr(new MetricShareData)
{
    d_ptr->baseType = mt;
    d_ptr->metric[mt] = MetricData() << (MetricItem() << m);
}

Metric::~Metric()
{
}

void Metric::clear()
{
    d_ptr->metric.clear();
}

void Metric::setMetric(MetricData m, MetricType t)
{
    d_ptr->metric.clear();
    d_ptr->baseType = t;
    d_ptr->metric[d_ptr->baseType] = m;
}

void Metric::setMetric(MetricItem m, MetricType t)
{
    setMetric(MetricData() << m, t);
}

void Metric::setMetric(MetricItem m, int ind, MetricType t)
{
    MetricData data = d_ptr->metric[d_ptr->baseType];
    d_ptr->metric.clear();
    if (ind >= data.size())
        data.resize(ind + 1);
    data[ind] = convertToType(m, findConvertFunction(t, d_ptr->baseType));
    d_ptr->metric[d_ptr->baseType] = data;
}

void Metric::setMetric(Metric m, int ind)
{
    MetricData tmp = m.d_ptr->metric.value(m.d_ptr->baseType);
    if (tmp.isEmpty())
        return;

    setMetric(tmp.first(), ind, m.d_ptr->baseType);
}

void Metric::setMetric(QPointF m, MetricType t)
{
    setMetric(MetricItem() << m, t);
}

void Metric::setMetric(QPointF m, int ind, MetricType t)
{
    setMetric(MetricItem() << m, ind, t);
}

MetricData Metric::toType(MetricType t) const
{
    if (!d_ptr->metric.contains(t)) {
        MetricData data;
        d_ptr->metric[t] = data = convertToType(d_ptr->metric.value(d_ptr->baseType), findConvertFunction(d_ptr->baseType, t));
        return data;
    }
    return d_ptr->metric.value(t);
}

MetricItem Metric::toType(int ind, MetricType t) const
{
    return toType(t).at(ind);
}

MetricItem Metric::toTypeE(int ind, MetricType t) const
{
    if (ind < 0 || d_ptr->metric.value(d_ptr->baseType).size() <= ind)
        return MetricItem();
    return toType(t).at(ind);
}

QPointF Metric::toTypeE(int ind, int pos, MetricType t) const
{
    if (ind < 0 || d_ptr->metric.value(d_ptr->baseType).size() <= ind)
        return QPointF();
    MetricItem const &m = toType(t).at(ind);
    if (pos < 0 || pos >= m.size())
        return QPointF();
    return m.at(pos);
}

MetricType Metric::type() const
{
    return d_ptr->baseType;
}

int Metric::size() const
{
    return d_ptr->metric[d_ptr->baseType].size();
}

bool Metric::operator==(Metric const &other) const
{
    return d_ptr->baseType == other.d_ptr->baseType && d_ptr->metric[d_ptr->baseType] == other.d_ptr->metric[d_ptr->baseType];
}

bool Metric::operator!=(Metric const &other) const
{
    return !(*this == other);
}

Metric::PointToPoint Metric::findConvertFunction(MetricType from, MetricType to) const
{
    switch (from) {
    case Mercator: {
        switch (to) {
        case SK42_flat:
            return &CoordTranform::worldToSK42; break;
        case WGS84_geo:
            return &CoordTranform::worldToGeo; break;
        default:
            break;
        }
        break;
    }
    case SK42_flat: {
        switch (to) {
        case Mercator:
            return &CoordTranform::sk42ToWorld; break;
        case WGS84_geo:
            return &CoordTranform::sk42ToGeo; break;
        default:
            break;
        }
        break;
    }
    case WGS84_geo: {
        switch (to) {
        case SK42_flat:
            return &CoordTranform::geoToSK42; break;
        case Mercator:
            return &CoordTranform::geoToWorld; break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return &noopTransform;
}

MetricItem Metric::convertToType(const MetricItem &m, Metric::PointToPoint f) const
{
    MetricItem result;
    foreach (QPointF const point, m)
        result.append(f(point));
    return result;
}

MetricData Metric::convertToType(const MetricData &m, Metric::PointToPoint f) const
{
    MetricData result;
    foreach (MetricItem const &p, m)
        result.append(convertToType(p, f));
    return result;
}

// -------------------------------------------------------

#ifndef QT_NO_DEBUG_STREAM

void Metric::debug(QDebug dbg) const
{
    dbg.nospace() << "Metric#";
    switch (d_ptr->baseType) {
        case minigis::Mercator:  dbg.nospace() << "Mercator"; break;
        case minigis::WGS84_geo: dbg.nospace() << "WGS84_geo"; break;
        case minigis::SK42_flat: dbg.nospace() << "SK42_flat"; break;
    };
    dbg.nospace() << "{";
    MetricData const &md = d_ptr->metric[d_ptr->baseType];
    for (QVectorIterator<MetricItem> it(md); it.hasNext(); ) {
        dbg.nospace() << "[";
        for (QVectorIterator<QPointF> pIt(it.next()); pIt.hasNext(); ) {
            QPointF const &p = pIt.next();
            dbg.nospace() << "(" << p.x() << ", " << p.y() << ")";
        }
        dbg.nospace() << "]";
    }
    dbg.nospace() << "}";
}

#endif // QT_NO_DEBUG_STREAM

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------
