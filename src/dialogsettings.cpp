#include <Qt>

#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QtCore/QStandardPaths>

#include "dialogsettings.h"
#include "modelaccounttree.h"
#include "utils.h"
#include "mailaccountdialog.h"
#include "thunderbird.h"
#include "birdtrayapp.h"
#include "log.h"

// Plasma starts the desktop files in this directory at login
static QString autostartFilePath()
{
    return QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation )
           + "/autostart/io.github.vegiii.BirdtrayLite.desktop";
}

static void setAutostart( bool enabled )
{
    const QString path = autostartFilePath();

    if ( !enabled )
    {
        QFile::remove( path );
        return;
    }

    QDir().mkpath( QFileInfo( path ).absolutePath() );
    QFile file( path );
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        QMessageBox::warning( nullptr, QObject::tr("Autostart"),
                QObject::tr("Could not write %1:\n%2").arg( path, file.errorString() ) );
        return;
    }

    file.write( QString(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Birdtray Lite\n"
        "Exec=%1\n"
        "Icon=io.github.vegiii.BirdtrayLite\n" ).arg( QCoreApplication::applicationFilePath() ).toUtf8() );
}

DialogSettings::DialogSettings( QWidget *parent)
    : QDialog(parent), Ui::DialogSettings()
{
    setupUi(this);
    Settings* settings = BirdtrayApp::get()->getSettings();

    // Show the first tab
    tabWidget->setCurrentIndex( 0 );

    connect( buttonBox, &QDialogButtonBox::accepted, this, &DialogSettings::accept );
    connect( buttonBox, &QDialogButtonBox::rejected, this, &DialogSettings::reject );

    connect( treeAccounts, &QTreeView::doubleClicked, this, &DialogSettings::accountEditIndex  );
    connect( treeAccounts, &QTreeViewWithKeyEvents::onKeyPressed, this, &DialogSettings::onKeyPressedOnTreeAccount );

    connect( btnAccountAdd, &QPushButton::clicked, this, &DialogSettings::accountAdd );
    connect( btnAccountAddInboxes, &QPushButton::clicked, this, &DialogSettings::accountAddInboxes );
    connect( btnAccountEdit, &QPushButton::clicked, this, &DialogSettings::accountEdit );
    connect( btnAccountRemove, &QPushButton::clicked, this, &DialogSettings::accountRemove );

    connect( btnShowLogWindow, &QPushButton::clicked, this, &DialogSettings::onShowLogWindow );

    // General tab
    boxAutostart->setChecked( QFile::exists( autostartFilePath() ) );
    boxLaunchThunderbirdAtStart->setChecked( settings->mLaunchThunderbird );
    boxRestartThunderbird->setChecked( settings->mRestartThunderbird );
    boxStartThunderbirdOnTrayIconClick->setChecked( settings->startClosedThunderbird );
    boxStartThunderbirdHidden->setChecked( settings->mStartThunderbirdHidden );
    boxShowHideThunderbird->setChecked( settings->mShowHideThunderbird );
    boxStopThunderbirdOnExit->setChecked( settings->mExitThunderbirdWhenQuit );
    boxMonitorThunderbirdWindow->setChecked( settings->mMonitorThunderbirdWindow );
    boxShowUnreadCount->setChecked( settings->mShowUnreadEmailCount );
    btnNotificationColor->setColor( settings->mNotificationDefaultColor );
    boxBlink->setChecked( settings->mBlinkSpeed > 0 );
    onlyShowIconOnNewMail->setChecked( settings->onlyShowIconOnUnreadMessages );
    boxEnableNewEmail->setChecked( settings->mNewEmailMenuEnabled );

    // Accounts tab
    mAccountModel = new ModelAccountTree(this, treeAccounts);
    treeAccounts->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeAccounts->setCurrentIndex(mAccountModel->index(0, 0));

    // Create the "About" box
    QString origabout = browserAbout->toHtml();
    origabout.replace( "[VERSION]", Utils::getBirdtrayVersion() );
    origabout.replace( "[DATE]", QString("%1 %2").arg(__DATE__) .arg(__TIME__) );
    origabout.replace( "[QT_VERSION]", QT_VERSION_STR );
    browserAbout->setText( origabout );
}

void DialogSettings::onKeyPressedOnTreeAccount(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        accountRemove();
    }
}

void DialogSettings::accept()
{
    Settings* settings = BirdtrayApp::get()->getSettings();

    if ( boxAutostart->isChecked() != QFile::exists( autostartFilePath() ) )
        setAutostart( boxAutostart->isChecked() );
    settings->mLaunchThunderbird = boxLaunchThunderbirdAtStart->isChecked();
    settings->mRestartThunderbird = boxRestartThunderbird->isChecked();
    settings->startClosedThunderbird = boxStartThunderbirdOnTrayIconClick->isChecked();
    settings->mStartThunderbirdHidden = boxStartThunderbirdHidden->isChecked();
    settings->mShowHideThunderbird = boxShowHideThunderbird->isChecked();
    settings->mExitThunderbirdWhenQuit = boxStopThunderbirdOnExit->isChecked();
    settings->mMonitorThunderbirdWindow = boxMonitorThunderbirdWindow->isChecked();
    settings->mShowUnreadEmailCount = boxShowUnreadCount->isChecked();
    settings->mNotificationDefaultColor = btnNotificationColor->color();
    // Keep a custom blinking speed from an old config, otherwise blink every 500 ms
    if ( !boxBlink->isChecked() )
        settings->mBlinkSpeed = 0;
    else if ( settings->mBlinkSpeed == 0 )
        settings->mBlinkSpeed = 10;
    settings->onlyShowIconOnUnreadMessages = onlyShowIconOnNewMail->isChecked();
    settings->mNewEmailMenuEnabled = boxEnableNewEmail->isChecked();

    mAccountModel->applySettings();

    QDialog::accept();
}

void DialogSettings::accountAdd()
{
    if ( QGuiApplication::keyboardModifiers() & Qt::ShiftModifier &&
               QGuiApplication::keyboardModifiers() & Qt::ControlModifier )
    {
        QStringList files = QFileDialog::getOpenFileNames(
                nullptr, tr("Choose one or more MSF files"), "", tr("Mail Index (*.msf)"));
        if (files.isEmpty()) {
            return;
        }
        for (const QString &file : files) {
            mAccountModel->addAccount(file, btnNotificationColor->color());
        }
    }
    else
    {
        QMap<QString, QColor> monitored;
        for (int i = 0; i < mAccountModel->accounts().size(); i++) {
            monitored.insert(mAccountModel->accounts()[i], mAccountModel->colors()[i]);
        }
        MailAccountDialog accountDialog(this, btnNotificationColor->color(), monitored);
        if (accountDialog.exec() != QDialog::Accepted) {
            return;
        }

        // Folders that were unchecked in the dialog are no longer monitored
        QStringList unselected;
        accountDialog.getUnselectedFolders(unselected);
        for (const QString &folder : unselected) {
            int row = mAccountModel->accounts().indexOf(folder);
            if (row != -1) {
                mAccountModel->removeAccount(mAccountModel->index(row, 0));
            }
        }

        QString path;
        QColor color;
        QList<std::tuple<QString, QColor>> accountInfoList;
        accountDialog.getSelectedAccounts(accountInfoList);
        for (const std::tuple<QString, QColor> &accountInfo : accountInfoList) {
            std::tie(path, color) = accountInfo;
            mAccountModel->addAccount(path, color);
        }
    }

    treeAccounts->setCurrentIndex(mAccountModel->index(mAccountModel->rowCount() - 1, 0));
}

void DialogSettings::accountAddInboxes()
{
    const QStringList inboxes = Thunderbird::inboxes();
    int added = 0;
    for (const QString &inbox : inboxes) {
        if (!mAccountModel->accounts().contains(inbox)) {
            mAccountModel->addAccount(inbox, btnNotificationColor->color());
            added++;
        }
    }

    if (inboxes.isEmpty()) {
        QMessageBox::information(this, tr("Add inboxes"),
                tr("No Thunderbird inboxes were found. Use Add to choose the folders manually."));
    } else if (added == 0) {
        QMessageBox::information(this, tr("Add inboxes"), tr("All inboxes are already monitored."));
    } else {
        treeAccounts->setCurrentIndex(mAccountModel->index(mAccountModel->rowCount() - 1, 0));
    }
}

void DialogSettings::accountEdit()
{
    if ( treeAccounts->currentIndex().isValid() )
        accountEditIndex( treeAccounts->currentIndex() );
}

void DialogSettings::accountEditIndex(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    QString path;
    QColor color;
    mAccountModel->getAccount(index, path, color);
    color = QColorDialog::getColor(color, this);
    if (color.isValid()) {
        mAccountModel->editAccount(index, path, color);
    }
}

void DialogSettings::accountRemove()
{
    if ( treeAccounts->currentIndex().isValid() )
        accountRemoveIndex( treeAccounts->currentIndex() );
}


void DialogSettings::accountRemoveIndex(const QModelIndex &index)
{
    if ( !index.isValid() )
        return;

    mAccountModel->removeAccount( index );
}

void DialogSettings::onShowLogWindow()
{
    Log::showLogger();
}
