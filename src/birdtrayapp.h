#ifndef BIRDTRAY_BIRDTRAY_APP_H
#define BIRDTRAY_BIRDTRAY_APP_H

#include <QApplication>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtCore/QCommandLineParser>
#include "settings.h"
#include "trayicon.h"


/**
 * Represents the Birdtray application.
 */
class BirdtrayApp: public QApplication {
    Q_OBJECT
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

protected Q_SLOTS:
    
    /**
     * Called when a secondary Birdtray instance attaches to this primary instance.
     */
    void onSecondInstanceAttached();

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
     * A server to handle commands from secondary Birdtray instances.
     */
    QLocalServer* singleInstanceServer = nullptr;
    
    /**
     * Command line parser containing the parsed Birdtray command line.
     */
    QCommandLineParser commandLineParser;
    
    /**
     * Parse the command line arguments given to Birdtray.
     */
    void parseCmdArguments();
    
    /**
     * Start a local server to handle requests from secondary instances of Birdtray.
     * If this succeeds, this instance of Birdtray will become the primary singleton instance.
     *
     * @return true if the server was started successfully,
     *         false if another instance of Birdtray is already running.
     */
    bool startSingleInstanceServer();
    
    /**
     * Connect to a running instance of Birdtray.
     *
     * @return true, if a connection was established successfully.
     */
    bool connectToRunningInstance() const;
    
    /**
     * Send commands indicated by the command line to the primary singleton Birdtray instance.
     *
     * @param serverSocket The socket tu use to communicate with the singleton Birdtray instance.
     */
    void sendCommandsToRunningInstance(QLocalSocket &serverSocket) const;
    
    /**
     * Called when the primary Birdtray instance receives
     * a command from a secondary Birdtray instance.
     *
     * @param clientSocket The socket to the secondary Birdtray instance.
     */
    void onSecondInstanceCommand(QLocalSocket* clientSocket);
    
    /**
     * Wait for the system tray to become available and exit if it doesn't within 60 seconds.
     */
    static void ensureSystemTrayAvailable();
};


#endif /* BIRDTRAY_BIRDTRAY_APP_H */
