#include <QMenu>
#include <QTimer>
#include <QPainter>
#include <QProcess>
#include <QMessageBox>

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
    mThunderbirdStartTime = QDateTime::currentDateTime().addSecs(settings->mLaunchThunderbirdDelay);

    mWinTools = WindowTools::create();
    if (mWinTools) {
        connect(mWinTools, &WindowTools::onWindowShown, this, &TrayIcon::onThunderbirdWindowShown);
        connect(mWinTools, &WindowTools::onWindowHidden, this, &TrayIcon::onThunderbirdWindowHidden);
    }
    createMenu();

    // On the first start, start at login and monitor the inboxes of all Thunderbird accounts
    QStringList inboxes;
    if (settings->isFirstRun) {
        DialogSettings::setAutostart(true);
    }
    if (settings->isFirstRun && settings->watchedMorkFiles.isEmpty()) {
        inboxes = Thunderbird::inboxes();
        settings->watchedMorkFiles = inboxes;
        if (!inboxes.isEmpty()) {
            settings->save();
        }
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

    if (!inboxes.isEmpty()) {
        showMessage(tr("Birdtray Lite"), tr("Monitoring %n inbox(es) from Thunderbird.", "", inboxes.size()));
    }

    // If the settings are not yet configure, pop up the message
    if (!showSettings && settings->showDialogIfNoAccountsConfigured
        && settings->watchedMorkFiles.isEmpty()) {
        QMessageBox questionDialog(QMessageBox::Question, tr("Would you like to set up Birdtray Lite?"),
                tr("You have not yet configured any email folders to monitor. "
                   "Would you like to do it now?"), QMessageBox::Yes | QMessageBox::No);
        QPushButton* dontAskAgainButton = questionDialog.addButton(
                tr("Don't ask again"), QMessageBox::RejectRole);
        showSettings = questionDialog.exec() == QMessageBox::Yes;
        if (questionDialog.clickedButton() == dontAskAgainButton) {
            settings->showDialogIfNoAccountsConfigured = false;
            settings->save();
        }
    }
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

void TrayIcon::unreadCounterUpdate( unsigned int total )
{
    Log::debug("unreadCounterUpdate %d", total );
    mUnreadCounter = total;

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

    p.drawPixmap(settings->getNotificationIcon().rect(), settings->getNotificationIcon());

    // Do we need to draw error sign?
    if (settings->mMonitorThunderbirdWindow && !mThunderbirdWindowExists) {
        p.setOpacity( 1.0 );
        QPen pen( Qt::red );
        pen.setWidth( (temp.width() * 10) / 100 );
        p.setPen( pen );
        p.drawLine( 2, 2, temp.width() - 3, temp.height() - 3 );
        p.drawLine( temp.width() - 3, 2, 2, temp.height() - 3 );
        unread = 0;
    }

    // Do we need to draw the unread counter?
    if (unread > 0 && settings->mShowUnreadEmailCount) {
        drawCountBadge(p, temp.size(), unread, settings->mCountColor);
    }
    const QMap<QString, QString> warnings = mUnreadMonitor->getWarnings();
    if (warnings.isEmpty()) {
        setToolTip(QString());
    } else {
        QStringList toolTip;
        const QString &globalWarning = warnings.value(QString());
        if (!globalWarning.isNull()) {
            toolTip << tr("Warning: %1").arg(globalWarning);
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
        setToolTip(toolTip.join('\n'));
        drawWarningIndicator(p, temp.size());
    }

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

            // Hide the window if requested
            if ( mThunderbirdWindowHide )
            {
                mThunderbirdWindowHide = false;
                hideThunderbird();
            }
        }
        else
        {
            Settings* settings = BirdtrayApp::get()->getSettings();
            // Thunderbird is not running. Has it run before?
            if (!mThunderbirdWindowExisted) {
                // No. Shall we start it?
                if (settings->mLaunchThunderbird && !mThunderbirdProcess &&
                    mThunderbirdStartTime < QDateTime::currentDateTime()) {
                    startThunderbird();

                    // Hide after?
                    if (settings->mStartThunderbirdHidden) {
                        mThunderbirdWindowHide = true;
                    }
                }
            } else {
                // It has run before, but not running now. Should we restart?
                if (settings->mRestartThunderbird && !mThunderbirdProcess) {
                    startThunderbird();

                    // Hide after?
                    if (settings->mStartThunderbirdHidden) {
                        mThunderbirdWindowHide = true;
                    }
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
    createMenu();
    updateIcon();
    emit settingsChanged();
}

void TrayIcon::actionActivate()
{
    if ( !mWinTools )
        return;

    Settings* settings = BirdtrayApp::get()->getSettings();
    if ( settings->startClosedThunderbird && !mWinTools->lookup() && mThunderbirdProcess == nullptr ) {
        startThunderbird();
        if (settings->mStartThunderbirdHidden) {
            mThunderbirdWindowHide = true;
        }
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

    if ( !settings->getStartThunderbirdCmdline( executable, args ) )
        return;

    args << "-compose";
    QProcess::startDetached(executable, args);
}

void TrayIcon::actionSystrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        Settings* settings = BirdtrayApp::get()->getSettings();
        if (settings->mShowHideThunderbird || (!mThunderbirdWindowExists && settings->startClosedThunderbird)) {
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

    if ( !BirdtrayApp::get()->getSettings()->getStartThunderbirdCmdline( executable, args ) )
    {
        Log::debug("Failed to get Thunderbird command-line" );
        return;
    }

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

void TrayIcon::hideThunderbird()
{
    mWinTools->hide();
}

void TrayIcon::showThunderbird()
{
    mWinTools->show();
}

void TrayIcon::drawCountBadge(QPainter &painter, const QSize &iconSize, unsigned int count,
                              const QColor &color) {
    // A pill in the bottom right corner that grows with the number of digits
    QString text = count > 99 ? QString("99+") : QString::number(count);
    double height = iconSize.height() * 0.56;
    double width = height + (text.length() - 1) * iconSize.width() * 0.23;
    QRectF badge(iconSize.width() - width, iconSize.height() - height, width, height);
    badge.adjust(2, 2, -2, -2);

    painter.setOpacity(1.0);
    painter.setPen(QPen(Qt::white, iconSize.width() * 0.05));
    painter.setBrush(color);
    painter.drawRoundedRect(badge, badge.height() / 2, badge.height() / 2);

    QFont font = QApplication::font();
    font.setWeight(QFont::ExtraBold);
    font.setPixelSize(static_cast<int>(height * (text.length() > 2 ? 0.6 : 0.78)));
    painter.setFont(font);
    painter.setPen(Qt::white);
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
