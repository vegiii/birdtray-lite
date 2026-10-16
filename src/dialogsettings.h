#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "ui_dialogsettings.h"

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

        // Lets the user choose another Thunderbird profile
        void    changeProfile();

        void    onShowLogWindow();

    private:
        // The Thunderbird profile whose folders are shown
        QString mProfileDir;
};

#endif // SETTINGSDIALOG_H
