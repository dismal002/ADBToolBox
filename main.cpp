#include <QApplication>
#include <QtPlugin>

#ifdef QT_STATIC
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QSvgPlugin)
Q_IMPORT_PLUGIN(QSvgIconPlugin)
#endif
#include <QTranslator>
#include <QLocale>

#include "mainwindow.h"
#include "appsettings.h"
#include "theme/materialtheme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("AndroidToolbox"));
    app.setApplicationDisplayName(QObject::tr("Android ADB Toolbox"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("AndroidToolbox"));

    // Load AppSettings
    AppSettings *settings = AppSettings::instance();

    // Apply Material theme based on themeNum
    int themeNum = settings->themeNum();
    if (themeNum < 0 || themeNum > 5) themeNum = 0;
    MaterialTheme::applyTheme(static_cast<MaterialTheme::ThemeType>(themeNum));

    // Create and show MainWindow
    MainWindow mainWindow;
    if (settings->windowOpacity() > 0.05 && settings->windowOpacity() <= 1.0) {
        mainWindow.setWindowOpacity(settings->windowOpacity());
    }
    mainWindow.show();

    return app.exec();
}