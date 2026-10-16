#include <QtCore/QThread>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include "birdtrayapp.h"
#include "utils.h"
#include "morkparser.h"
#include "windowtools.h"
#include "version.h"
#include "log.h"

#define DBUS_SERVICE_NAME "io.github.vegiii.BirdtrayLite"
#define DBUS_OBJECT_PATH "/"
#define TOGGLE_THUNDERBIRD_COMMAND "toggle-tb"
#define SHOW_THUNDERBIRD_COMMAND "show-tb"
#define HIDE_THUNDERBIRD_COMMAND "hide-tb"
#define SETTINGS_COMMAND "settings"


BirdtrayApp::BirdtrayApp(int &argc, char** argv) : QApplication(argc, argv)
{
    QApplication::setWindowIcon(QIcon(QString::fromUtf8(":/res/birdtray.svg")));
    QCoreApplication::setApplicationName("birdtray-lite");
    QGuiApplication::setDesktopFileName("io.github.vegiii.BirdtrayLite");
    QCoreApplication::setApplicationVersion(Utils::getBirdtrayVersion());
    // This prevents exiting the application when the dialogs are closed on Gnome/XFCE
    QApplication::setQuitOnLastWindowClosed(false);

    parseCmdArguments();
    
    QString morkPath = commandLineParser.value("dump-mork");
    if (!morkPath.isEmpty()) {
        QTimer::singleShot(0, [=]() { exit(MorkParser::dumpMorkFile(morkPath)); });
        return;
    }
    QString imapString = commandLineParser.value("decode");
    if (!imapString.isEmpty()) {
        printf("Decoded: %s\n", qPrintable(Utils::decodeIMAPutf7(imapString)));
        QTimer::singleShot(0, &BirdtrayApp::quit);
        return;
    }
    
    if (!startSingleInstanceServer()) {
        QTimer::singleShot(0, &BirdtrayApp::quit);
        return;
    }
    
    Log::initialize(commandLineParser.value("log"));

    Log::debug( "Birdtray Lite version %d.%d.%d started", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH );

    ensureSystemTrayAvailable();
    // Load settings
    settings = new Settings();
    if (commandLineParser.isSet("reset-settings")) {
        settings->save(); // Saving without loading will reset the values
    } else {
        settings->load();
    }

    trayIcon = new TrayIcon(commandLineParser.isSet("settings"));
}

BirdtrayApp::~BirdtrayApp() {
    delete trayIcon;
    delete settings;
}

BirdtrayApp* BirdtrayApp::get() {
    return dynamic_cast<BirdtrayApp*>(QCoreApplication::instance());
}

Settings* BirdtrayApp::getSettings() const {
    return settings;
}

TrayIcon* BirdtrayApp::getTrayIcon() const {
    return trayIcon;
}

void BirdtrayApp::runCommand(const QString &command) {
    if (trayIcon == nullptr) {
        return;
    }
    if (command == TOGGLE_THUNDERBIRD_COMMAND) {
        if (trayIcon->getWindowTools()->isHidden()) {
            trayIcon->showThunderbird();
        } else {
            trayIcon->hideThunderbird();
        }
    } else if (command == SHOW_THUNDERBIRD_COMMAND) {
        trayIcon->showThunderbird();
    } else if (command == HIDE_THUNDERBIRD_COMMAND) {
        trayIcon->hideThunderbird();
    } else if (command == SETTINGS_COMMAND) {
        trayIcon->showSettings();
    }
}

void BirdtrayApp::parseCmdArguments() {
    commandLineParser.setApplicationDescription(
            tr("A free system tray notification for new mail for Thunderbird."));
    commandLineParser.addHelpOption();
    commandLineParser.addVersionOption();
    commandLineParser.addOptions({
            {"dump-mork", tr("Display the contents of the given mork database."),
             tr("databaseFile")},
            {"decode", tr("Decode an IMAP Utf7 string."), tr("string")},
            {SETTINGS_COMMAND, tr("Show the settings.")},
            {{"t", TOGGLE_THUNDERBIRD_COMMAND}, tr("Toggle the Thunderbird window.")},
            {{"s", SHOW_THUNDERBIRD_COMMAND}, tr("Show the Thunderbird window.")},
            {{"H", HIDE_THUNDERBIRD_COMMAND}, tr("Hide the Thunderbird window.")},
            {{"r", "reset-settings"}, tr("Reset the settings to the defaults.")},
            {{"l", "log"}, tr("Write log to a file."), tr("file")}
    });
    commandLineParser.process(*this);
}

bool BirdtrayApp::startSingleInstanceServer() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qWarning("No D-Bus session bus, running without single instance check");
        return true;
    }
    bus.registerObject(DBUS_OBJECT_PATH, this, QDBusConnection::ExportScriptableSlots);
    if (bus.registerService(DBUS_SERVICE_NAME)) {
        return true;
    }
    bus.unregisterObject(DBUS_OBJECT_PATH);

    for (const char* command : {TOGGLE_THUNDERBIRD_COMMAND, SHOW_THUNDERBIRD_COMMAND,
                                HIDE_THUNDERBIRD_COMMAND, SETTINGS_COMMAND}) {
        if (commandLineParser.isSet(command)) {
            QDBusMessage message = QDBusMessage::createMethodCall(
                    DBUS_SERVICE_NAME, DBUS_OBJECT_PATH, DBUS_SERVICE_NAME, "runCommand");
            message << QString(command);
            bus.call(message);
        }
    }
    return false;
}

void BirdtrayApp::ensureSystemTrayAvailable() {
    int passed = 0;
    while (!QSystemTrayIcon::isSystemTrayAvailable()) {
        if (passed == 0) {
            qDebug("Waiting for system tray to become available");
        }
        passed++;
        if (passed > 120) {
            Log::fatal(tr("Sorry, the system tray cannot be controlled "
                          "by this add-on on your operating system."));
        }

        // Let the Qt event loop recognize the tray appearing
        QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

        QThread::msleep(500);
    }
}
