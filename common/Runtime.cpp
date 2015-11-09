#include <QDebug>
#include <QDir>
#ifdef ANDROID
#include <QAndroidJniObject>
#else
#include <QCoreApplication>
#endif
#include "Runtime.h"

QString getAppDataPath()
{
    QString dataPath;
    if (dataPath.isEmpty())
    {
        QString path;
#ifdef ANDROID
        path = "assets:";
#else
        path = QCoreApplication::applicationDirPath();
#endif
        dataPath = path + "/";
    }
    return dataPath;
}

QString Runtime::getDataPath()
{
    static QString dataPath = getAppDataPath();
    return dataPath;
}

QString Runtime::getEtcPath()
{
    static QString etcPath = getDataPath() + "etc/";
    return etcPath;
}

QString Runtime::getSharePath()
{
    static QString sharePath = getDataPath() + "share/";
    return sharePath;
}

QString Runtime::getCachePath()
{
    static QString cachePath =
#ifdef ANDROID
            // имя пакета хардкодим, пока не найдем как это сделать программно
            QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;").callObjectMethod("getAbsolutePath", "()Ljava/lang/String;").toString() + "/Android/data/org.gkpromtech.monitoring/"
#else
            getDataPath()
#endif
            + "cache/";
    return cachePath;
}

void Runtime::mkPath(QString path)
{
    QDir dir;
    if (dir.mkpath(path))
        qDebug() << "Created Path: " << path;
    else
        qDebug() << "Not Create Path: " << path;
}
