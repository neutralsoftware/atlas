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

    const QColor bg = QColor("#10141D");
    const QColor panel = QColor("#202938");
    const QColor panel2 = QColor("#1B2230");
    const QColor text = QColor("#E6EDF7");
    const QColor mutedText = QColor("#A5B2C7");
    const QColor accent = QColor("#354E70");

    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, panel2);
    p.setColor(QPalette::AlternateBase, panel);
    p.setColor(QPalette::ToolTipBase, panel2);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, QColor("#252F40"));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    p.setColor(QPalette::Light, QColor("#4F6077"));
    p.setColor(QPalette::Midlight, QColor("#425166"));
    p.setColor(QPalette::Mid, QColor("#334054"));
    p.setColor(QPalette::Dark, QColor("#10141D"));
    p.setColor(QPalette::Shadow, QColor("#0E121A"));
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, mutedText);

    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#748198"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#748198"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#748198"));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor("#202938"));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor("#1B2230"));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#354E70"));

    app.setPalette(p);
}

void styling::applyTheme(QApplication& app) {
    applyColorPalette(app);
    app.setStyleSheet(QString::fromUtf8(DARK_THEME));
}
