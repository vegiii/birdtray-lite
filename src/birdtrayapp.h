#ifndef BIRDTRAY_BIRDTRAY_APP_H
#define BIRDTRAY_BIRDTRAY_APP_H

#include <QApplication>
#include <QtCore/QCommandLineParser>
#include "settings.h"
#include "trayicon.h"


/**
 * Represents the Birdtray application.
 */
class BirdtrayApp: public QApplication {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.vegiii.BirdtrayLite")
public:
    
    /**
     * Creates a new Birdtray application.
     *
     * @param argc The number of command line arguments.
     * @param argv The command line arguments.
     */
    BirdtrayApp(int &argc, char** argv);
    
    ~BirdtrayApp() override;
    
    /**
     * @return The main Birdtray app instance.
     */
    static BirdtrayApp* get();
    
    /**
     * @return The Birdtray settings.
     */
    Settings* getSettings() const;
    
    /**
     * @return The tray icon.
     */
    TrayIcon* getTrayIcon() const;

public Q_SLOTS:

    /**
     * Called over D-Bus when a secondary Birdtray Lite instance sends a command.
     *
     * @param command The command given on the secondary instance's command line.
     */
    Q_SCRIPTABLE void runCommand(const QString &command);

    /**
     * Called over D-Bus by the KWin watcher script when a Thunderbird window opens.
     */
    Q_SCRIPTABLE void thunderbirdWindowAdded();

    /**
     * Called over D-Bus by the KWin watcher script when it hid the minimized Thunderbird window.
     */
    Q_SCRIPTABLE void thunderbirdWindowHidden();

    /**
     * Called over D-Bus by the KWin watcher script when the hidden Thunderbird window was shown.
     */
    Q_SCRIPTABLE void thunderbirdWindowShown();

private:
    /**
     * The Birdtray settings.
     */
    Settings* settings = nullptr;
    
    /**
     * The system tray icon.
     */
    TrayIcon* trayIcon = nullptr;
    
    /**
     * Command line parser containing the parsed Birdtray command line.
     */
    QCommandLineParser commandLineParser;
    
    /**
     * Parse the command line arguments given to Birdtray.
     */
    void parseCmdArguments();
    
    /**
     * Register the D-Bus service that makes this the primary Birdtray Lite instance.
     * If another instance already owns it, the command line commands are sent to that instance.
     *
     * @return true if this is now the primary instance,
     *         false if another instance of Birdtray Lite is already running.
     */
    bool startSingleInstanceServer();
    
    /**
     * Wait for the system tray to become available and exit if it doesn't within 60 seconds.
     */
    static void ensureSystemTrayAvailable();
};


#endif /* BIRDTRAY_BIRDTRAY_APP_H */
