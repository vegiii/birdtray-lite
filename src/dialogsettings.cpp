#include <Qt>

#include <QFileDialog>
#include <QMessageBox>
#include <QDate>
#include <QDir>
#include <QLocale>
#include <QPainter>
#include <QRegularExpression>
#include <QtCore/QStandardPaths>

#include "dialogsettings.h"
#include "utils.h"
#include "thunderbird.h"
#include "birdtrayapp.h"
#include "log.h"

struct CountColor
{
    QString name;
    QColor color;
};

// Colors that stay readable both as a bold number and as a badge
static QList<CountColor> countColors()
{
    return {
        { QObject::tr("White"), QColor( "#ffffff" ) },
        { QObject::tr("Yellow"), QColor( "#fdd835" ) },
        { QObject::tr("Orange"), QColor( "#ff9800" ) },
        { QObject::tr("Red"), QColor( "#f44336" ) },
        { QObject::tr("Green"), QColor( "#4caf50" ) },
        { QObject::tr("Blue"), QColor( "#3daee9" ) },
    };
}

// Plasma starts the desktop files in this directory at login
static QString autostartFilePath()
{
    return QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation )
           + "/autostart/io.github.vegiii.BirdtrayLite.desktop";
}

void DialogSettings::setAutostart( bool enabled )
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
    connect( buttonBox->button( QDialogButtonBox::Apply ), &QPushButton::clicked, this, &DialogSettings::apply );

    connect( btnProfileChange, &QPushButton::clicked, this, &DialogSettings::changeProfile );
    connect( btnOnlyInboxes, &QPushButton::clicked, folderTree, &FolderTree::checkOnlyInboxes );
    connect( btnShowLogWindow, &QPushButton::clicked, this, &DialogSettings::onShowLogWindow );

    // General tab
    boxAutostart->setChecked( QFile::exists( autostartFilePath() ) );
    boxKeepThunderbirdRunning->setChecked( settings->mKeepThunderbirdRunning );
    boxStartThunderbirdHidden->setChecked( settings->mStartThunderbirdHidden );
    boxShowHideThunderbird->setChecked( settings->mShowHideThunderbird );
    boxStopThunderbirdOnExit->setChecked( settings->mExitThunderbirdWhenQuit );
    boxShowUnreadCount->setChecked( settings->mShowUnreadEmailCount );
    comboCountStyle->setCurrentIndex( settings->mCountBadge ? 1 : 0 );
    // The preset colors, each shown with a colored dot. A color from an older config gets the closest preset.
    int closest = 0;
    int closestDistance = INT_MAX;
    for ( const CountColor& preset : countColors() )
    {
        QPixmap dot( 16, 16 );
        dot.fill( Qt::transparent );
        QPainter painter( &dot );
        painter.setRenderHint( QPainter::Antialiasing );
        painter.setPen( QColor( 0, 0, 0, 90 ) );
        painter.setBrush( preset.color );
        painter.drawEllipse( QRectF( 1.5, 1.5, 13, 13 ) );
        painter.end();
        comboCountColor->addItem( QIcon( dot ), preset.name, preset.color );

        int distance = qAbs( preset.color.red() - settings->mCountColor.red() )
                + qAbs( preset.color.green() - settings->mCountColor.green() )
                + qAbs( preset.color.blue() - settings->mCountColor.blue() );
        if ( distance < closestDistance )
        {
            closest = comboCountColor->count() - 1;
            closestDistance = distance;
        }
    }
    comboCountColor->setCurrentIndex( closest );
    onlyShowIconOnNewMail->setChecked( settings->onlyShowIconOnUnreadMessages );
    boxEnableNewEmail->setChecked( settings->mNewEmailMenuEnabled );

    // Monitoring tab: use the profile of the monitored folders, or Thunderbird's default profile
    if ( !settings->watchedMorkFiles.isEmpty() )
        mProfileDir = FolderTree::profileOf( settings->watchedMorkFiles.first() );
    if ( mProfileDir.isEmpty() )
        mProfileDir = Thunderbird::defaultProfile( Thunderbird::profilesDir() );
    editProfile->setText( mProfileDir );
    folderTree->load( mProfileDir, settings->watchedMorkFiles );

    // Apply is enabled once something changes
    QPushButton* applyButton = buttonBox->button( QDialogButtonBox::Apply );
    applyButton->setEnabled( false );
    auto markChanged = [applyButton]() { applyButton->setEnabled( true ); };
    for ( QCheckBox* box : tabGeneral->findChildren<QCheckBox*>() )
        connect( box, &QCheckBox::toggled, this, markChanged );
    connect( comboCountStyle, &QComboBox::currentIndexChanged, this, markChanged );
    connect( comboCountColor, &QComboBox::currentIndexChanged, this, markChanged );
    connect( folderTree, &FolderTree::foldersChanged, this, markChanged );

    // About tab
    QString settingsFile = QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation ) + "/config.json";
    settingsFile.replace( QRegularExpression( "^" + QRegularExpression::escape( QDir::homePath() ) ), "~" );
    labelTitle->setText( tr("Birdtray Lite %1").arg( Utils::getBirdtrayVersion() ) );
    QFont titleFont = labelTitle->font();
    titleFont.setBold( true );
    titleFont.setPointSizeF( titleFont.pointSizeF() * 1.4 );
    labelTitle->setFont( titleFont );
    labelSettings->setText( settingsFile );
    labelBuilt->setText( tr("%1 with Qt %2").arg( QLocale().toString( QDate::fromString( QString(__DATE__).simplified(), "MMM d yyyy" ), "d MMMM yyyy" ), QT_VERSION_STR ) );

    // The detail names are dimmed like in KDE's own About pages
    for ( QLabel* name : { labelSourceName, labelSettingsName, labelBuiltName } )
    {
        QPalette palette = name->palette();
        palette.setColor( QPalette::WindowText, palette.color( QPalette::PlaceholderText ) );
        name->setPalette( palette );
    }
}

void DialogSettings::accept()
{
    apply();
    QDialog::accept();
}

void DialogSettings::apply()
{
    Settings* settings = BirdtrayApp::get()->getSettings();

    if ( boxAutostart->isChecked() != QFile::exists( autostartFilePath() ) )
        setAutostart( boxAutostart->isChecked() );
    settings->mKeepThunderbirdRunning = boxKeepThunderbirdRunning->isChecked();
    settings->mStartThunderbirdHidden = boxStartThunderbirdHidden->isChecked();
    settings->mShowHideThunderbird = boxShowHideThunderbird->isChecked();
    settings->mExitThunderbirdWhenQuit = boxStopThunderbirdOnExit->isChecked();
    settings->mShowUnreadEmailCount = boxShowUnreadCount->isChecked();
    settings->mCountBadge = comboCountStyle->currentIndex() == 1;
    settings->mCountColor = comboCountColor->currentData().value<QColor>();
    settings->onlyShowIconOnUnreadMessages = onlyShowIconOnNewMail->isChecked();
    settings->mNewEmailMenuEnabled = boxEnableNewEmail->isChecked();
    settings->watchedMorkFiles = folderTree->checkedFolders();

    BirdtrayApp::get()->getTrayIcon()->applySettings();
    buttonBox->button( QDialogButtonBox::Apply )->setEnabled( false );
}

void DialogSettings::changeProfile()
{
    QString dir = QFileDialog::getExistingDirectory( this, tr("Choose the Thunderbird profile"), mProfileDir );
    if ( dir.isEmpty() || dir == mProfileDir )
        return;

    // Start with the inboxes of the new profile
    mProfileDir = dir;
    editProfile->setText( mProfileDir );
    folderTree->load( mProfileDir, {} );
    folderTree->checkOnlyInboxes();
    buttonBox->button( QDialogButtonBox::Apply )->setEnabled( true );
}

void DialogSettings::onShowLogWindow()
{
    Log::showLogger();
}
