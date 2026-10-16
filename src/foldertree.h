#pragma once

#include <QStringList>
#include <QTreeWidget>

/**
 * Shows the mail accounts and folders of a Thunderbird profile, with a checkbox for each folder.
 */
class FolderTree : public QTreeWidget
{
    Q_OBJECT

    public:
        explicit FolderTree(QWidget* parent = nullptr);

        /**
         * Show the folders of a profile.
         *
         * @param profileDir The Thunderbird profile directory.
         * @param monitored The index files of the folders to check.
         */
        void load(const QString &profileDir, const QStringList &monitored);

        /**
         * @return The index files of the checked folders.
         */
        QStringList checkedFolders() const;

        /**
         * Check the inbox of every account and uncheck all other folders.
         */
        void checkOnlyInboxes();

        /**
         * @param folder The index file of a mail folder.
         * @return The profile directory containing the folder, or an empty string.
         */
        static QString profileOf(const QString &folder);

    signals:
        // A folder was checked or unchecked
        void foldersChanged();

    protected:
        void paintEvent(QPaintEvent* event) override;
};
