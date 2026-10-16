#pragma once

#include <QList>
#include <QString>
#include <QStringList>

/**
 * Reads the Thunderbird profile configuration to find the mail accounts and their inboxes.
 */
class Thunderbird
{
    public:
        struct Account {
            // The account name shown in Thunderbird, usually the email address
            QString name;
            // The directory containing the account's mail folders
            QString directory;
            // The index file of the account's inbox, or an empty string if there is none
            QString inbox;
        };

        /**
         * @return The directory containing profiles.ini, or an empty string if none was found.
         */
        static QString profilesDir();

        /**
         * @param profilesDir A directory containing profiles.ini.
         * @return The default profile directory listed in profiles.ini, or an empty string.
         */
        static QString defaultProfile(const QString &profilesDir);

        /**
         * @param profileDir A Thunderbird profile directory.
         * @return The mail accounts configured in the profile, without Local Folders and feeds.
         */
        static QList<Account> accounts(const QString &profileDir);

        /**
         * @return The inbox index files of all accounts in the default profile.
         */
        static QStringList inboxes();

        /**
         * @param accountDir The directory containing an account's mail folders.
         * @return The account name shown in Thunderbird, or an empty string if it is unknown.
         */
        static QString accountName(const QString &accountDir);
};
