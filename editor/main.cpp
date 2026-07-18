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
#include <QTimer>

#include "DockManager.h"
#include "DockWidget.h"
#include "../include/editor/application/styling.h"
#include "editor/debug.h"
#include "editor/styling/icons.h"
#include "editor/views/editorWindow.h"
#include "editor/views/projectBrowser.h"
#include "editor/views/splashScreen.h"

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
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

    const QStringList manropeFamilies =
        QFontDatabase::applicationFontFamilies(manropeFont);
    QFont applicationFont = manropeFamilies.isEmpty()
                                ? QFontDatabase::systemFont(
                                      QFontDatabase::GeneralFont)
                                : QFont(manropeFamilies.first());
    applicationFont.setPointSizeF(11.0);
    app.setFont(applicationFont);
    styling::loadIconFont();

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

    auto *startupSplash = new SplashScreen();
    startupSplash->start("Preparing the project browser...");
    QTimer::singleShot(0, &app, [&app, startupSplash] {
        auto *projectBrowser = new ProjectBrowser();
        QObject::connect(
            projectBrowser, &ProjectBrowser::openProjectRequested, &app,
            [projectBrowser](const QString &projectFile) {
                projectBrowser->setEnabled(false);
                projectBrowser->hide();
                auto *splash = new SplashScreen();
                splash->start("Restoring editor workspace...");
                QTimer::singleShot(
                    0, splash, [projectBrowser, projectFile, splash] {
                        auto *editor = new EditorWindow(projectFile);
                        editor->setAttribute(Qt::WA_DeleteOnClose);
                        QObject::connect(
                            editor, &EditorWindow::startupStatusChanged,
                            splash, &SplashScreen::setStatus);
                        QObject::connect(
                            editor, &EditorWindow::startupReady, splash,
                            [projectBrowser, editor, splash](bool,
                                                             const QString &) {
                                splash->finish();
                                splash->deleteLater();
                                projectBrowser->deleteLater();
                                editor->raise();
                                editor->activateWindow();
                            });
                        editor->show();
                    });
            });
        projectBrowser->show();
        projectBrowser->raise();
        projectBrowser->activateWindow();
        startupSplash->finish();
        startupSplash->deleteLater();
    });
    return app.exec();
}
