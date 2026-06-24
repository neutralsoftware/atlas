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

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Atlas Engine");
    window.resize(1280, 720);

    auto* label = new QLabel("Hello, World!");
    label->setAlignment(Qt::AlignCenter);

    window.setCentralWidget(label);
    window.show();

    return app.exec();
}
