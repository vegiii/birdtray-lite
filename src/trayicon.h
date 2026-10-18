#ifndef TRAYICON_H
#define TRAYICON_H

#include <QTimer>
#include <QPointer>
#include <QWidget>
#include <QImage>
#include <QProcess>
#include <QSystemTrayIcon>
#include "dialogsettings.h"

class UnreadMonitor;
class WindowTools;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

    public:
        explicit TrayIcon(bool showSettings);
        ~TrayIcon() override;
        
        /**
         * @return The window tools used by the tray icon.
         */
        WindowTools* getWindowTools() const;
    
        /**
         * @return The unread monitor holding information about the watched mail accounts.
         */
        UnreadMonitor* getUnreadMonitor() const;
    
        /**
         * Hide the Thunderbird window.
         */
        void hideThunderbird();
        
        /**
         * Show the Thunderbird window.
         */
        void showThunderbird();

        /**
         * Save the settings and update the tray icon and menu.
         */
        void applySettings();

    signals:
        void    settingsChanged();

    public slots:
        void    unreadCounterUpdate( const QMap<QString, quint32>& counts );

        /**
         * The warning status of a watched path in the unread monitor changed.
         *
         * @param path The path whose warning has changed or a null-string for a global warning.
         */
        void unreadMonitorWarningChanged(const QString &path);
        
        /**
         * Show the settings dialog.
         */
        void showSettings();


    private slots:
        // Updates the icon
        void    updateIcon();

        // Checks the application current state
        void    updateState();

        // Context menu actions
        void    actionActivate();
        void    actionNewEmail();

        void    actionSystrayIconActivated( QSystemTrayIcon::ActivationReason reason );

        void    startThunderbird();
        
        /**
         * Callback if Thunderbird fails to start.
         * @param error The reason for the start failure.
         */
        void    tbProcessError( QProcess::ProcessError error);
        void    tbProcessFinished( int exitCode, QProcess::ExitStatus exitStatus );
        /**
         * Callback that is called when we are about to quit.
         */
        void    onQuit();
        
        /**
         * Called when the Thunderbird window is shown.
         */
        void    onThunderbirdWindowShown();
        
        /**
         * Called when the Thunderbird window is hidden.
         */
        void    onThunderbirdWindowHidden();

        /**
         * Called when a Thunderbird window opens.
         */
        void    onThunderbirdWindowAdded();

    private:
        void    updateTrayStatus(bool relevant);
        QPointer<QObject> mNotifier;
        void    createMenu();
        void    createUnreadCounterThread();
        
        /**
         * Draw the warning indicator.
         * @param painter The painter to use when drawing.
         * @param iconSize The size of the icon image to draw on in pixel.
         */
        static void drawWarningIndicator(QPainter &painter, const QSize &iconSize);

        /**
         * Draw the unread count in bold over the icon.
         * @param painter The painter to use when drawing.
         * @param iconSize The size of the icon image to draw on in pixel.
         * @param count The unread count.
         * @param color The color of the number.
         */
        static void drawCount(QPainter &painter, const QSize &iconSize, unsigned int count,
                              const QColor &color);

        /**
         * Draw the unread count in a badge in the top right corner.
         * @param painter The painter to use when drawing.
         * @param iconSize The size of the icon image to draw on in pixel.
         * @param count The unread count.
         * @param color The badge color.
         */
        static void drawCountBadge(QPainter &painter, const QSize &iconSize, unsigned int count,
                                   const QColor &color);

        // Current unread messages count, in total and per monitored folder
        unsigned int    mUnreadCounter;
        QMap<QString, quint32> mUnreadCounts;

        // Show/hide Thunderbird menu item (we modify its text)
        QAction *       mMenuShowHideThunderbird;


        // Unread counter thread
        UnreadMonitor * mUnreadMonitor = nullptr;

        // State checking timer (once a second)
        QTimer          mStateTimer;

        // If true, Thunderbird window exists right now
        bool            mThunderbirdWindowExists;

        // Startup hiding stays pending until confirmed, cancelled, or timed out
        QTimer          mStartupHideTimer;

        // Window tools (show/hide)
        WindowTools *   mWinTools;

        // Cached last drawn icon
        QImage          mLastDrawnIcon;

        // Thunderbird process which we have started. This can be nullptr if Thunderbird
        // was started before Birdtray (thus our process would just activate it and exit)
        // Thus checking this pointer for null doesn't mean TB is not started.
        QProcess    *   mThunderbirdProcess;

        // System tray context menu. Once set, it remains there, so we have to modify existing one
        QMenu       *   mSystrayMenu;
    
        /**
         * The currently opened settings dialog.
         */
        DialogSettings* settingsDialog = nullptr;
};

#endif // TRAYICON_H
