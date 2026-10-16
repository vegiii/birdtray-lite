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

    public slots:
        void    accept() override;

        // Account buttons
        /**
         * Add one or multiple accounts.
         */
        void    accountAdd();
        void    accountAddInboxes();
        void    accountEdit();
        void    accountEditIndex( const QModelIndex& index );
        void    accountRemove();
        void    accountRemoveIndex( const QModelIndex& index );

        void    onShowLogWindow();

        void onKeyPressedOnTreeAccount(QKeyEvent * event);

    private:
        // Model to show the accounts
        ModelAccountTree    *   mAccountModel;
};

#endif // SETTINGSDIALOG_H
