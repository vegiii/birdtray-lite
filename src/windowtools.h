#ifndef WINDOWTOOLS_H
#define WINDOWTOOLS_H

#include <QObject>

// Finds the Thunderbird process and controls its window through KWin scripting,
// which works for Wayland-native and Flatpak Thunderbird on KDE Plasma.
class WindowTools : public QObject
{
    Q_OBJECT

    public:
        static WindowTools * create();

        // Checks whether the Thunderbird process is running. Returns true if found,
        // false if not found.
        bool    lookup();

        // Shows/activates the window
        bool    show();

        // Hides the window from the taskbar (without closing the process)
        bool    hide();

        // Is window hidden? true if yes
        bool    isHidden();

        // Closes the Thunderbird window, which quits Thunderbird
        bool    closeWindow();

    signals:
        /**
         * Called when the Thunderbird window is hidden.
         */
        void onWindowHidden();

        /**
         * Called when the Thunderbird window is shown.
         */
        void onWindowShown();

    private:
        WindowTools();

        // Run a KWin script body on the first window whose caption matches the window name pattern.
        // The matching window is available to the script as `w`.
        void    runKWinScript( const QString& action );

        // Whether we have hidden the window
        bool    mHidden;

        // Whether the Thunderbird process was found by the last lookup
        bool    mFound;
};

#endif // WINDOWTOOLS_H
