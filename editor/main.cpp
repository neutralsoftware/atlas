/*
 * main.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Main entry point for the editor
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStyle>
#include <QStyleHints>

#include "DockManager.h"
#include "DockWidget.h"
#include "../include/editor/application/styling.h"
#include "editor/debug.h"
#include "editor/views/editorWindow.h"
#include "editor/views/projectBrowser.h"
#include "editor/views/splashScreen.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Atlas Engine");
    app.setApplicationDisplayName("Atlas Engine");
    app.setOrganizationName("Neutral Software");
    app.setQuitOnLastWindowClosed(true);

    const int manropeFont = QFontDatabase::addApplicationFont(
        ":/editor/assets/Manrope-VariableFont_wght.ttf");
    if (manropeFont < 0) {
        qWarning() << "Failed to load Manrope";
    }

    const QFont systemFont =
        QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    app.setFont(systemFont);

#ifdef ATLAS_DEBUG_BUILD
    app.setWindowIcon(
        QIcon(":/editor/assets/Icon-iOS-Default-1024x1024@1x.png"));
#else
    app.setWindowIcon(
        QIcon(":/editor/assets/iconFile-iOS-Dark-1024x1024@1x.png"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

    app.setStyle("Fusion");
    styling::applyTheme(app);

    auto *projectBrowser = new ProjectBrowser();
    QObject::connect(projectBrowser, &ProjectBrowser::openProjectRequested,
                     &app, [projectBrowser](const QString &projectFile) {
                         projectBrowser->setEnabled(false);
                         auto *splash = new SplashScreen();
                         QObject::connect(
                             splash, &SplashScreen::ready, splash,
                             [projectBrowser, projectFile, splash] {
                                 auto *editor = new EditorWindow(projectFile);
                                 editor->setAttribute(Qt::WA_DeleteOnClose);
                                 editor->show();
                                 projectBrowser->deleteLater();
                                 splash->deleteLater();
                             });
                         splash->start("Opening your project…", 1000);
                         projectBrowser->hide();
                     });

    auto *startupSplash = new SplashScreen();
    QObject::connect(startupSplash, &SplashScreen::ready, startupSplash,
                     [projectBrowser, startupSplash] {
                         projectBrowser->show();
                         projectBrowser->raise();
                         projectBrowser->activateWindow();
                         startupSplash->deleteLater();
                     });
    startupSplash->start("Loading the engine…", 1200);
    return app.exec();
}
