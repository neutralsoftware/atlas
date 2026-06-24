/*
* inspectorView.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Inspector View declaration
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_INSPECTORVIEW_H
#define ATLAS_INSPECTORVIEW_H

#include <QWidget>

class QFormLayout;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;

class InspectorPanel : public QWidget {
    Q_OBJECT

public:
    explicit InspectorPanel(QWidget* parent = nullptr);

    void inspectTemporaryObject(const QString& name);

private:
    QFormLayout* form = nullptr;
    QLineEdit* nameField = nullptr;
};

#endif //ATLAS_INSPECTORVIEW_H
