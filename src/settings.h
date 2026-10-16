#ifndef SETTINGS_H
#define SETTINGS_H

#include <QColor>
#include <QCoreApplication>
#include <QMap>
#include <QPixmap>

#include "utils.h"


class Settings
{
    public:
        Settings();

        // Color of the unread count
        QColor  mCountColor;

        // Whether to draw the unread count in a badge instead of over the whole icon
        bool    mCountBadge;

        // Whether to show the unread email count
        bool    mShowUnreadEmailCount;

        // Whether to show the tray icon only if there are unread mail messages
        bool    onlyShowIconOnUnreadMessages;

        // Whether to show a "New Email" entry in the tray menu
        bool    mNewEmailMenuEnabled;

        // The command to start Thunderbird. The first element is the executable to launch.
        QStringList mThunderbirdCmdLine;

        // The name of the Thunderbird process.
        QString mThunderbirdProcessName;

        // Thunderbird window match
        QString mThunderbirdWindowMatch;

        // Whether clicking the tray icon shows/hides Thunderbird, or starts it if it is closed
        bool    mShowHideThunderbird;

        // Whether to start Thunderbird with the app and restart it if it was closed
        bool    mKeepThunderbirdRunning;

        // The delay in seconds to launch Thunderbird
        int     mLaunchThunderbirdDelay;

        // Whether to hide the Thunderbird window when we start Thunderbird
        bool    mStartThunderbirdHidden;

        // Whether to quit Thunderbird when the app quits
        bool    mExitThunderbirdWhenQuit;

        // Enables or disables the dialog on startup that shows if no accounts were configured
        bool    showDialogIfNoAccountsConfigured;

        // Whether no settings file existed when loading the settings
        bool    isFirstRun;

        // The watched mork files in the order the user added them
        QStringList watchedMorkFiles;

        // Load and save them
        void    save();
        void    load();

        /**
         * @param executable The actual executable necessary to start (not necessary Thunderbird itself - may be shell or script).
         * @param arguments The arguments necessary to start Thunderbird.
         * @return true if executable/arguments is set, false if error
         */
        bool getStartThunderbirdCmdline( QString& executable, QStringList &arguments );

        /**
         * @return The icon to use for the system tray.
         */
        const QPixmap& getNotificationIcon();

    private:
        Q_DECLARE_TR_FUNCTIONS(Settings)

        void    fromJSON( const QJsonObject& settings );

        // Notification icon
        QPixmap     mNotificationIcon;

        // Settings filename
        QString     mSettingsFilename;
};

#endif // SETTINGS_H
