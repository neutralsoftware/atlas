#include <editor/views/splashScreen.h>

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QScreen>

#ifndef ATLAS_VERSION
#define ATLAS_VERSION "No version"
#endif

#ifndef ATLAS_BUILD_STRING
#define ATLAS_BUILD_STRING ""
#endif

SplashScreen::SplashScreen(QWidget *parent)
    : QDialog(parent, Qt::SplashScreen | Qt::FramelessWindowHint) {
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setObjectName("atlasSplash");
    setFixedSize(708, 252);

    auto *card = new QFrame(this);
    card->setObjectName("splashCard");
    card->setGeometry(0, 0, 708, 252);
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20.0);
    shadow->setOffset(0.0, 5.0);
    shadow->setColor(QColor(0, 0, 0, 64));
    card->setGraphicsEffect(shadow);

    auto *icon = new QLabel(card);
    icon->setObjectName("splashIcon");
    icon->setGeometry(30, 38, 108, 108);
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
    title->setGeometry(164, 42, 514, 44);

    QFont titleFont = title->font();
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
    titleFont.setKerning(true);
#ifdef ATLAS_RELEASE_BUILD
    title->setText("Atlas Engine");
#else
    title->setText("Atlas Engine (Development)");
    title->setFont(titleFont);
#endif
    title->setWordWrap(false);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *version = new QLabel(card);
    version->setObjectName("splashVersion");
    version->setGeometry(164, 86, 500, 28);
#ifdef ATLAS_RELEASE_BUILD
    version->setText(QStringLiteral(ATLAS_VERSION));
#else
    version->setText(QStringLiteral("%1 (build %2)")
                         .arg(QStringLiteral(ATLAS_VERSION),
                              QStringLiteral(ATLAS_BUILD_STRING)));
#endif
    version->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *company = new QLabel("by neutral software", card);
    company->setObjectName("splashCompany");
    company->setGeometry(164, 114, 400, 22);
    company->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    statusLabel = new QLabel("Loading the engine…", card);
    statusLabel->setObjectName("splashStatus");
    statusLabel->setGeometry(164, 151, 500, 22);
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *progress = new QProgressBar(card);
    progress->setObjectName("splashProgress");
    progress->setGeometry(164, 181, 500, 5);
    progress->setRange(0, 0);
    progress->setTextVisible(false);

#ifndef ATLAS_RELEASE_BUILD
    auto *warning = new QLabel(
        "As this software is in its development version issues may be found "
        "with the experience. If you meant to use the traditional version "
        "please access: https://atlasengine.org to get the official builds. "
        "In development versions, the engine may require you to have a "
        "runtime already installed therefore, make sure that you have an "
        "appropriate runtime in your system that works with this version.",
        card);
    warning->setObjectName("splashWarning");
    warning->setGeometry(40, 199, 628, 44);
    warning->setWordWrap(true);
    warning->setAlignment(Qt::AlignLeft | Qt::AlignTop);
#endif

    setStyleSheet(R"(
#atlasSplash {
    background: transparent;
}
#splashCard {
    background: #202224;
    border: 1px solid #494D52;
    border-radius: 18px;
}
#splashTitle {
    background: transparent;
    color: #F5F7FA;
    font-size: 34px;
    font-weight: 750;
}
#splashVersion {
    background: transparent;
    color: #8498A8;
    font-size: 14px;
    font-weight: 700;
}
#splashCompany {
    background: transparent;
    color: #7F8B9D;
    font-family: "Manrope";
    font-size: 12px;
    font-weight: 650;
}
#splashStatus {
    background: transparent;
    color: #B7C1CF;
    font-size: 10px;
    font-weight: 550;
}
#splashProgress {
    background: #34373A;
    border: none;
    border-radius: 2px;
}
#splashProgress::chunk {
    background: #71889A;
    border-radius: 2px;
}
#splashWarning {
    background: transparent;
    color: #68758A;
    font-size: 9px;
    font-weight: 450;
}
)");
}

void SplashScreen::start(const QString &statusText) {
    setStatus(statusText);
    const QRect available =
        QGuiApplication::primaryScreen()->availableGeometry();
    move(available.left() + (available.width() - width()) / 2,
         available.top() + (available.height() - height()) / 2);
    show();
    raise();
}

void SplashScreen::setStatus(const QString &statusText) {
    QString displayStatus = statusText;
    displayStatus.replace(QChar(0x2026), "...");
    statusLabel->setText(displayStatus);
    statusLabel->repaint();
}

void SplashScreen::finish() {
    hide();
    emit ready();
}
