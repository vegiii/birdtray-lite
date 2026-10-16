#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QProgressDialog>
#include <QPalette>
#include <QtCore/QStringListModel>

#include "ui_dialogsettings.h"

class ModelAccountTree;
class ModelNewEmails;

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
        void    accountEdit();
        void    accountEditIndex( const QModelIndex& index );
        void    accountRemove();
        void    accountRemoveIndex( const QModelIndex& index );

        // New Email buttons
        void    newEmailAdd();
        void    newEmailEdit();
        void    newEmailEditIndex( const QModelIndex& index );
        void    newEmailRemove();
        void    newEmailRemoveIndex( const QModelIndex& index );
        
        // Advanced buttons
        void    onShowLogWindow();

        // Icon change
        void    buttonChangeIcon();
        void    buttonChangeUnreadIcon();
        
        /**
         * The unread count border width changed.
         * @param value The new border width.
         */
        void onBorderWidthChanged(int value);

        /**
         * The maximum font size changed.
         * @param value The new maximum font size.
         */
        void onMaximumFontSizeChanged(int value);
        
        /**
         * The minimum font size changed.
         * @param value The new minimum font size.
         */
        void onMinimumFontSizeChanged(int value);

        void onKeyPressedOnTreeAccount(QKeyEvent * event);

    private:
        void    changeIcon(QToolButton * button );

        // Model to show the accounts
        ModelAccountTree    *   mAccountModel;

        // Model to show new emails
        ModelNewEmails      *   mModelNewEmails;
};

#endif // SETTINGSDIALOG_H
