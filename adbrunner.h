#ifndef ADBRUNNER_H
#define ADBRUNNER_H

#include <QString>
#include <QStringList>

class AdbRunner
{
public:
    AdbRunner() = delete;
    ~AdbRunner() = delete;

    static QString runCommand(const QString &cmd);
    static QString runAdb(const QString &args);
    static QStringList listDevices();
    static void openUrl(const QString &url);
    static QString formatUptime(int seconds);
    static bool isAdbAvailable();
};

#endif // ADBRUNNER_H
