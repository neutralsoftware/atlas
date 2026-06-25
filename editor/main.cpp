/*
* main.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Main entry point for the editor
* Copyright (c) 2026 Max Van den Eynde
*/

#include <QApplication>
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

int main(int argc, char** argv) {
    QApplication app(argc, argv);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif


    app.setStyle("Fusion");
    styling::applyTheme(app);

    EditorWindow window;
    window.show();
    return app.exec();
}
