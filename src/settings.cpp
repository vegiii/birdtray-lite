#include <QBuffer>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QtCore/QStandardPaths>
#include <QCoreApplication>
#include <QMessageBox>
#include <QtCore/QUrl>
#include <QtCore/QDirIterator>

#include "settings.h"
#include "utils.h"
#include "log.h"

#define BORDER_COLOR_KEY "common/bordercolor"
#define BORDER_WIDTH_KEY "common/borderwidth"
#define HORIZONTAL_UNREAD_COUNT_OFFSET_KEY "advanced/horizontalUnreadCountOffset"
#define VERTICAL_UNREAD_COUNT_OFFSET_KEY "advanced/verticalUnreadCountOffset"
#define START_CLOSED_THUNDERBIRD_KEY "common/startClosedThunderbird"
#define HIDE_WHEN_STARTED_MANUALLY_KEY "common/hideWhenStartedManually"
#define THUNDERBIRD_PROCESS_NAME_KEY "advanced/tbprocessname"
#define THUNDERBIRD_PROCESS_NAME_DEFAULT "thunderbird"
#define ONLY_SHOW_ICON_ON_UNREAD_MESSAGES_KEY "advanced/onlyShowIconOnUnreadMessages"

Settings::Settings()
{
    QString configDir = QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
    mSettingsFilename = configDir + "/config.json";

    // Make sure the path exists, and create it if it does not
    QDir apppath( configDir );

    if ( !apppath.exists() )
        apppath.mkpath( apppath.absolutePath() );

    mIconSize = QSize( ICON_SIZE, ICON_SIZE );
    mNotificationDefaultColor = QColor("#0000FF");
    mNotificationBorderColor = QColor("#FFFFFF");
    mNotificationBorderWidth = 15;
    horizontalUnreadCountOffset = 0.0;
    verticalUnreadCountOffset = 0.0;
    mBlinkSpeed = 0;
    mShowHideThunderbird = false;
    mLaunchThunderbird = false;
    mExitThunderbirdWhenQuit = false;
    mNotificationFontWeight = 50;
    mMonitorThunderbirdWindow = false;
    mRestartThunderbird = false;
    mHideWhenStarted = false;
    mHideWhenRestarted = false;
    startClosedThunderbird = false;
    hideWhenStartedManually = false;
    mAllowSuppressingUnreads = false;
    mLaunchThunderbirdDelay = 0;
    mShowUnreadEmailCount = true;
    ignoreUnreadCountOnStart = false;
    ignoreUnreadCountOnShow = false;
    ignoreUnreadCountOnHide = false;
    showDialogIfNoAccountsConfigured = true;
    isFirstRun = false;
    mThunderbirdWindowMatch = " Mozilla Thunderbird";
    mThunderbirdProcessName = THUNDERBIRD_PROCESS_NAME_DEFAULT;
    mNotificationMinimumFontSize = 4;
    mNotificationMaximumFontSize = 512;
    mWatchFileTimeout = 150;
    mBlinkingUseAlphaTransition = false;
    onlyShowIconOnUnreadMessages = false;
    mUnreadOpacityLevel = 0.75;
    mNewEmailMenuEnabled = false;
    mIndexFilesRereadIntervalSec = 0;
    mThunderbirdCmdLine = Utils::getDefaultThunderbirdCommand();
}

Settings::~Settings()
{
}

void Settings::save()
{
    QJsonObject out;

    out[ "common/notificationfont" ] = mNotificationFont.toString();
    out[ "common/defaultcolor" ] = mNotificationDefaultColor.name();
    out[ BORDER_COLOR_KEY ] = mNotificationBorderColor.name();
    out[ BORDER_WIDTH_KEY ] = static_cast<int>( mNotificationBorderWidth );
    out[ HORIZONTAL_UNREAD_COUNT_OFFSET_KEY ] = horizontalUnreadCountOffset;
    out[ VERTICAL_UNREAD_COUNT_OFFSET_KEY ] = verticalUnreadCountOffset;
    out[ "common/blinkspeed" ] = static_cast<int>( mBlinkSpeed );
    out[ "common/showhidethunderbird" ] = mShowHideThunderbird;
    out[ "common/launchthunderbird" ] = mLaunchThunderbird;
    out[ "common/exitthunderbirdonquit" ] = mExitThunderbirdWhenQuit;
    out[ "common/restartthunderbird" ] = mRestartThunderbird;
    out[ "common/notificationfontweight" ] = static_cast<int>( mNotificationFontWeight );
    out[ "common/monitorthunderbirdwindow" ] = mMonitorThunderbirdWindow;
    out[ "common/hidewhenstarted" ] = mHideWhenStarted;
    out[ "common/hidewhenrestarted" ] = mHideWhenRestarted;
    out[ START_CLOSED_THUNDERBIRD_KEY ] = startClosedThunderbird;
    out[ HIDE_WHEN_STARTED_MANUALLY_KEY ] = hideWhenStartedManually;
    out[ "common/allowsuppressingunread" ] = mAllowSuppressingUnreads;
    out[ "common/launchthunderbirddelay" ] = mLaunchThunderbirdDelay;
    out[ "common/showunreademailcount" ] = mShowUnreadEmailCount;
    out[ "common/ignoreStartUnreadCount" ] = ignoreUnreadCountOnStart;
    out[ "common/ignoreShowUnreadCount" ] = ignoreUnreadCountOnShow;
    out[ "common/forceIgnoreUnreadEmailsOnMinimize" ] = ignoreUnreadCountOnHide;
    out[ "common/showDialogIfNoAccountsConfigured"  ] = showDialogIfNoAccountsConfigured;

    out[ "advanced/tbcmdline" ] = QJsonArray::fromStringList( mThunderbirdCmdLine );
    out[ "advanced/tbwindowmatch" ] = mThunderbirdWindowMatch;
    out[ THUNDERBIRD_PROCESS_NAME_KEY ] = mThunderbirdProcessName;
    out[ "advanced/notificationfontminsize" ] = static_cast<int>( mNotificationMinimumFontSize );
    out[ "advanced/notificationfontmaxsize" ] = static_cast<int>( mNotificationMaximumFontSize );
    out[ "advanced/watchfiletimeout" ] = static_cast<int>( mWatchFileTimeout );
    out[ "advanced/blinkingusealpha" ] = mBlinkingUseAlphaTransition;
    out[ "advanced/unreadopacitylevel" ] = mUnreadOpacityLevel;
    out[ ONLY_SHOW_ICON_ON_UNREAD_MESSAGES_KEY ] = onlyShowIconOnUnreadMessages;
    out[ "advanced/forcedRereadInterval" ] = static_cast<int>( mIndexFilesRereadIntervalSec );
    out[ "advanced/runProcessOnChange" ] = mProcessRunOnCountChange;

    // Store the account map
    QJsonArray accounts;

    for ( const QString& path : watchedMorkFiles.orderedKeys() )
    {
        QJsonObject ac;
        ac[ "color" ] = watchedMorkFiles[path].name();
        ac[ "path" ] = path;

        accounts.push_back( ac );
    }

    out[ "accounts" ] = accounts;

    // Store the new email data
    QJsonArray newemaildata;

    out[ "common/newemailEnabled" ] = mNewEmailMenuEnabled;

    for ( const Setting_NewEmail& e :  mNewEmailData )
        newemaildata.push_back( e.toJSON() );

    if ( !newemaildata.isEmpty() )
        out[ "newemails" ] = newemaildata;

    if ( !mNotificationIconUnread.isNull() ) {
        out[ "common/notificationiconunread" ] = Utils::pixmapToString( mNotificationIconUnread );
    }
    out[ "common/notificationicon" ] = Utils::pixmapToString( mNotificationIcon );

    // QSaveFile is an I/O device for writing text and binary files, without
    // losing existing data if the writing operation fails.
    QSaveFile file( mSettingsFilename );

    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        QMessageBox::critical(nullptr, tr("Could not save the settings"),
                tr("Could not save the settings into file %1:\n%2")
                        .arg(file.fileName()).arg(file.errorString()));
        return;
    }

    QJsonDocument document( out );

    // QSaveFile will remember the write error happen, so no need to check
    file.write( document.toJson() );
    file.commit();
}

void Settings::load()
{
    // Load the settings file
    QFile file( mSettingsFilename );

    if ( file.open(QIODevice::ReadOnly ) )
    {
        QByteArray data = file.readAll();

        if ( !data.isEmpty() )
        {
            QJsonDocument document = QJsonDocument::fromJson(data);

            if ( document.isObject() )
            {
                // Parse the settings from JSON
                fromJSON( document.object() );
                return;
            }
        }
    }

    (void) getNotificationIcon(); // Load the default
    isFirstRun = true;

    save();
}

void Settings::fromJSON( const QJsonObject& settings )
{
    if ( settings.contains( "common/notificationfont" ) )
        mNotificationFont.fromString( settings.value( "common/notificationfont" ).toString() );

    mNotificationIcon = Utils::pixmapFromString(
            settings.value("common/notificationicon").toString());
    mNotificationIconUnread = Utils::pixmapFromString(
            settings.value("common/notificationiconunread").toString());

    (void) getNotificationIcon(); // Load the default

    mNotificationDefaultColor = QColor( settings.value( "common/defaultcolor" ).toString() );
    mNotificationBorderColor = QColor(settings.value(BORDER_COLOR_KEY).toString() );

    // Disable border on existing installations
    if ( settings.contains( "common/defaultcolor") )
        mNotificationBorderWidth = settings.value( BORDER_WIDTH_KEY ).toInt();

    horizontalUnreadCountOffset = std::clamp(settings.value(HORIZONTAL_UNREAD_COUNT_OFFSET_KEY).toDouble(), -1.0, 1.0);
    verticalUnreadCountOffset = std::clamp(settings.value(VERTICAL_UNREAD_COUNT_OFFSET_KEY).toDouble(), -1.0, 1.0);
    mBlinkSpeed = settings.value("common/blinkspeed").toInt();
    mShowHideThunderbird = settings.value("common/showhidethunderbird").toBool();
    mLaunchThunderbird = settings.value("common/launchthunderbird").toBool();
    mExitThunderbirdWhenQuit = settings.value("common/exitthunderbirdonquit").toBool();
    mNotificationFontWeight = qMin(99, settings.value("common/notificationfontweight").toInt());
    mMonitorThunderbirdWindow = settings.value("common/monitorthunderbirdwindow").toBool();
    mRestartThunderbird = settings.value("common/restartthunderbird").toBool();
    mHideWhenStarted = settings.value("common/hidewhenstarted").toBool();
    mHideWhenRestarted = settings.value("common/hidewhenrestarted").toBool();
    startClosedThunderbird = settings.value(START_CLOSED_THUNDERBIRD_KEY).toBool();
    hideWhenStartedManually = settings.value(HIDE_WHEN_STARTED_MANUALLY_KEY).toBool();
    mAllowSuppressingUnreads = settings.value("common/allowsuppressingunread").toBool();
    mLaunchThunderbirdDelay = settings.value("common/launchthunderbirddelay").toInt();
    mShowUnreadEmailCount = settings.value("common/showunreademailcount").toBool();
    ignoreUnreadCountOnStart = mAllowSuppressingUnreads &&
            settings.value("common/ignoreStartUnreadCount").toBool();
    ignoreUnreadCountOnShow = mAllowSuppressingUnreads &&
            settings.value("common/ignoreShowUnreadCount").toBool();
    ignoreUnreadCountOnHide = mAllowSuppressingUnreads &&
            settings.value("common/forceIgnoreUnreadEmailsOnMinimize").toBool();
    showDialogIfNoAccountsConfigured = settings.value("common/showDialogIfNoAccountsConfigured").toBool();

    mThunderbirdWindowMatch = settings.value("advanced/tbwindowmatch").toString();
    mThunderbirdProcessName = settings.value(THUNDERBIRD_PROCESS_NAME_KEY).toString(mThunderbirdProcessName);
    mNotificationMinimumFontSize = settings.value("advanced/notificationfontminsize").toInt();
    mNotificationMaximumFontSize = settings.value("advanced/notificationfontmaxsize").toInt();
    mWatchFileTimeout = settings.value("advanced/watchfiletimeout").toInt();
    mBlinkingUseAlphaTransition = settings.value("advanced/blinkingusealpha").toBool();
    mUnreadOpacityLevel = settings.value("advanced/unreadopacitylevel").toDouble();
    onlyShowIconOnUnreadMessages = settings.value(ONLY_SHOW_ICON_ON_UNREAD_MESSAGES_KEY).toBool();
    mIndexFilesRereadIntervalSec = settings.value("advanced/forcedRereadInterval").toInt();
    mProcessRunOnCountChange = settings.value( "advanced/runProcessOnChange" ).toString();

    QStringList thunderbirdCommand = settings.value("advanced/tbcmdline").toVariant().toStringList();
    if ( !thunderbirdCommand.isEmpty() && !thunderbirdCommand[0].isEmpty() )
        mThunderbirdCmdLine = thunderbirdCommand;

    watchedMorkFiles.clear();

    // Convert the map into settings
    if ( settings["accounts"].isArray() )
    {
        for ( const QJsonValueRef& a : settings["accounts"].toArray() )
        {
            QString path = a.toObject().value("path").toString();
            QString color = a.toObject().value("color").toString();

            if ( path.isEmpty() )
                continue;

            watchedMorkFiles[ path ] = QColor( color );
        }
    }

    // Load new email data from settings
    mNewEmailMenuEnabled = settings.value("common/newemailEnabled").toBool();

    mNewEmailData.clear();

    if ( settings["newemails"].isArray() )
    {
        for ( const QJsonValueRef& a : settings["newemails"].toArray() )
            mNewEmailData.push_back( Setting_NewEmail::fromJSON( a.toObject() ) );
    }
}

bool Settings::getStartThunderbirdCmdline( QString& executable, QStringList &arguments )
{
    if ( mThunderbirdCmdLine.isEmpty() )
        return false;

    arguments = mThunderbirdCmdLine;

    executable = Utils::expandPath( arguments.takeFirst() );

    return true;
}

const QPixmap &Settings::getNotificationIcon() {
    if (mNotificationIcon.isNull()) {
        if (!mNotificationIcon.load(":res/thunderbird.png")) {
            Log::fatal( tr("Cannot load default system tray icon.") );
        }
    }
    return mNotificationIcon;
}

void Settings::setNotificationIcon(const QPixmap& icon) {
    mNotificationIcon = icon;
}
