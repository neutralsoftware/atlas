#include <editor/views/splashScreen.h>

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QLabel>
#include <QPixmap>
#include <QScreen>
#include <QTimer>

#ifndef ATLAS_VERSION
#define ATLAS_VERSION "Alpha 9"
#endif

#ifndef ATLAS_BUILD_STRING
#define ATLAS_BUILD_STRING ""
#endif

SplashScreen::SplashScreen(QWidget *parent)
    : QDialog(parent, Qt::SplashScreen | Qt::FramelessWindowHint) {
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setObjectName("atlasSplash");
    setFixedSize(708, 218);

    auto *card = new QFrame(this);
    card->setObjectName("splashCard");
    card->setGeometry(0, 0, 708, 218);
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20.0);
    shadow->setOffset(0.0, 5.0);
    shadow->setColor(QColor(0, 0, 0, 64));
    card->setGraphicsEffect(shadow);

    auto *icon = new QLabel(card);
    icon->setObjectName("splashIcon");
    icon->setGeometry(28, 32, 110, 110);
#ifdef ATLAS_DEBUG_BUILD
    icon->setPixmap(QPixmap(":/editor/assets/Icon-iOS-Default-1024x1024@1x.png")
                        .scaled(icon->size(), Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
#else
    icon->setPixmap(
        QPixmap(":/editor/assets/iconFile-iOS-Dark-1024x1024@1x.png")
            .scaled(icon->size(), Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
#endif

    auto *title = new QLabel(card);
    title->setObjectName("splashTitle");
    title->setGeometry(162, 42, 530, 44);

    QFont titleFont = title->font();
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
    titleFont.setKerning(true);
#ifdef ATLAS_DEBUG_BUILD
    title->setText("Atlas Engine (Development)");
    title->setFont(titleFont);
#else
    title->setText("Atlas Engine");
#endif
    title->setWordWrap(false);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *version = new QLabel(card);
    version->setObjectName("splashVersion");
    version->setGeometry(162, 85, 500, 32);
#ifdef ATLAS_DEBUG_BUILD
    version->setText(QStringLiteral("%1 (build %2)")
                         .arg(QStringLiteral(ATLAS_VERSION),
                              QStringLiteral(ATLAS_BUILD_STRING)));
#else
    version->setText(QStringLiteral(ATLAS_VERSION));
#endif
    version->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *company = new QLabel("by neutral software", card);
    company->setObjectName("splashCompany");
    company->setGeometry(162, 116, 400, 23);
    company->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    statusLabel = new QLabel("Loading the engine…", card);
    statusLabel->setObjectName("splashStatus");
    statusLabel->setGeometry(0, 136, 708, 22);
    statusLabel->setAlignment(Qt::AlignCenter);

#ifdef ATLAS_DEBUG_BUILD
    auto *warning = new QLabel(
        "As this software is in its development version issues may be found "
        "with the experience. If you meant to use the traditional version "
        "please access: https://atlasengine.org to get the official builds. "
        "In development versions, the engine may require you to have a "
        "runtime already installed therefore, make sure that you have an "
        "appropriate runtime in your system that works with this version.",
        card);
    warning->setObjectName("splashWarning");
    warning->setGeometry(40, 161, 628, 48);
    warning->setWordWrap(true);
    warning->setAlignment(Qt::AlignLeft | Qt::AlignTop);
#endif

    setStyleSheet(R"(
#atlasSplash {
    background: transparent;
}
#splashCard {
    background: #FFFFFF;
    border: 1px solid rgba(20, 24, 28, 18);
    border-radius: 40px;
}
#splashTitle {
    background: transparent;
    color: #0B0D0E;
    font-size: 40px;
    font-weight: 700;
}
#splashVersion {
    background: transparent;
    color: #A1A5A8;
    font-size: 18px;
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
    font-size: 9px;
    font-weight: 450;
}
#splashWarning {
    background: transparent;
    color: #A4A8AB;
    font-size: 9px;
    font-weight: 450;
}
)");

    finishTimer = new QTimer(this);
    finishTimer->setSingleShot(true);
    connect(finishTimer, &QTimer::timeout, this, [this] {
        hide();
        emit ready();
    });
}

void SplashScreen::start(const QString &statusText, int durationMs) {
    QString displayStatus = statusText;
    displayStatus.replace(QChar(0x2026), "...");
    statusLabel->setText(displayStatus);
    const QRect available =
        QGuiApplication::primaryScreen()->availableGeometry();
    move(available.left() + (available.width() - width()) / 2,
         available.top() + (available.height() - height()) / 2);
    show();
    raise();
    finishTimer->start(durationMs < 300 ? 300 : durationMs);
}
