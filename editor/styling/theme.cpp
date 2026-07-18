/*
* theme.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Theme definition for the Atlas Editor
* Copyright (c) 2026 Max Van den Eynde
*/

#include <qpalette.h>
#include <QApplication>

#include "editor/application/styling.h"
#include "editor/core/themes.h"

void styling::applyColorPalette(QApplication& app) {
    QPalette p;

    const QColor bg = QColor("#0D1117");
    const QColor panel = QColor("#151B24");
    const QColor panel2 = QColor("#10151C");
    const QColor text = QColor("#E7ECF3");
    const QColor mutedText = QColor("#8490A4");
    const QColor accent = QColor("#7C5CFC");

    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, panel2);
    p.setColor(QPalette::AlternateBase, panel);
    p.setColor(QPalette::ToolTipBase, panel2);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, QColor("#1A2230"));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    p.setColor(QPalette::Light, QColor("#364154"));
    p.setColor(QPalette::Midlight, QColor("#2A3444"));
    p.setColor(QPalette::Mid, QColor("#222B39"));
    p.setColor(QPalette::Dark, QColor("#090C11"));
    p.setColor(QPalette::Shadow, QColor("#05070A"));
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, mutedText);

    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#566174"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#566174"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#566174"));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor("#131922"));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor("#11161D"));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#332A5E"));

    app.setPalette(p);
}

void styling::applyTheme(QApplication& app) {
    applyColorPalette(app);
    app.setStyleSheet(QString::fromUtf8(DARK_THEME));
}
