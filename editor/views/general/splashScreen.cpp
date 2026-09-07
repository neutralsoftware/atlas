#include <editor/views/splashScreen.h>
#include <editor/styling/icons.h>

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
    setFixedSize(640, 280);

    auto *card = new QFrame(this);
    card->setObjectName("splashCard");
    card->setGeometry(0, 0, 640, 280);
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20.0);
    shadow->setOffset(0.0, 5.0);
    shadow->setColor(QColor(0, 0, 0, 64));
    card->setGraphicsEffect(shadow);

    auto *icon = new QLabel(card);
    icon->setObjectName("splashIcon");
    icon->setGeometry(30, 44, 104, 104);
    icon->setPixmap(styling::brandMark(icon->size()));

    auto *title = new QLabel(card);
    title->setObjectName("splashTitle");
    title->setGeometry(158, 44, 444, 48);

    QFont titleFont = title->font();
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
    titleFont.setKerning(true);
    title->setText("Atlas Engine");
    title->setFont(titleFont);
    title->setWordWrap(false);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *version = new QLabel(card);
    version->setObjectName("splashVersion");
    version->setGeometry(160, 100, 440, 24);
#ifdef ATLAS_RELEASE_BUILD
    version->setText(QStringLiteral(ATLAS_VERSION));
#else
    version->setText(QStringLiteral("%1 · Development %2")
                         .arg(QStringLiteral(ATLAS_VERSION),
                              QStringLiteral(ATLAS_BUILD_STRING)));
#endif
    version->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *company = new QLabel("by neutral software", card);
    company->setObjectName("splashCompany");
    company->setGeometry(160, 128, 440, 22);
    company->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    statusLabel = new QLabel("Loading the engine…", card);
    statusLabel->setObjectName("splashStatus");
    statusLabel->setGeometry(36, 198, 568, 24);
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *progress = new QProgressBar(card);
    progress->setObjectName("splashProgress");
    progress->setGeometry(36, 232, 568, 4);
    progress->setRange(0, 0);
    progress->setTextVisible(false);

#ifndef ATLAS_RELEASE_BUILD
    auto *warning = new QLabel("Development build · atlasengine.org", card);
    warning->setObjectName("splashWarning");
    warning->setGeometry(36, 246, 568, 20);
    warning->setWordWrap(true);
    warning->setAlignment(Qt::AlignLeft | Qt::AlignTop);
#endif

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
