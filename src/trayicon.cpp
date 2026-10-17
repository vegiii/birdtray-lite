#include <QMenu>
#include <QTimer>
#include <QPainter>
#include <QProcess>
#include <QMessageBox>
#include <QPainterPath>

#include "trayicon.h"
#include "unreadmonitor.h"
#include "windowtools.h"
#include "utils.h"
#include "birdtrayapp.h"
#include "thunderbird.h"
#include "log.h"

TrayIcon::TrayIcon(bool showSettings)
{
    mUnreadCounter = 0;

    // Context menu
    mSystrayMenu = new QMenu();
    setContextMenu( mSystrayMenu );

    mMenuShowHideThunderbird = 0;
    mThunderbirdProcess = nullptr;
    
    mThunderbirdWindowExists = false;
    mThunderbirdWindowExisted = false;
    mThunderbirdWindowHide = false;
    connect(QApplication::instance(), &QApplication::aboutToQuit, this, &TrayIcon::onQuit);

    Settings* settings = BirdtrayApp::get()->getSettings();

    mWinTools = WindowTools::create();
    if (mWinTools) {
        connect(mWinTools, &WindowTools::onWindowShown, this, &TrayIcon::onThunderbirdWindowShown);
        connect(mWinTools, &WindowTools::onWindowHidden, this, &TrayIcon::onThunderbirdWindowHidden);
        connect(mWinTools, &WindowTools::onWindowAdded, this, &TrayIcon::onThunderbirdWindowAdded);
        mWinTools->startWatcher(settings->mHideWhenMinimized);
    }
    createMenu();

    // On the first start, start at login, monitor the inboxes of all Thunderbird accounts
    // and show the settings so they can be checked
    if (settings->isFirstRun) {
        DialogSettings::setAutostart(true);
        settings->watchedMorkFiles = Thunderbird::inboxes();
        settings->save();
        showSettings = true;
    }
    createUnreadCounterThread();

    connect( this, &TrayIcon::activated, this, &TrayIcon::actionSystrayIconActivated );

    // State timer
    connect( &mStateTimer, &QTimer::timeout, this, &TrayIcon::updateState );
    mStateTimer.setInterval( 1000 );
    mStateTimer.start();
    
    // Update the state and icon when everything is settled
    updateState();
    updateIcon();
    show();

    if (showSettings) {
        QTimer::singleShot(0, this, &TrayIcon::showSettings);
    }
}

TrayIcon::~TrayIcon() {
    if (settingsDialog != nullptr) {
        settingsDialog->deleteLater();
    }
    if (mUnreadMonitor != nullptr) {
        if (mUnreadMonitor->isRunning()) {
            mUnreadMonitor->quit();
            mUnreadMonitor->wait();
        }
        mUnreadMonitor->deleteLater();
        mUnreadMonitor = nullptr;
    }
}

WindowTools* TrayIcon::getWindowTools() const {
    return mWinTools;
}

UnreadMonitor* TrayIcon::getUnreadMonitor() const {
    return mUnreadMonitor;
}

void TrayIcon::unreadCounterUpdate( const QMap<QString, quint32>& counts )
{
    mUnreadCounts = counts;
    mUnreadCounter = 0;
    for ( quint32 count : counts )
        mUnreadCounter += count;
    Log::debug("unreadCounterUpdate %d", mUnreadCounter );

    updateIcon();
}

void TrayIcon::unreadMonitorWarningChanged(const QString &path) {
    const QString &message = mUnreadMonitor->getWarnings().value(path);
    if (!message.isNull()) {
        qWarning("UnreadMonitor generated a warning for %s: %s",
                qPrintable(path), qPrintable(message));
    }
    updateIcon();
}

void TrayIcon::updateIcon()
{
    Settings* settings = BirdtrayApp::get()->getSettings();
    // How many unread messages are here?
    unsigned int unread = mUnreadCounter;

    if (settings->onlyShowIconOnUnreadMessages && unread == 0) {
        this->hide();
        return;
    }

    QPixmap temp(settings->getNotificationIcon().size());
    QPainter p;

    temp.fill( Qt::transparent );
    p.begin( &temp );
    p.setRenderHint( QPainter::Antialiasing );

    bool thunderbirdMissing = !mThunderbirdWindowExists;
    bool showCount = unread > 0 && settings->mShowUnreadEmailCount && !thunderbirdMissing;
    p.drawPixmap(settings->getNotificationIcon().rect(), settings->getNotificationIcon());

    // Do we need to draw error sign?
    if (thunderbirdMissing) {
        p.setOpacity( 1.0 );
        QPen pen( Qt::red );
        pen.setWidth( (temp.width() * 10) / 100 );
        p.setPen( pen );
        p.drawLine( 2, 2, temp.width() - 3, temp.height() - 3 );
        p.drawLine( temp.width() - 3, 2, 2, temp.height() - 3 );
    }

    if (showCount && settings->mCountBadge) {
        drawCountBadge(p, temp.size(), unread, settings->mCountColor);
    } else if (showCount) {
        drawCount(p, temp.size(), unread, settings->mCountColor);
    }
    // The tooltip shows the unread count of each account, followed by any warnings
    QStringList toolTip;
    if (mUnreadCounter == 0) {
        toolTip << tr("No unread mail");
    } else {
        QMap<QString, unsigned int> accountCounts;
        for (auto it = mUnreadCounts.cbegin(); it != mUnreadCounts.cend(); ++it) {
            if (it.value() > 0) {
                accountCounts[Utils::getMailAccountName(QFileInfo(it.key()))] += it.value();
            }
        }
        for (auto it = accountCounts.cbegin(); it != accountCounts.cend(); ++it) {
            toolTip << QString("%1: %2").arg(it.key()).arg(it.value());
        }
    }

    const QMap<QString, QString> warnings = mUnreadMonitor->getWarnings();
    if (!warnings.isEmpty()) {
        toolTip << QString();
        const QString &globalWarning = warnings.value(QString());
        if (!globalWarning.isNull()) {
            toolTip << globalWarning;
        }
        QMapIterator<QString, QString> warningsIterator(warnings);
        while (warningsIterator.hasNext()) {
            warningsIterator.next();
            const QString &path = warningsIterator.key();
            if (path.isNull()) {
                continue;
            }
            QFileInfo accountMorkFile(path);
            QString accountName = Utils::getMailAccountName(accountMorkFile);
            QString mailFolderName = Utils::getMailFolderName(accountMorkFile);
            QString name;
            if (accountName.isNull() || mailFolderName.isNull()) {
                name = path;
            } else {
                name = accountName + " [" + mailFolderName + "]";
            }
            const QString &warning = warningsIterator.value();
            toolTip << name + ": " + warning;
        }
        drawWarningIndicator(p, temp.size());
    }
    setToolTip(toolTip.join('\n'));

    p.end();

    // FIXME: this is not very efficient, although at our icon sizes (128x128) is probably ok.
    if ( mLastDrawnIcon != temp.toImage() )
    {
        mLastDrawnIcon = temp.toImage();
        setIcon( temp );
    }
    this->show();
}

void TrayIcon::updateState()
{
    if ( mWinTools )
    {
        mThunderbirdWindowExists = mWinTools->lookup();

        // Is Thunderbird running?
        if ( mThunderbirdWindowExists )
        {
            // If the window is found, we remember it
            if ( !mThunderbirdWindowExisted )
                mThunderbirdWindowExisted = true;

            if ( !mMenuShowHideThunderbird->isEnabled() )
                mMenuShowHideThunderbird->setEnabled( true );
        }
        else
        {
            Settings* settings = BirdtrayApp::get()->getSettings();
            // Thunderbird is not running. Start it, or restart it if it has run before
            if (settings->mKeepThunderbirdRunning && !mThunderbirdProcess) {
                startThunderbird();

                // Hide once the window opens?
                if (settings->mStartThunderbirdHidden) {
                    mThunderbirdWindowHide = true;
                }
            }
        }

        // Update the menu text as the window can be hidden in wintools
        if ( mWinTools->isHidden() )
            mMenuShowHideThunderbird->setText( tr("Show Thunderbird") );
        else
            mMenuShowHideThunderbird->setText( tr("Hide Thunderbird") );

        updateIcon();
    }
}

void TrayIcon::actionQuit()
{
    QApplication::quit();
}

void TrayIcon::showSettings()
{
    if (settingsDialog != nullptr) {
        settingsDialog->show();
        settingsDialog->raise();
        settingsDialog->activateWindow();
        return;
    }
    settingsDialog = new DialogSettings();
    connect(settingsDialog, &QDialog::finished, this, [=]() {
        settingsDialog->deleteLater();
        settingsDialog = nullptr;
    });
    settingsDialog->show();
}

void TrayIcon::applySettings()
{
    BirdtrayApp::get()->getSettings()->save();
    BirdtrayApp::get()->getSettings()->resetNotificationIcon();
    createMenu();
    updateIcon();
    if (mWinTools) {
        mWinTools->startWatcher(BirdtrayApp::get()->getSettings()->mHideWhenMinimized);
    }
    emit settingsChanged();
}

void TrayIcon::actionActivate()
{
    if ( !mWinTools )
        return;

    // Start Thunderbird if it is closed, and show it since the user asked for it
    if ( !mWinTools->lookup() && mThunderbirdProcess == nullptr ) {
        startThunderbird();
    } else if ( mWinTools->isHidden() ) {
        showThunderbird();
    } else {
        hideThunderbird();
    }
}

void TrayIcon::actionNewEmail() {
    Settings* settings = BirdtrayApp::get()->getSettings();

    QString executable;
    QStringList args;

    settings->getStartThunderbirdCmdline( executable, args );
    args << "-compose";
    QProcess::startDetached(executable, args);
}

void TrayIcon::actionSystrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        Settings* settings = BirdtrayApp::get()->getSettings();
        if (settings->mShowHideThunderbird) {
            actionActivate();
        }
    }
}

void TrayIcon::createMenu()
{
    Settings* settings = BirdtrayApp::get()->getSettings();
    mSystrayMenu->clear();

    // Show and hide action
    mMenuShowHideThunderbird = new QAction( tr("Hide Thunderbird"), this );
    connect( mMenuShowHideThunderbird, &QAction::triggered, this, &TrayIcon::actionActivate );

    // We start with disabled action, and enable it once the window is detected
    mMenuShowHideThunderbird->setEnabled( false );

    mSystrayMenu->addAction( mMenuShowHideThunderbird );
    mSystrayMenu->addSeparator();

    if (settings->mNewEmailMenuEnabled) {
        mSystrayMenu->addAction(tr("New Email"), this, SLOT(actionNewEmail()));
        mSystrayMenu->addSeparator();
    }

    // Some generic actions
    mSystrayMenu->addAction( tr("Settings..."), this, SLOT(showSettings()) );

    mSystrayMenu->addSeparator();

    // And exit
    mSystrayMenu->addAction( tr("Quit"), this, SLOT(actionQuit()) );
}

void TrayIcon::createUnreadCounterThread()
{
    mUnreadMonitor = new UnreadMonitor( this );

    connect( mUnreadMonitor, &UnreadMonitor::unreadUpdated, this, &TrayIcon::unreadCounterUpdate );
    connect(mUnreadMonitor, &UnreadMonitor::warningChanged, this,
            &TrayIcon::unreadMonitorWarningChanged);

    mUnreadMonitor->start();
}

void TrayIcon::startThunderbird()
{
    if ( mThunderbirdProcess ) {
        Log::debug("Not starting Thunderbird because we already started it and it is still running" );
        return;
    }

    QString executable;
    QStringList args;

    BirdtrayApp::get()->getSettings()->getStartThunderbirdCmdline( executable, args );

    Log::debug("Starting Thunderbird as '%s %s'", qPrintable(executable), qPrintable(args.join(' ')));

    mThunderbirdProcess = new QProcess();
    connect( mThunderbirdProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(tbProcessFinished(int,QProcess::ExitStatus)) );

    connect( mThunderbirdProcess, &QProcess::errorOccurred, this, &TrayIcon::tbProcessError );

    mThunderbirdProcess->start(executable, args);
}

void TrayIcon::tbProcessError(QProcess::ProcessError error)
{
    if (mThunderbirdProcess == nullptr) {
        Log::debug("Got Thunderbird process error %s, but mThunderbirdProcess was null", qPrintable(QVariant::fromValue(error).toString()));
        return;
    }

    QMessageBox::critical(nullptr,
            tr("Cannot start Thunderbird"),
            tr("Error starting Thunderbird as '%1 %2':\n\n%3")
                    .arg( mThunderbirdProcess->program() )
                    .arg( mThunderbirdProcess->arguments().join(' ') )
                    .arg( mThunderbirdProcess->errorString()) );

    // We keep the mThunderbirdProcess pointer, so the process is not restarted again
}

void TrayIcon::tbProcessFinished(int, QProcess::ExitStatus)
{
    // If we are here this could mean that either Thunderbird was quit manually,
    // in which case it is restarted in updateState(), or that we started TB
    // and the active instance was activated (and our instance exited).
    // Thus we just destroy the process later, to let updateState() make decision
    if (mThunderbirdProcess != nullptr) {
        mThunderbirdProcess->deleteLater();
        mThunderbirdProcess = nullptr;
    }
}


void TrayIcon::onQuit() {
    if (mWinTools) {
        mWinTools->stopWatcher();
    }
    if (mWinTools && mWinTools->isHidden()) {
        mWinTools->show();
    }
    if (BirdtrayApp::get()->getSettings()->mExitThunderbirdWhenQuit) {
        if (mWinTools) {
            mWinTools->closeWindow();
        }
    }
}

void TrayIcon::onThunderbirdWindowShown() {
    mMenuShowHideThunderbird->setText( tr("Hide Thunderbird") );
}

void TrayIcon::onThunderbirdWindowHidden() {
    mMenuShowHideThunderbird->setText( tr("Show Thunderbird") );
}

void TrayIcon::onThunderbirdWindowAdded() {
    // Hide the window we started hidden now that it exists
    if (mThunderbirdWindowHide) {
        mThunderbirdWindowHide = false;
        hideThunderbird();
    }
}

void TrayIcon::hideThunderbird()
{
    mWinTools->hide();
}

void TrayIcon::showThunderbird()
{
    mWinTools->show();
}

void TrayIcon::drawCount(QPainter &painter, const QSize &iconSize, unsigned int count,
                         const QColor &color) {
    QFont font = QApplication::font();
    font.setWeight(QFont::ExtraBold);
    font.setPixelSize(100);
    QPainterPath path;
    path.addText(0, 0, font, QString::number(count));

    // Scale the number to fill the icon and center it
    QRectF bounds = path.boundingRect();
    double scale = std::min(iconSize.width() * 0.92 / bounds.width(),
                            iconSize.height() * 0.62 / bounds.height());
    QTransform transform;
    transform.translate(iconSize.width() / 2.0, iconSize.height() / 2.0);
    transform.scale(scale, scale);
    transform.translate(-bounds.center().x(), -bounds.center().y());
    path = transform.map(path);

    painter.setOpacity(1.0);
    QPen outline(QColor(17, 17, 17), iconSize.width() * 0.09);
    outline.setJoinStyle(Qt::RoundJoin);
    painter.strokePath(path, outline);
    painter.fillPath(path, color);
}

void TrayIcon::drawCountBadge(QPainter &painter, const QSize &iconSize, unsigned int count,
                              const QColor &color) {
    // A dot in the top right corner that grows into a pill with the number of digits
    QString text = count > 99 ? QString("99+") : QString::number(count);
    double height = iconSize.height() * 0.5;
    double width = height + (text.length() - 1) * iconSize.width() * 0.2;
    QRectF badge(iconSize.width() - width, 0, width, height);

    // Dark text on light badge colors, white text on dark ones
    QColor textColor = color.lightnessF() > 0.6 ? QColor(17, 17, 17) : QColor(Qt::white);

    painter.setOpacity(1.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(badge, badge.height() / 2, badge.height() / 2);

    QFont font = QApplication::font();
    font.setWeight(QFont::Bold);
    font.setPixelSize(static_cast<int>(height * (text.length() > 2 ? 0.55 : 0.72)));
    painter.setFont(font);
    painter.setPen(textColor);
    painter.drawText(badge, Qt::AlignCenter, text);
}

void TrayIcon::drawWarningIndicator(QPainter &painter, const QSize &iconSize) {
    painter.setOpacity(1.0);
    int width = iconSize.width() / 4;
    QPen pen(QColor(255, 200, 0, 255));
    pen.setWidth(width);
    painter.setPen(pen);
    int x = iconSize.width() - static_cast<int>(iconSize.width() * 0.125) - pen.width() / 2;
    painter.drawLine(x, static_cast<int>(iconSize.height() * 0.33),
            x, iconSize.height() - width / 2);
    pen.setColor(QColor(255, 120, 0, 255));
    pen.setWidthF(std::max(pen.width() - 16, 1));
    painter.setPen(pen);
    painter.drawLine(x, static_cast<int>(iconSize.height() * 0.33),
            x, iconSize.height() - 20 - width);
    painter.drawPoint(x, iconSize.height() - width / 2);
}
