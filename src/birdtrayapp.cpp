#include <QtCore/QThread>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include "birdtrayapp.h"
#include "windowtools.h"
#include "log.h"

#define DBUS_SERVICE_NAME "io.github.vegiii.BirdtrayLite"
#define DBUS_OBJECT_PATH "/"
#define TOGGLE_THUNDERBIRD_COMMAND "toggle-tb"
#define SHOW_THUNDERBIRD_COMMAND "show-tb"
#define HIDE_THUNDERBIRD_COMMAND "hide-tb"
#define SETTINGS_COMMAND "settings"


int main(int argc, char *argv[]) {
    BirdtrayApp app(argc, argv);
    return QApplication::exec();
}

BirdtrayApp::BirdtrayApp(int &argc, char** argv) : QApplication(argc, argv)
{
    QApplication::setWindowIcon(QIcon(QString::fromUtf8(":/res/birdtray.svg")));
    QCoreApplication::setApplicationName("birdtray-lite");
    // Shown in the tray popup and window titles
    QGuiApplication::setApplicationDisplayName("Birdtray Lite");
    QGuiApplication::setDesktopFileName("io.github.vegiii.BirdtrayLite");
    // Keep running when the settings or log window is closed
    QApplication::setQuitOnLastWindowClosed(false);

    parseCmdArguments();

    if (!startSingleInstanceServer()) {
        QTimer::singleShot(0, &BirdtrayApp::quit);
        return;
    }

    Log::debug( "Birdtray Lite started" );

    ensureSystemTrayAvailable();
    // Load settings
    settings = new Settings();
    settings->load();

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

void BirdtrayApp::thunderbirdWindowAdded() {
    if (trayIcon != nullptr) {
        trayIcon->getWindowTools()->windowAdded();
    }
}

void BirdtrayApp::thunderbirdWindowHidden() {
    if (trayIcon != nullptr) {
        trayIcon->getWindowTools()->windowHidden();
    }
}

void BirdtrayApp::thunderbirdWindowShown() {
    if (trayIcon != nullptr) {
        trayIcon->getWindowTools()->windowShown();
    }
}

void BirdtrayApp::parseCmdArguments() {
    commandLineParser.setApplicationDescription(
            tr("Shows Thunderbird's unread mail count in the system tray."));
    commandLineParser.addHelpOption();
    commandLineParser.addOptions({
            {SETTINGS_COMMAND, tr("Show the settings.")},
            {{"t", TOGGLE_THUNDERBIRD_COMMAND}, tr("Toggle the Thunderbird window.")},
            {{"s", SHOW_THUNDERBIRD_COMMAND}, tr("Show the Thunderbird window.")},
            {{"H", HIDE_THUNDERBIRD_COMMAND}, tr("Hide the Thunderbird window.")},
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

    // Launched again without a command, e.g. from the app menu: show the settings
    bool noCommand = commandLineParser.optionNames().isEmpty();
    for (const char* command : {TOGGLE_THUNDERBIRD_COMMAND, SHOW_THUNDERBIRD_COMMAND,
                                HIDE_THUNDERBIRD_COMMAND, SETTINGS_COMMAND}) {
        if (commandLineParser.isSet(command)
                || (noCommand && command == QLatin1String(SETTINGS_COMMAND))) {
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
