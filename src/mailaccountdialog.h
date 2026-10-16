#ifndef MAIL_ACCOUNT_DIALOG_H
#define MAIL_ACCOUNT_DIALOG_H

#include <QWizard>
#include <QtCore/QDir>
#include <QtCore/QMap>
#include <QtWidgets/QTreeWidgetItem>
#include <utility>

namespace Ui {
    class MailAccountDialog;
}

/**
 * A QTreeWidget that displays a custom text if no items are in the tree.
 */
class AccountsTreeWidget : public QTreeWidget {
Q_OBJECT
Q_PROPERTY(QString emptyText READ emptyText WRITE setEmptyText)
public:
    explicit AccountsTreeWidget(QWidget* parent = nullptr, QString emptyText = QString()) :
            QTreeWidget(parent), _emptyText(std::move(emptyText)) {
    }
    
    /**
     * @param emptyText The new text that is displayed if the tree is empty.
     */
    void setEmptyText(const QString &emptyText) {
        _emptyText = emptyText;
    }
    
    /**
     * @return The text that is displayed if the tree is empty.
     */
    const QString &emptyText() const {
        return _emptyText;
    }

protected:
    /**
     * The text that is displayed if the tree is empty.
     */
    QString _emptyText;
    
    void paintEvent(QPaintEvent* event) override;
};

/**
 * A dialog that lets the user add new msf files to monitor.
 */
class MailAccountDialog : public QDialog {
Q_OBJECT
public:
    explicit MailAccountDialog(QWidget* parent = nullptr, QStringList monitoredFolders = {});
    
    ~MailAccountDialog() override;
    
    /**
     * @param outList Receives the paths to the selected msf files.
     */
    void getSelectedAccounts(QStringList &outList) const;
    
    /**
     * @param outList Receives the paths of all folders shown in the dialog that are not selected.
     */
    void getUnselectedFolders(QStringList &outList) const;
    
    void accept() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
    
    /**
     * Called when the user clicks on th button to browse for the Thunderbird profiles directory.
     */
    void onProfilesDirBrowseButtonClicked();
    
    /**
     * Called when an account item in the accounts list changed.
     * @param item The account item.
     * @param column The changed column.
     */
    static void onAccountItemChanged(QTreeWidgetItem* item, int column);

private:
    
    /**
     * Populate the list of profiles and accounts and mail folders.
     */
    void loadProfiles();
    
    /**
     * Populate the profiles tree item with the accounts and mail folders.
     *
     * @param profileTreeItem The profiles tree item to populate.
     * @param mailFolders The mail folders for the profiles, which contain the account folders.
     */
    void loadAccounts(QTreeWidgetItem* profileTreeItem, const QStringList &mailFolders);
    
    /**
     * @return An iterator over all checked account items from the accountList.
     */
    Q_ALWAYS_INLINE QTreeWidgetItemIterator iterateCheckedAccountItems() const;
    
    /**
     * Determine all mail folders in the profile directory.
     * Mail folders are folders which contain msf files.
     *
     * @param profileDirPath The path to the profile directory.
     * @return A list of absolute file paths to the mail folders.
     */
    static QStringList getMailFoldersFor(const QString &profileDirPath);
    
    /**
     * Get the name of the profile from the profile directory name.
     *
     * @param profileDirName The profile directory name.
     * @return The name of the profile.
     */
    static QString getProfileName(const QString &profileDirName);
    
    /**
     * The UI of the dialog.
     */
    Ui::MailAccountDialog* ui;
    
    const QStringList monitoredFolders;
    
    /**
     * The current directory path that contains the profiles.
     */
    QString profilesDirPath;
};

#endif // MAIL_ACCOUNT_DIALOG_H
