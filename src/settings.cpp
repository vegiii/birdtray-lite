#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QtCore/QStandardPaths>
#include <QIcon>
#include <QMessageBox>

#include "settings.h"
#include "thunderbird.h"
#include "log.h"

Settings::Settings()
{
    QString configDir = QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
    mSettingsFilename = configDir + "/config.json";

    // Make sure the path exists, and create it if it does not
    QDir apppath( configDir );

    if ( !apppath.exists() )
        apppath.mkpath( apppath.absolutePath() );

    mCountColor = QColor("#f44336");
    mCountBadge = true;
    mThemeTrayIcon = false;
    mShowUnreadEmailCount = true;
    onlyShowIconOnUnreadMessages = false;
    mShowWhenRelevant = false;
    mNewEmailMenuEnabled = true;
    mThunderbirdCmdLine = Thunderbird::defaultCommand();
    mShowHideThunderbird = true;
    mKeepThunderbirdRunning = true;
    mStartThunderbirdHidden = true;
    mHideWhenMinimized = true;
    mExitThunderbirdWhenQuit = true;
    isFirstRun = false;
}

void Settings::save()
{
    QJsonObject out;

    out[ "common/countcolor" ] = mCountColor.name();
    out[ "common/countstyle" ] = mCountBadge ? "badge" : "number";
    out[ "common/trayicon" ] = mThemeTrayIcon ? "theme" : "birdtray";
    out[ "common/showunreademailcount" ] = mShowUnreadEmailCount;
    out[ "advanced/onlyShowIconOnUnreadMessages" ] = onlyShowIconOnUnreadMessages;
    out[ "common/showwhenrelevant" ] = mShowWhenRelevant;
    out[ "common/newemailEnabled" ] = mNewEmailMenuEnabled;
    out[ "common/showhidethunderbird" ] = mShowHideThunderbird;
    out[ "common/keepthunderbirdrunning" ] = mKeepThunderbirdRunning;
    out[ "common/startThunderbirdHidden" ] = mStartThunderbirdHidden;
    out[ "common/hidewhenminimized" ] = mHideWhenMinimized;
    out[ "common/exitthunderbirdonquit" ] = mExitThunderbirdWhenQuit;

    // Store the account map
    QJsonArray accounts;

    for ( const QString& path : watchedMorkFiles )
    {
        QJsonObject ac;
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
    mCountColor = QColor( settings.value( "common/countcolor" ).toString( mCountColor.name() ) );
    mCountBadge = settings.value( "common/countstyle" ).toString() == "badge";
    mThemeTrayIcon = settings.value( "common/trayicon" ).toString() == "theme";
    mShowUnreadEmailCount = settings.value( "common/showunreademailcount" ).toBool( mShowUnreadEmailCount );
    onlyShowIconOnUnreadMessages = settings.value( "advanced/onlyShowIconOnUnreadMessages" ).toBool(
            onlyShowIconOnUnreadMessages );
    mShowWhenRelevant = settings.value("common/showwhenrelevant").toBool(mShowWhenRelevant);
    mNewEmailMenuEnabled = settings.value( "common/newemailEnabled" ).toBool( mNewEmailMenuEnabled );
    mShowHideThunderbird = settings.value( "common/showhidethunderbird" ).toBool( mShowHideThunderbird );
    mKeepThunderbirdRunning = settings.value( "common/keepthunderbirdrunning" ).toBool( mKeepThunderbirdRunning );
    mStartThunderbirdHidden = settings.value( "common/startThunderbirdHidden" ).toBool( mStartThunderbirdHidden );
    mHideWhenMinimized = settings.value( "common/hidewhenminimized" ).toBool( mHideWhenMinimized );
    mExitThunderbirdWhenQuit = settings.value( "common/exitthunderbirdonquit" ).toBool( mExitThunderbirdWhenQuit );
    watchedMorkFiles.clear();

    // Convert the map into settings
    if ( settings["accounts"].isArray() )
    {
        for ( const QJsonValueRef& a : settings["accounts"].toArray() )
        {
            QString path = a.toObject().value("path").toString();

            if ( !path.isEmpty() && !watchedMorkFiles.contains( path ) )
                watchedMorkFiles.push_back( path );
        }
    }
}

void Settings::getStartThunderbirdCmdline( QString& executable, QStringList &arguments )
{
    arguments = mThunderbirdCmdLine;
    executable = arguments.takeFirst();
}

const QPixmap &Settings::getNotificationIcon() {
    // Use the icon theme's Thunderbird icon if chosen, and the Birdtray Lite icon otherwise
    // or if the theme has none
    for (const QString &name : {QString("thunderbird"), QString("org.mozilla.Thunderbird")}) {
        if (mThemeTrayIcon && mNotificationIcon.isNull() && QIcon::hasThemeIcon(name)) {
            mNotificationIcon = QIcon::fromTheme(name).pixmap(QSize(128, 128), 1.0);
        }
    }
    if (mNotificationIcon.isNull()) {
        if (!mNotificationIcon.load(":res/icons/128/io.github.vegiii.BirdtrayLite.png")) {
            Log::fatal( tr("Cannot load default system tray icon.") );
        }
    }
    return mNotificationIcon;
}

void Settings::resetNotificationIcon() {
    mNotificationIcon = QPixmap();
}
