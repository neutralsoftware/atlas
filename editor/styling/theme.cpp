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

    const QColor bg = QColor("#222526");
    const QColor panel = QColor("#282C2E");
    const QColor panel2 = QColor("#1A1D1E");
    const QColor text = QColor("#DCE2E5");
    const QColor mutedText = QColor("#8F9AA0");
    const QColor accent = QColor("#3D8BFF");

    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, panel2);
    p.setColor(QPalette::AlternateBase, panel);
    p.setColor(QPalette::ToolTipBase, panel2);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, QColor("#303538"));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    p.setColor(QPalette::Light, QColor("#4A555B"));
    p.setColor(QPalette::Midlight, QColor("#3A4246"));
    p.setColor(QPalette::Mid, QColor("#303638"));
    p.setColor(QPalette::Dark, QColor("#151819"));
    p.setColor(QPalette::Shadow, QColor("#0D0F10"));
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, mutedText);

    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#727B80"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#727B80"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#727B80"));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor("#292D2F"));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor("#25292B"));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#34404A"));

    app.setPalette(p);
}

void styling::applyTheme(QApplication& app) {
    applyColorPalette(app);
    app.setStyleSheet(QString::fromUtf8(DARK_THEME));
}
