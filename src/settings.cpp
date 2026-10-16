#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QtCore/QStandardPaths>
#include <QMessageBox>

#include "settings.h"
#include "utils.h"
#include "log.h"

Settings::Settings()
{
    QString configDir = QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
    mSettingsFilename = configDir + "/config.json";

    // Make sure the path exists, and create it if it does not
    QDir apppath( configDir );

    if ( !apppath.exists() )
        apppath.mkpath( apppath.absolutePath() );

    mNotificationDefaultColor = QColor("#0000FF");
    mBlinkSpeed = 0;
    mShowUnreadEmailCount = true;
    onlyShowIconOnUnreadMessages = false;
    mNewEmailMenuEnabled = false;
    mThunderbirdCmdLine = Utils::getDefaultThunderbirdCommand();
    mThunderbirdProcessName = "thunderbird";
    mThunderbirdWindowMatch = " Mozilla Thunderbird";
    mShowHideThunderbird = false;
    mLaunchThunderbird = false;
    mLaunchThunderbirdDelay = 0;
    mRestartThunderbird = false;
    startClosedThunderbird = false;
    mStartThunderbirdHidden = false;
    mExitThunderbirdWhenQuit = false;
    mMonitorThunderbirdWindow = false;
    showDialogIfNoAccountsConfigured = true;
    isFirstRun = false;
}

void Settings::save()
{
    QJsonObject out;

    out[ "common/defaultcolor" ] = mNotificationDefaultColor.name();
    out[ "common/blinkspeed" ] = static_cast<int>( mBlinkSpeed );
    out[ "common/showunreademailcount" ] = mShowUnreadEmailCount;
    out[ "advanced/onlyShowIconOnUnreadMessages" ] = onlyShowIconOnUnreadMessages;
    out[ "common/newemailEnabled" ] = mNewEmailMenuEnabled;
    out[ "common/showhidethunderbird" ] = mShowHideThunderbird;
    out[ "common/launchthunderbird" ] = mLaunchThunderbird;
    out[ "common/launchthunderbirddelay" ] = mLaunchThunderbirdDelay;
    out[ "common/restartthunderbird" ] = mRestartThunderbird;
    out[ "common/startClosedThunderbird" ] = startClosedThunderbird;
    out[ "common/startThunderbirdHidden" ] = mStartThunderbirdHidden;
    out[ "common/exitthunderbirdonquit" ] = mExitThunderbirdWhenQuit;
    out[ "common/monitorthunderbirdwindow" ] = mMonitorThunderbirdWindow;
    out[ "common/showDialogIfNoAccountsConfigured" ] = showDialogIfNoAccountsConfigured;

    out[ "advanced/tbcmdline" ] = QJsonArray::fromStringList( mThunderbirdCmdLine );
    out[ "advanced/tbprocessname" ] = mThunderbirdProcessName;
    out[ "advanced/tbwindowmatch" ] = mThunderbirdWindowMatch;

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
        QJsonDocument document = QJsonDocument::fromJson( file.readAll() );

        if ( document.isObject() )
        {
            // Parse the settings from JSON, and save to drop keys that are no longer used
            fromJSON( document.object() );
            save();
            return;
        }
    }

    isFirstRun = true;
    save();
}

void Settings::fromJSON( const QJsonObject& settings )
{
    // Missing keys keep their default value
    mNotificationDefaultColor = QColor( settings.value( "common/defaultcolor" ).toString( mNotificationDefaultColor.name() ) );
    mBlinkSpeed = settings.value( "common/blinkspeed" ).toInt( mBlinkSpeed );
    mShowUnreadEmailCount = settings.value( "common/showunreademailcount" ).toBool( mShowUnreadEmailCount );
    onlyShowIconOnUnreadMessages = settings.value( "advanced/onlyShowIconOnUnreadMessages" ).toBool(
            onlyShowIconOnUnreadMessages );
    mNewEmailMenuEnabled = settings.value( "common/newemailEnabled" ).toBool( mNewEmailMenuEnabled );
    mShowHideThunderbird = settings.value( "common/showhidethunderbird" ).toBool( mShowHideThunderbird );
    mLaunchThunderbird = settings.value( "common/launchthunderbird" ).toBool( mLaunchThunderbird );
    mLaunchThunderbirdDelay = settings.value( "common/launchthunderbirddelay" ).toInt( mLaunchThunderbirdDelay );
    mRestartThunderbird = settings.value( "common/restartthunderbird" ).toBool( mRestartThunderbird );
    startClosedThunderbird = settings.value( "common/startClosedThunderbird" ).toBool( startClosedThunderbird );
    mStartThunderbirdHidden = settings.value( "common/startThunderbirdHidden" ).toBool(
            settings.value( "common/hidewhenstarted" ).toBool( mStartThunderbirdHidden ) );
    mExitThunderbirdWhenQuit = settings.value( "common/exitthunderbirdonquit" ).toBool( mExitThunderbirdWhenQuit );
    mMonitorThunderbirdWindow = settings.value( "common/monitorthunderbirdwindow" ).toBool( mMonitorThunderbirdWindow );
    showDialogIfNoAccountsConfigured = settings.value( "common/showDialogIfNoAccountsConfigured" ).toBool(
            showDialogIfNoAccountsConfigured );

    mThunderbirdProcessName = settings.value( "advanced/tbprocessname" ).toString( mThunderbirdProcessName );
    mThunderbirdWindowMatch = settings.value( "advanced/tbwindowmatch" ).toString( mThunderbirdWindowMatch );

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
