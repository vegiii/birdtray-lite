#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include "windowtools.h"
#include "log.h"

// The Thunderbird process name, and the text Thunderbird's window titles end with
static const QString PROCESS_NAME = "thunderbird";
static const QString WINDOW_MATCH = "Mozilla Thunderbird";

#define KWIN_SCRIPT_NAME QString("birdtray-lite")
#define KWIN_WATCHER_NAME QString("birdtray-lite-watcher")


WindowTools::WindowTools()
    : QObject()
{
    mHidden = false;
    mFound = false;
}

WindowTools *WindowTools::create()
{
    static WindowTools * tools;

    if ( !tools )
        tools = new WindowTools();

    return tools;
}

bool WindowTools::lookup()
{
    const QStringList entries = QDir( "/proc" ).entryList( QDir::Dirs | QDir::NoDotAndDotDot );
    bool found = false;

    for ( const QString& entry : entries )
    {
        bool ok;
        entry.toInt( &ok );
        if ( !ok )
            continue;

        QFile cmdlineFile( QString("/proc/%1/cmdline").arg(entry) );
        if ( !cmdlineFile.open( QIODevice::ReadOnly ) )
            continue;

        // cmdline args are NUL-separated. We scan all args, not just argv[0],
        // because Flatpak Thunderbird runs inside bwrap, so "thunderbird"
        // only appears in a later argument.
        const QList<QByteArray> args = cmdlineFile.readAll().split('\0');
        for ( const QByteArray& arg : args )
        {
            if ( !arg.isEmpty() && QString::fromLocal8Bit(arg).contains( PROCESS_NAME, Qt::CaseInsensitive ) )
            {
                found = true;
                break;
            }
        }

        if ( found )
            break;
    }

    if ( found != mFound )
        Log::debug( found ? "Thunderbird process found" : "Thunderbird process not running" );

    // A restarted Thunderbird opens a new, visible window
    if ( !found )
        mHidden = false;

    mFound = found;
    return found;
}

bool WindowTools::show()
{
    // Restore to taskbar, un-minimize and focus
    runKWinScript(
        "w.skipTaskbar = false;\n"
        "w.skipPager = false;\n"
        "w.minimized = false;\n"
        "workspace.activeWindow = w;\n" );

    mHidden = false;
    emit onWindowShown();
    return true;
}

bool WindowTools::hide()
{
    // Minimize and remove from taskbar/pager
    runKWinScript(
        "w.skipTaskbar = true;\n"
        "w.skipPager = true;\n"
        "w.minimized = true;\n"
        "if (w.minimized && w.skipTaskbar && w.skipPager)\n"
        "    callDBus(\"io.github.vegiii.BirdtrayLite\", \"/\", \"io.github.vegiii.BirdtrayLite\", \"thunderbirdWindowHidden\");\n" );

    // The D-Bus confirmation updates mHidden; a missing window or failed script
    // must not make us believe Thunderbird was hidden.
    return true;
}

bool WindowTools::isHidden()
{
    return mHidden;
}

bool WindowTools::closeWindow()
{
    if ( !lookup() )
        return false;

    runKWinScript( "w.closeWindow();\n" );
    return true;
}

void WindowTools::startWatcher( bool hideWhenMinimized )
{
    // Tells Birdtray Lite over D-Bus when a Thunderbird window opens, and hides Thunderbird
    // when it is minimized. A window shown by something else, such as a notification,
    // gets its taskbar entry back.
    const QString script = QString(
        "var hideWhenMinimized = %1;\n"
        "var seen = {};\n"
        "function isThunderbird(w) { return w.caption.indexOf(\"%2\") >= 0; }\n"
        "function notify(method) {\n"
        "    callDBus(\"io.github.vegiii.BirdtrayLite\", \"/\", \"io.github.vegiii.BirdtrayLite\", method);\n"
        "}\n"
        "function checkAdded(w) {\n"
        "    var id = String(w.internalId);\n"
        "    if (isThunderbird(w) && !seen[id]) {\n"
        "        seen[id] = true;\n"
        "        notify(\"thunderbirdWindowAdded\");\n"
        "        if (w.minimized && w.skipTaskbar)\n"
        "            notify(\"thunderbirdWindowHidden\");\n"
        "    }\n"
        "}\n"
        "function watch(w) {\n"
        "    w.captionChanged.connect(function() { checkAdded(w); });\n"
        "    w.minimizedChanged.connect(function() {\n"
        "        if (!isThunderbird(w))\n"
        "            return;\n"
        "        if (w.minimized && hideWhenMinimized && !w.skipTaskbar) {\n"
        "            w.skipTaskbar = true;\n"
        "            w.skipPager = true;\n"
        "            notify(\"thunderbirdWindowHidden\");\n"
        "        } else if (!w.minimized && w.skipTaskbar) {\n"
        "            w.skipTaskbar = false;\n"
        "            w.skipPager = false;\n"
        "            notify(\"thunderbirdWindowShown\");\n"
        "        }\n"
        "    });\n"
        "    checkAdded(w);\n"
        "}\n"
        "workspace.windowList().forEach(watch);\n"
        "workspace.windowAdded.connect(watch);\n" ).arg( hideWhenMinimized ? "true" : "false", WINDOW_MATCH );

    loadKWinScript( KWIN_WATCHER_NAME, script );
}

void WindowTools::stopWatcher()
{
    unloadKWinScript( KWIN_WATCHER_NAME );
}

void WindowTools::windowAdded()
{
    emit onWindowAdded();
}

void WindowTools::windowHidden()
{
    mHidden = true;
    emit onWindowHidden();
}

void WindowTools::windowShown()
{
    mHidden = false;
    emit onWindowShown();
}

void WindowTools::runKWinScript( const QString& action )
{
    // This is the only reliable way to control a Wayland-native window from outside
    // the compositor on KDE Plasma.
    const QString script = QString(
        "var windows = workspace.windowList();\n"
        "for (var i = 0; i < windows.length; i++) {\n"
        "    var w = windows[i];\n"
        "    if (w.caption.indexOf(\"%1\") >= 0) {\n"
        "%2"
        "        break;\n"
        "    }\n"
        "}\n" ).arg( WINDOW_MATCH, action );

    if ( loadKWinScript( KWIN_SCRIPT_NAME, script ) )
        unloadKWinScript( KWIN_SCRIPT_NAME );
}

bool WindowTools::loadKWinScript( const QString& name, const QString& script )
{
    // KWin only loads scripts from files, so the script is written to the private runtime directory
    const QString scriptPath = QStandardPaths::writableLocation( QStandardPaths::RuntimeLocation )
                               + "/" + name + "-kwin.js";
    QFile f( scriptPath );
    if ( !f.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        Log::debug("loadKWinScript: failed to write script" );
        return false;
    }
    f.write( script.toUtf8() );
    f.close();

    // Unload a leftover script first, KWin refuses to load a name twice
    unloadKWinScript( name );

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusMessage load = QDBusMessage::createMethodCall(
        "org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "loadScript" );
    load << scriptPath << name;
    const QDBusMessage reply = bus.call( load, QDBus::Block, 2000 );

    const QList<QVariant> args = reply.arguments();
    const int scriptId = args.isEmpty() ? -1 : args.first().toInt();
    if ( reply.type() != QDBusMessage::ReplyMessage || scriptId < 0 )
    {
        Log::debug("loadKWinScript: loadScript failed: %s", qPrintable( reply.errorMessage() ) );
        QFile::remove( scriptPath );
        return false;
    }

    // KWin replies once the script has run, so it is safe to remove the file afterwards
    const QDBusMessage run = bus.call( QDBusMessage::createMethodCall(
        "org.kde.KWin", QString("/Scripting/Script%1").arg( scriptId ), "org.kde.kwin.Script", "run" ),
        QDBus::Block, 2000 );
    QFile::remove( scriptPath );
    if ( run.type() != QDBusMessage::ReplyMessage )
    {
        Log::debug("loadKWinScript: run failed: %s", qPrintable( run.errorMessage() ) );
        return false;
    }
    return true;
}

void WindowTools::unloadKWinScript( const QString& name )
{
    QDBusMessage unload = QDBusMessage::createMethodCall(
        "org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "unloadScript" );
    unload << name;
    QDBusConnection::sessionBus().call( unload, QDBus::Block, 2000 );
}
