#include <editor/views/splashScreen.h>

#include <QColor>
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
    setFixedSize(708, 218);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(8, 8, 8, 8);

    auto* card = new QFrame(this);
    card->setObjectName("splashCard");
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20.0);
    shadow->setOffset(0.0, 5.0);
    shadow->setColor(QColor(0, 0, 0, 64));
    card->setGraphicsEffect(shadow);
    outerLayout->addWidget(card);

    auto* rootLayout = new QHBoxLayout(card);
    rootLayout->setContentsMargins(22, 18, 22, 16);
    rootLayout->setSpacing(20);

    auto* icon = new QLabel(card);
    icon->setObjectName("splashIcon");
    icon->setFixedSize(116, 116);
#ifdef ATLAS_DEBUG_BUILD
    icon->setPixmap(QPixmap(":/editor/assets/Icon-iOS-Default-1024x1024@1x.png")
                        .scaled(icon->size(), Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
#else
    icon->setPixmap(QPixmap(":/editor/assets/iconFile-iOS-Dark-1024x1024@1x.png")
                        .scaled(icon->size(), Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
#endif
    rootLayout->addWidget(icon, 0, Qt::AlignVCenter);

    auto* copyLayout = new QVBoxLayout();
    copyLayout->setSpacing(2);
    auto* title = new QLabel(card);
    title->setObjectName("splashTitle");
#ifdef ATLAS_DEBUG_BUILD
    title->setText("Atlas Engine (Development)");
#else
    title->setText("Atlas Engine");
#endif
    title->setWordWrap(false);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
    version->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    copyLayout->addWidget(version);

    auto* company = new QLabel("by neutral software", card);
    company->setObjectName("splashCompany");
    company->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    copyLayout->addWidget(company);
    copyLayout->addStretch();

    statusLabel = new QLabel("Loading the engine…", card);
    statusLabel->setObjectName("splashStatus");
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    copyLayout->addWidget(statusLabel);

#ifdef ATLAS_DEBUG_BUILD
    auto* warning = new QLabel(
        "Development build — features may change. Official builds: "
        "atlasengine.org",
        card);
    warning->setObjectName("splashWarning");
    warning->setWordWrap(false);
    warning->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    copyLayout->addWidget(warning);
#endif

    auto* progress = new QProgressBar(card);
    progress->setObjectName("splashProgress");
    progress->setRange(0, 0);
    progress->setTextVisible(false);
    progress->setFixedHeight(2);
    copyLayout->addWidget(progress);
    rootLayout->addLayout(copyLayout, 1);

    setStyleSheet(R"(
#atlasSplash {
    background: transparent;
}
#splashCard {
    background: #FFFFFF;
    border: 1px solid rgba(20, 24, 28, 18);
    border-radius: 26px;
}
#splashTitle {
    background: transparent;
    color: #0B0D0E;
    font-family: "Manrope";
    font-size: 25px;
    font-weight: 700;
}
#splashVersion {
    background: transparent;
    color: #A1A5A8;
    font-family: "Manrope";
    font-size: 15px;
    font-weight: 700;
}
#splashCompany {
    background: transparent;
    color: #111416;
    font-family: "Manrope";
    font-size: 12px;
    font-weight: 650;
}
#splashStatus {
    background: transparent;
    color: #9A9EA1;
    font-family: "Manrope";
    font-size: 11px;
    font-weight: 450;
}
#splashWarning {
    background: transparent;
    color: #A4A8AB;
    font-family: "Manrope";
    font-size: 9px;
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
    move(available.left() + (available.width() - width()) / 2,
         available.top() + (available.height() - height()) / 2);
    show();
    raise();
    finishTimer->start(durationMs < 300 ? 300 : durationMs);
}
