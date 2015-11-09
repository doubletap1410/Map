
#include <QString>

#include "mapdefs.h"

// -------------------------------------------------------

namespace minigis {

// -------------------------------------------------------

QDataStream &operator<<(QDataStream &out, HelperState const &state)
{
    char const *txt = NULL;
    switch (state) {
    case stMove   : txt = "stMove"; break;
    case stRotate : txt = "stRotate"; break;
    case stScale  : txt = "stScale"; break;
    case stSelect : txt = "stSelect"; break;
    case stCreate : txt = "stCreate"; break;
    case stEdit   : txt = "stEdit"; break;
    case stNoop   : txt = "stNoop"; break;
    default: txt = "stUnknown"; break;
    }
    out << txt;
    return out;
}

// -------------------------------------------------------

QDataStream &operator<<(QDataStream &out, States const &state)
{
    out << "minimap::States(";
    if (state.testFlag(stMove))
        out << "stMove";
    if (state.testFlag(stRotate))
        out << "stRotate";
    if (state.testFlag(stScale))
        out << "stScale";
    if (state.testFlag(stSelect))
        out << "stSelect";
    if (state.testFlag(stCreate))
        out << "stCreate";
    if (state.testFlag(stEdit))
        out << "stEdit";
    out << ")";
    return out;
}

// -------------------------------------------------------

QString attributeHumanName(int attr)
{
    switch (attr) {

    case attrOpacity           : return QString::fromUtf8("Непрозрачность");

    case attrPenColor          : return QString::fromUtf8("Цвет линии");
    case attrPenWidth          : return QString::fromUtf8("Толщина линии");
    case attrPenStyle          : return QString::fromUtf8("Стиль линии");
    case attrPenCapStyle       : return QString::fromUtf8("Концы линий");
    case attrPenJoinStyle      : return QString::fromUtf8("Соединения линий");

    case attrBrushColor        : return QString::fromUtf8("Цвет заливки");
    case attrBrushStyle        : return QString::fromUtf8("Стиль заливки");

    case attrFontFamily        : return QString::fromUtf8("Семейство шрифта");
    case attrFontColor         : return QString::fromUtf8("Цвет шрифта");
    case attrFontPixelSize     : return QString::fromUtf8("Размер в пикселах шрифта");
    case attrFontStretch       : return QString::fromUtf8("Вытянутость шрифта");
    case attrFontStyle         : return QString::fromUtf8("Стиль шрифта");
    case attrFontWeight        : return QString::fromUtf8("Ширина шрифта");
    case attrFontBold          : return QString::fromUtf8("Жирность шрифта");
    case attrFontItalic        : return QString::fromUtf8("Наклон шрифта");
    case attrFontUnderline     : return QString::fromUtf8("Подчёркнутость шрифта");
    case attrFontStrikeOut     : return QString::fromUtf8("Зачёркнутость шрифта");

    case attrRoundedRectXRadius: return QString::fromUtf8("Радиус скругления прямоугольника по X");
    case attrRoundedRectYRadius: return QString::fromUtf8("Радиус скругления прямоугольника по Y");

    case attrSmoothed          : return QString::fromUtf8("Сглаженный объект");

    case attrStartArrow        : return QString::fromUtf8("Начало");
    case attrEndArrow          : return QString::fromUtf8("Конец");

    case attrDescriptionObject : return QString::fromUtf8("Описание");
    case attrGenMinObject      : return QString::fromUtf8("Нижняя граница генерализации");
    case attrGenMaxObject      : return QString::fromUtf8("Верхняя граница генерализации");

    case attrRotatableObject   : return QString::fromUtf8("Вращаемый объект");
    case attrMirroredObject    : return QString::fromUtf8("Зеркальный объект");

    case attrSize              : return QString::fromUtf8("Размер точечного объекта");

    case attrHeightList        : return QString::fromUtf8("Высоты точек");
    default:
        return QString::fromUtf8("АТРИБУТ[%1]").arg(static_cast<int>(attr));
    }
}

// -------------------------------------------------------

QStringList attributeHumanName(minigis::Attributes const &attr)
{
    QStringList res;
    foreach (int a, attr)
        res.append(attributeHumanName(a));
    return res;
}

// -------------------------------------------------------

QStringList attributeHumanName(QHash<int, QVariant> const &values, QString const &delimiter, Attributes const &sort)
{
    QStringList res;
    QHash<int, QVariant> val(values);
    foreach (int a, sort) {
        QVariant aVal = val.take(a);
        res.append(QString("%1%2%3").arg(attributeHumanName(a)).arg(delimiter).arg(aVal.toString()));
    }
    for (QHashIterator<int, QVariant> it(val); it.hasNext(); ) {
        it.next();
        QVariant aVal = it.value();
        res.append(QString("%1%2%3").arg(attributeHumanName(it.key())).arg(delimiter).arg(aVal.toString()));
    }
    return res;
}

// -------------------------------------------------------

QString attributeHumanName(QHash<int, QVariant> const &values, QString const &delimiter, QString const &spacer, Attributes const &sort)
{
    return attributeHumanName(values, delimiter, sort).join(spacer);
}

// -------------------------------------------------------

} // namespace minigis

// -------------------------------------------------------

