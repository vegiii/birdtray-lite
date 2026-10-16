#include <QTimer>
#include <QDir>

#include "unreadmonitor.h"
#include "morkparser.h"
#include "trayicon.h"
#include "log.h"
#include "birdtrayapp.h"

UnreadMonitor::UnreadMonitor( TrayIcon * parent )
    : QThread( 0 ), mChangedMSFtimer(this)
{
    moveToThread( this );
    mLastReportedUnread = 0;

    // We get notification once Mork files have been modified.
    connect( &mDBWatcher, &QFileSystemWatcher::fileChanged, this, &UnreadMonitor::watchedFileChanges );

    // Settings changed
    connect( parent, &TrayIcon::settingsChanged, this, &UnreadMonitor::slotSettingsChanged );

    // Set up the watched file timer, Thunderbird writes the index files in bursts
    mChangedMSFtimer.setInterval(150);
    mChangedMSFtimer.setSingleShot( true );

    connect( &mChangedMSFtimer, &QTimer::timeout, this, &UnreadMonitor::updateUnread );
}

void UnreadMonitor::run()
{
    // Start it as soon as thread starts its event loop
    QTimer::singleShot( 0, [=](){ updateUnread(); } );

    // Start the event loop
    exec();
}

const QMap<QString, QString> &UnreadMonitor::getWarnings() const {
    return warnings;
}

void UnreadMonitor::slotSettingsChanged()
{
    Settings* settings = BirdtrayApp::get()->getSettings();

    // We reinitialize everything because the settings changed
    mMorkUnreadCounts.clear();

    const QStringList &accountsList = settings->watchedMorkFiles;
    for (const QString &path : warnings.keys()) {
        if (!accountsList.contains(path)) {
            clearWarning(path);
        }
    }
    updateUnread();
}

void UnreadMonitor::watchedFileChanges(const QString &filechanged)
{
    if ( !mChangedMSFfiles.contains( filechanged ) )
        mChangedMSFfiles.push_back( filechanged );

    mChangedMSFtimer.start();
}

void UnreadMonitor::updateUnread()
{
    Log::debug("Triggering the unread counter update");

    int total = getUnreadCount_Mork();

    if ( total != mLastReportedUnread )
    {
        emit unreadUpdated( total );
        mLastReportedUnread = total;
    }
}

int UnreadMonitor::getUnreadCount_Mork()
{
    Settings* settings = BirdtrayApp::get()->getSettings();
    bool rescanall = false;

    // We rebuild and rescan the whole map if there is no such path in there, or map is empty (first run)
    if ( mMorkUnreadCounts.isEmpty() )
        rescanall = true;
    else
    {
        for ( QString path : mChangedMSFfiles )
            if ( !mMorkUnreadCounts.contains( path ) )
                rescanall = true;
    }

    if ( rescanall )
    {
        mMorkUnreadCounts.clear();
        for (const QString &path : settings->watchedMorkFiles) {
            mMorkUnreadCounts[path] = getMorkUnreadCount(path);
            if (!mDBWatcher.files().contains(path) && !mDBWatcher.addPath(path)) {
                setWarning(tr("Unable to watch %1 for changes.")
                        .arg(QFileInfo(path).fileName()), path);
            } else {
                clearWarning(path);
            }
        }
    }
    else
    {
        for ( QString path : mChangedMSFfiles )
            mMorkUnreadCounts[ path ] = getMorkUnreadCount( path );
    }

    int count = 0;
    for ( quint32 unread : mMorkUnreadCounts )
        count += unread;

    mChangedMSFfiles.clear();
    return count;
}

int UnreadMonitor::getMorkUnreadCount(const QString &path)
{
    MailMorkParser parser;
    if (!parser.open(path)) {
        Log::debug("Unable to parser mork file %s: %s", qPrintable( path ), qPrintable(parser.errorMsg()));
        setWarning(tr("Unable to read from %1.").arg(QFileInfo(path).fileName()), path);
        return 0;
    } else {
        clearWarning(path);
    }
    int unread = static_cast<int>(parser.getNumUnreadMessages());
    Log::debug("Unread counter for %s: %d", qPrintable( path ), unread );
    return unread;
}

void UnreadMonitor::setWarning(const QString &message, const QString &path) {
    if (warnings.value(path) != message) {
        warnings.insert(path, message);
        emit warningChanged(path);
    }
}

void UnreadMonitor::clearWarning(const QString &path) {
    if (warnings.remove(path) > 0) {
        emit warningChanged(path);
    }
}
