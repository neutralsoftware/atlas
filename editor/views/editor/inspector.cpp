/*
* inspector.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Inspector definition and functions
* Copyright (c) 2026 Max Van den Eynde
*/

#include <editor/views/inspectorView.h>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>

static QDoubleSpinBox* makeNumberBox(QWidget* parent) {
    auto* box = new QDoubleSpinBox(parent);
    box->setRange(-100000.0, 100000.0);
    box->setDecimals(3);
    return box;
}

InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);

    auto* objectGroup = new QGroupBox("Object", this);
    auto* objectLayout = new QFormLayout(objectGroup);

    nameField = new QLineEdit(objectGroup);
    objectLayout->addRow("Name", nameField);

    auto* active = new QCheckBox(objectGroup);
    active->setChecked(true);
    objectLayout->addRow("Active", active);

    auto* transformGroup = new QGroupBox("Transform", this);
    auto* transformLayout = new QFormLayout(transformGroup);

    transformLayout->addRow("Position X", makeNumberBox(transformGroup));
    transformLayout->addRow("Position Y", makeNumberBox(transformGroup));
    transformLayout->addRow("Position Z", makeNumberBox(transformGroup));

    transformLayout->addRow("Rotation X", makeNumberBox(transformGroup));
    transformLayout->addRow("Rotation Y", makeNumberBox(transformGroup));
    transformLayout->addRow("Rotation Z", makeNumberBox(transformGroup));

    transformLayout->addRow("Scale X", makeNumberBox(transformGroup));
    transformLayout->addRow("Scale Y", makeNumberBox(transformGroup));
    transformLayout->addRow("Scale Z", makeNumberBox(transformGroup));

    rootLayout->addWidget(objectGroup);
    rootLayout->addWidget(transformGroup);
    rootLayout->addStretch();

    inspectTemporaryObject("Nothing Selected");
}

void InspectorPanel::inspectTemporaryObject(const QString& name) {
    nameField->setText(name);
}
