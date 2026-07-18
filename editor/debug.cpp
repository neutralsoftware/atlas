/*
* debug.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Debug view for theming and more
* Copyright (c) 2026 Max Van den Eynde
*/

#include <editor/debug.h>
#include <editor/widgets/scrubbableSpinBox.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDial>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTimeEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

DebugComponentsView::DebugComponentsView(QWidget* parent)
    : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    auto* content = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(12);

    auto* title = new QLabel("Debug Components View", content);
    auto* subtitle = new QLabel(
        "A test view containing common Qt widgets for checking layout, behavior, focus, interaction, and future theme changes.",
        content
    );
    subtitle->setWordWrap(true);

    contentLayout->addWidget(title);
    contentLayout->addWidget(subtitle);

    contentLayout->addWidget(createBasicControlsSection());
    contentLayout->addWidget(createInputSection());
    contentLayout->addWidget(createSelectionSection());
    contentLayout->addWidget(createRangeSection());
    contentLayout->addWidget(createTextSection());
    contentLayout->addWidget(createItemViewsSection());
    contentLayout->addWidget(createTabsSection());
    contentLayout->addWidget(createCollapsibleSection());
    contentLayout->addWidget(createStatusSection());

    contentLayout->addStretch();

    scrollArea->setWidget(content);
    rootLayout->addWidget(scrollArea);
}

QWidget* DebugComponentsView::createSection(const QString& title, QWidget* content) {
    auto* groupBox = new QGroupBox(title, this);

    auto* layout = new QVBoxLayout(groupBox);
    layout->setContentsMargins(12, 16, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(content);

    return groupBox;
}

QWidget* DebugComponentsView::createBasicControlsSection() {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* normalButton = new QPushButton("Normal", container);
    auto* defaultButton = new QPushButton("Default", container);
    defaultButton->setDefault(true);

    auto* disabledButton = new QPushButton("Disabled", container);
    disabledButton->setEnabled(false);

    auto* checkableButton = new QPushButton("Checkable", container);
    checkableButton->setCheckable(true);
    checkableButton->setChecked(true);

    auto* toolButton = new QToolButton(container);
    toolButton->setText("Tool");

    layout->addWidget(normalButton);
    layout->addWidget(defaultButton);
    layout->addWidget(disabledButton);
    layout->addWidget(checkableButton);
    layout->addWidget(toolButton);
    layout->addStretch();

    return createSection("Basic Controls", container);
}

QWidget* DebugComponentsView::createInputSection() {
    auto* container = new QWidget(this);
    auto* layout = new QFormLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* lineEdit = new QLineEdit(container);
    lineEdit->setPlaceholderText("Type something...");

    auto* disabledLineEdit = new QLineEdit(container);
    disabledLineEdit->setText("Disabled input");
    disabledLineEdit->setEnabled(false);

    auto* comboBox = new QComboBox(container);
    comboBox->addItems({
        "Perspective",
        "Orthographic",
        "Wireframe",
        "Rendered"
    });

    auto* spinBox = new ScrubbableSpinBox(container);
    spinBox->setRange(0, 4096);
    spinBox->setValue(128);

    auto* doubleSpinBox = new ScrubbableDoubleSpinBox(container);
    doubleSpinBox->setRange(0.0, 1.0);
    doubleSpinBox->setSingleStep(0.01);
    doubleSpinBox->setValue(0.42);

    auto* dateEdit = new QDateEdit(container);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDate(QDate::currentDate());

    auto* timeEdit = new QTimeEdit(container);
    timeEdit->setTime(QTime::currentTime());

    layout->addRow("Line edit", lineEdit);
    layout->addRow("Disabled line edit", disabledLineEdit);
    layout->addRow("Combo box", comboBox);
    layout->addRow("Spin box", spinBox);
    layout->addRow("Double spin box", doubleSpinBox);
    layout->addRow("Date edit", dateEdit);
    layout->addRow("Time edit", timeEdit);

    return createSection("Inputs", container);
}

QWidget* DebugComponentsView::createSelectionSection() {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto* checkA = new QCheckBox("Visible", container);
    checkA->setChecked(true);

    auto* checkB = new QCheckBox("Locked", container);

    auto* checkC = new QCheckBox("Disabled", container);
    checkC->setEnabled(false);

    auto* radioA = new QRadioButton("Local", container);
    auto* radioB = new QRadioButton("Remote", container);
    auto* radioC = new QRadioButton("Cloud", container);

    radioA->setChecked(true);

    layout->addWidget(checkA);
    layout->addWidget(checkB);
    layout->addWidget(checkC);
    layout->addSpacing(20);
    layout->addWidget(radioA);
    layout->addWidget(radioB);
    layout->addWidget(radioC);
    layout->addStretch();

    return createSection("Selection Controls", container);
}

QWidget* DebugComponentsView::createRangeSection() {
    auto* container = new QWidget(this);
    auto* layout = new QFormLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* horizontalSlider = new QSlider(Qt::Horizontal, container);
    horizontalSlider->setRange(0, 100);
    horizontalSlider->setValue(64);

    auto* progressBar = new QProgressBar(container);
    progressBar->setRange(0, 100);
    progressBar->setValue(72);

    auto* busyProgressBar = new QProgressBar(container);
    busyProgressBar->setRange(0, 0);

    auto* dial = new QDial(container);
    dial->setRange(0, 100);
    dial->setValue(35);

    layout->addRow("Slider", horizontalSlider);
    layout->addRow("Progress bar", progressBar);
    layout->addRow("Busy progress", busyProgressBar);
    layout->addRow("Dial", dial);

    return createSection("Ranges and Progress", container);
}

QWidget* DebugComponentsView::createTextSection() {
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* plainTextEdit = new QPlainTextEdit(splitter);
    plainTextEdit->setPlainText(
        "void renderFrame(Scene& scene) {\n"
        "    renderer.beginFrame();\n"
        "    renderer.draw(scene);\n"
        "    renderer.endFrame();\n"
        "}\n"
    );

    auto* textEdit = new QTextEdit(splitter);
    textEdit->setHtml(
        "<h3>Rich Text</h3>"
        "<p>This checks rich text rendering, selection, scrolling, focus, and editor behavior.</p>"
        "<ul>"
        "<li>Scene</li>"
        "<li>Assets</li>"
        "<li>Inspector</li>"
        "</ul>"
    );

    splitter->addWidget(plainTextEdit);
    splitter->addWidget(textEdit);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    return createSection("Text Editors", splitter);
}

QWidget* DebugComponentsView::createItemViewsSection() {
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* listWidget = new QListWidget(splitter);
    listWidget->addItems({
        "main.scene",
        "player.mesh",
        "terrain.mesh",
        "skybox.material",
        "postprocess.shader"
    });

    auto* treeWidget = new QTreeWidget(splitter);
    treeWidget->setHeaderLabels({"Object", "Type"});

    auto* sceneRoot = new QTreeWidgetItem({"Scene", "Root"});
    sceneRoot->addChild(new QTreeWidgetItem({"Camera", "Entity"}));
    sceneRoot->addChild(new QTreeWidgetItem({"Directional Light", "Light"}));
    sceneRoot->addChild(new QTreeWidgetItem({"Player", "Entity"}));
    sceneRoot->addChild(new QTreeWidgetItem({"Terrain", "Mesh"}));

    treeWidget->addTopLevelItem(sceneRoot);
    treeWidget->expandAll();

    auto* tableWidget = new QTableWidget(5, 3, splitter);
    tableWidget->setHorizontalHeaderLabels({"Name", "Type", "Size"});

    tableWidget->setItem(0, 0, new QTableWidgetItem("albedo.png"));
    tableWidget->setItem(0, 1, new QTableWidgetItem("Texture"));
    tableWidget->setItem(0, 2, new QTableWidgetItem("2.4 MB"));

    tableWidget->setItem(1, 0, new QTableWidgetItem("player.mesh"));
    tableWidget->setItem(1, 1, new QTableWidgetItem("Mesh"));
    tableWidget->setItem(1, 2, new QTableWidgetItem("8.1 MB"));

    tableWidget->setItem(2, 0, new QTableWidgetItem("main.scene"));
    tableWidget->setItem(2, 1, new QTableWidgetItem("Scene"));
    tableWidget->setItem(2, 2, new QTableWidgetItem("12 KB"));

    tableWidget->setItem(3, 0, new QTableWidgetItem("pbr.shader"));
    tableWidget->setItem(3, 1, new QTableWidgetItem("Shader"));
    tableWidget->setItem(3, 2, new QTableWidgetItem("5 KB"));

    tableWidget->setItem(4, 0, new QTableWidgetItem("ambient.wav"));
    tableWidget->setItem(4, 1, new QTableWidgetItem("Audio"));
    tableWidget->setItem(4, 2, new QTableWidgetItem("3.2 MB"));

    tableWidget->horizontalHeader()->setStretchLastSection(true);

    splitter->addWidget(listWidget);
    splitter->addWidget(treeWidget);
    splitter->addWidget(tableWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 2);

    return createSection("Item Views", splitter);
}

QWidget* DebugComponentsView::createTabsSection() {
    auto* tabs = new QTabWidget(this);

    auto* viewportLabel = new QLabel("Viewport Preview", tabs);
    viewportLabel->setAlignment(Qt::AlignCenter);

    auto* inspectorLabel = new QLabel("Inspector", tabs);
    inspectorLabel->setAlignment(Qt::AlignCenter);

    auto* console = new QPlainTextEdit(tabs);
    console->setPlainText(
        "[Info] Atlas Engine started\n"
        "[Info] Loaded debug components view\n"
        "[Warning] This is a test warning\n"
        "[Error] This is a test error\n"
    );

    tabs->addTab(viewportLabel, "Viewport");
    tabs->addTab(inspectorLabel, "Inspector");
    tabs->addTab(console, "Console");

    return createSection("Tabs", tabs);
}

QWidget* DebugComponentsView::createCollapsibleSection() {
    auto* section = new QWidget(this);
    section->setObjectName("collapsibleSection");

    auto* layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* header = new QToolButton(section);
    header->setObjectName("collapsibleHeader");
    header->setText("Transform");
    header->setCheckable(true);
    header->setChecked(true);
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    header->setArrowType(Qt::DownArrow);

    auto* body = new QWidget(section);
    body->setObjectName("collapsibleBody");

    auto* form = new QFormLayout(body);
    form->setContentsMargins(18, 4, 0, 0);
    form->setSpacing(8);

    auto* positionX = new ScrubbableDoubleSpinBox(body);
    positionX->setRange(-10000.0, 10000.0);
    positionX->setValue(12.5);

    auto* positionY = new ScrubbableDoubleSpinBox(body);
    positionY->setRange(-10000.0, 10000.0);
    positionY->setValue(4.0);

    auto* positionZ = new ScrubbableDoubleSpinBox(body);
    positionZ->setRange(-10000.0, 10000.0);
    positionZ->setValue(-2.25);

    auto* visible = new QCheckBox("Visible in scene", body);
    visible->setChecked(true);

    form->addRow("Position X", positionX);
    form->addRow("Position Y", positionY);
    form->addRow("Position Z", positionZ);
    form->addRow("Visibility", visible);

    QObject::connect(header, &QToolButton::toggled, section, [header, body, section](bool checked) {
        header->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
        body->setVisible(checked);
        body->updateGeometry();
        section->updateGeometry();
        if (auto* parent = section->parentWidget()) {
            parent->updateGeometry();
        }
    });

    layout->addWidget(header);
    layout->addWidget(body);

    return section;
}

QWidget* DebugComponentsView::createStatusSection() {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    layout->addWidget(new QLabel("Ready", container));
    layout->addStretch();
    layout->addWidget(new QLabel("60 FPS", container));
    layout->addWidget(new QLabel("Renderer: Metal", container));
    layout->addWidget(new QLabel("Memory: 421 MB", container));

    return createSection("Status Row", container);
}
