#include <editor/views/splashScreen.h>

#include <QApplication>
#include <QColor>
#include <QEventLoop>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

#ifndef ATLAS_VERSION
#define ATLAS_VERSION "Alpha 9"
#endif

#ifndef ATLAS_BUILD_STRING
#define ATLAS_BUILD_STRING ""
#endif

SplashScreen::SplashScreen(QWidget* parent)
    : QDialog(parent, Qt::SplashScreen | Qt::FramelessWindowHint) {
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setObjectName("atlasSplash");

#ifdef ATLAS_DEBUG_BUILD
    resize(980, 360);
#else
    resize(900, 300);
#endif

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(28, 28, 28, 28);

    auto* card = new QFrame(this);
    card->setObjectName("splashCard");
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(32.0);
    shadow->setOffset(0.0, 12.0);
    shadow->setColor(QColor(0, 0, 0, 72));
    card->setGraphicsEffect(shadow);
    outerLayout->addWidget(card);

    auto* rootLayout = new QVBoxLayout(card);
    rootLayout->setContentsMargins(42, 34, 42, 30);
    rootLayout->setSpacing(18);

    auto* heroLayout = new QHBoxLayout();
    heroLayout->setSpacing(30);
    auto* icon = new QLabel(card);
    icon->setObjectName("splashIcon");
    icon->setFixedSize(150, 150);
#ifdef ATLAS_DEBUG_BUILD
    icon->setPixmap(QPixmap(":/editor/assets/Icon-iOS-Default-1024x1024@1x.png")
                        .scaled(icon->size(), Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
#else
    icon->setPixmap(QPixmap(":/editor/assets/iconFile-iOS-Dark-1024x1024@1x.png")
                        .scaled(icon->size(), Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
#endif
    heroLayout->addWidget(icon, 0, Qt::AlignTop);

    auto* copyLayout = new QVBoxLayout();
    copyLayout->setSpacing(8);
    auto* title = new QLabel(card);
    title->setObjectName("splashTitle");
#ifdef ATLAS_DEBUG_BUILD
    title->setText("Atlas Engine (Development)");
#else
    title->setText("Atlas Engine");
#endif
    title->setWordWrap(true);
    copyLayout->addWidget(title);

    auto* version = new QLabel(card);
    version->setObjectName("splashVersion");
#ifdef ATLAS_DEBUG_BUILD
    version->setText(QStringLiteral("%1 (build %2)")
                         .arg(QStringLiteral(ATLAS_VERSION),
                              QStringLiteral(ATLAS_BUILD_STRING)));
#else
    version->setText(QStringLiteral(ATLAS_VERSION));
#endif
    copyLayout->addWidget(version);

    auto* company = new QLabel("by neutral software", card);
    company->setObjectName("splashCompany");
    copyLayout->addWidget(company);
    copyLayout->addStretch();

    statusLabel = new QLabel("Loading the engine…", card);
    statusLabel->setObjectName("splashStatus");
    statusLabel->setAlignment(Qt::AlignCenter);
    copyLayout->addWidget(statusLabel);
    heroLayout->addLayout(copyLayout, 1);
    rootLayout->addLayout(heroLayout);

#ifdef ATLAS_DEBUG_BUILD
    auto* warning = new QLabel(
        "This is a development build of Atlas Engine. Features may change or "
        "behave unexpectedly. For official builds, visit atlasengine.org.",
        card);
    warning->setObjectName("splashWarning");
    warning->setWordWrap(true);
    rootLayout->addWidget(warning);
#endif

    auto* progress = new QProgressBar(card);
    progress->setObjectName("splashProgress");
    progress->setRange(0, 0);
    progress->setTextVisible(false);
    progress->setFixedHeight(3);
    rootLayout->addWidget(progress);

    setStyleSheet(R"(
#atlasSplash {
    background: transparent;
}
#splashCard {
    background: #FFFFFF;
    border: 1px solid rgba(20, 24, 28, 18);
    border-radius: 34px;
}
#splashTitle {
    background: transparent;
    color: #0B0D0E;
    font-family: "Manrope";
    font-size: 42px;
    font-weight: 700;
}
#splashVersion {
    background: transparent;
    color: #A1A5A8;
    font-family: "Manrope";
    font-size: 23px;
    font-weight: 700;
}
#splashCompany {
    background: transparent;
    color: #111416;
    font-family: "Manrope";
    font-size: 17px;
    font-weight: 650;
}
#splashStatus, #splashWarning {
    background: transparent;
    color: #9A9EA1;
    font-family: "Manrope";
    font-size: 14px;
    font-weight: 450;
}
#splashProgress {
    background: #ECEFF1;
    border: none;
    border-radius: 1px;
}
#splashProgress::chunk {
    background: #4B9EFF;
    border-radius: 1px;
}
)");

    finishTimer = new QTimer(this);
    finishTimer->setSingleShot(true);
    connect(finishTimer, &QTimer::timeout, this, [this] {
        hide();
        emit ready();
    });
}

void SplashScreen::start(const QString& statusText, int durationMs) {
    statusLabel->setText(statusText);
    const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize targetSize(std::min(width(), available.width() - 40),
                           std::min(height(), available.height() - 40));
    resize(targetSize);
    move(available.center() - rect().center());
    show();
    raise();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    finishTimer->start(std::max(300, durationMs));
}
