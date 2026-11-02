#include "adbrunner.h"
#include <QProcess>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QCoreApplication>

QString AdbRunner::runCommand(const QString &cmd)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
#if defined(Q_OS_WIN)
    process.start(QStringLiteral("cmd.exe"), {QStringLiteral("/C"), cmd});
#else
    process.start(QStringLiteral("bash"), {QStringLiteral("-c"), cmd});
#endif
    if (!process.waitForStarted(5000)) {
        return QString();
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(1000);
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

QString AdbRunner::runAdb(const QString &args)
{
    QString adbPath = QStandardPaths::findExecutable(QStringLiteral("adb"));
    if (adbPath.isEmpty()) {
        adbPath = QStringLiteral("adb");
    }

    if (args.contains(QLatin1Char('|')) || args.contains(QLatin1Char('>')) ||
        args.contains(QLatin1Char('<')) || args.contains(QLatin1Char('&'))) {
        return runCommand(QStringLiteral("\"%1\" %2").arg(adbPath, args));
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const QStringList argList = QProcess::splitArguments(args);
    process.start(adbPath, argList);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    process.startCommand(QStringLiteral("\"%1\" %2").arg(adbPath, args));
#else
    process.start(adbPath, args.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts));
#endif

    if (!process.waitForStarted(5000)) {
        return QString();
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(1000);
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

QStringList AdbRunner::listDevices()
{
    QStringList serials;
    const QString output = runAdb(QStringLiteral("devices"));
    const QStringList lines = output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("List of devices attached"), Qt::CaseInsensitive)) {
            continue;
        }

        if (trimmed.endsWith(QStringLiteral("device"), Qt::CaseInsensitive) ||
            trimmed.endsWith(QStringLiteral("offline"), Qt::CaseInsensitive)) {
            const QStringList tokens = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (!tokens.isEmpty()) {
                serials.append(tokens.first());
            }
        }
    }
    return serials;
}

void AdbRunner::openUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

QString AdbRunner::formatUptime(int seconds)
{
    if (seconds < 0) {
        seconds = 0;
    }
    const int days = seconds / 86400;
    const int remAfterDays = seconds % 86400;
    const int hours = remAfterDays / 3600;
    const int minutes = (remAfterDays % 3600) / 60;

    return QCoreApplication::translate("AdbRunner", "%1d %2h %3m")
        .arg(days)
        .arg(hours)
        .arg(minutes);
}

bool AdbRunner::isAdbAvailable()
{
    if (QStandardPaths::findExecutable(QStringLiteral("adb")).isEmpty()) {
        return false;
    }
    return QProcess::execute(QStringLiteral("adb"), {QStringLiteral("version")}) == 0;
}
