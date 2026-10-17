#pragma once

#include <QMap>
#include <QString>
#include <QtCore/QFileInfo>

class Utils
{
    public:
        /**
         * Expand the path as a shell would do. This will expand variables and ~/.
         * This function will also remove wrapping quotes.
         *
         * @param path The path.
         * @return The expanded path.
         */
        static QString expandPath(const QString& path);
        
        /**
         * @return A list of possible locations of the directory
         *         that contains the Thunderbird profiles.
         * @see https://support.mozilla.org/en-US/kb/profiles-where-thunderbird-stores-user-data#w_profile-location-summary
         */
        static QStringList getThunderbirdProfilesPaths();

        /**
         * @return The default Thunderbird command for the current platform.
         */
        static QStringList getDefaultThunderbirdCommand();
        
        /**
         * Get the mail folder name from the mork path.
         *
         * @param morkPath The file info for a msf file.
         * @return The mail folder name or a null string.
         */
        static QString getMailFolderName(const QFileInfo &morkFile);
        
        /**
         * Get the mil account name from a mork path
         *
         * @param morkPath The file info for a msf file.
         * @return The mail account name or a null string.
         */
        static QString getMailAccountName(const QFileInfo &morkFile);
};

