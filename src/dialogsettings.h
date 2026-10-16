#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QKeyEvent>

#include "ui_dialogsettings.h"

class ModelAccountTree;

class DialogSettings : public QDialog, public Ui::DialogSettings
{
    Q_OBJECT

    public:
        explicit DialogSettings(QWidget *parent = 0);

        // Adds or removes Birdtray Lite from the programs Plasma starts at login
        static void setAutostart( bool enabled );

    public slots:
        void    accept() override;

        // Saves the settings and updates the tray icon
        void    apply();

        // Account buttons
        /**
         * Add one or multiple accounts.
         */
        void    accountAdd();
        void    accountAddInboxes();
        void    accountRemove();
        void    accountRemoveIndex( const QModelIndex& index );

        void    onShowLogWindow();

        void onKeyPressedOnTreeAccount(QKeyEvent * event);

    private:
        // Model to show the accounts
        ModelAccountTree    *   mAccountModel;
};

#endif // SETTINGSDIALOG_H
