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

    const QColor bg = QColor("#171716");
    const QColor panel = QColor("#242423");
    const QColor panel2 = QColor("#20201F");
    const QColor text = QColor("#E8E7E3");
    const QColor mutedText = QColor("#A3A29B");
    const QColor accent = QColor("#39363F");

    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, panel2);
    p.setColor(QPalette::AlternateBase, panel);
    p.setColor(QPalette::ToolTipBase, panel2);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, QColor("#292927"));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    p.setColor(QPalette::Light, QColor("#505357"));
    p.setColor(QPalette::Midlight, QColor("#424549"));
    p.setColor(QPalette::Mid, QColor("#33332F"));
    p.setColor(QPalette::Dark, QColor("#171716"));
    p.setColor(QPalette::Shadow, QColor("#0D0E0F"));
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, mutedText);

    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#74746D"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#74746D"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#74746D"));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor("#242423"));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor("#20201F"));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#3A4146"));

    app.setPalette(p);
}

void styling::applyTheme(QApplication& app) {
    applyColorPalette(app);
    app.setStyleSheet(QString::fromUtf8(DARK_THEME));
}
