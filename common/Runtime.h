#ifndef TMS_RUNTIME_H
#define TMS_RUNTIME_H

class QString;

class Runtime
{
public:
    static QString getDataPath();
    static QString getEtcPath();
    static QString getSharePath();
    static QString getCachePath();

    static void mkPath(QString);
};

#endif // TMS_RUNTIME_H
