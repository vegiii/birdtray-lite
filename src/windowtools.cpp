#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include "windowtools.h"
#include "birdtrayapp.h"
#include "settings.h"
#include "log.h"

#define KWIN_SCRIPT_NAME QString("birdtray-lite")


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
    const QString processName = BirdtrayApp::get()->getSettings()->mThunderbirdProcessName;
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
            if ( !arg.isEmpty() && QString::fromLocal8Bit(arg).contains( processName, Qt::CaseInsensitive ) )
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
    if ( mHidden )
    {
        Log::debug("Window already hidden, ignored" );
        return false;
    }

    // Minimize and remove from taskbar/pager
    runKWinScript(
        "w.skipTaskbar = true;\n"
        "w.skipPager = true;\n"
        "w.minimized = true;\n" );

    mHidden = true;
    emit onWindowHidden();
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

void WindowTools::runKWinScript( const QString& action )
{
    // This is the only reliable way to control a Wayland-native window from outside
    // the compositor on KDE Plasma. KWin only loads scripts from files, so the script
    // is written to the private runtime directory.
    const QString windowMatch = BirdtrayApp::get()->getSettings()->mThunderbirdWindowMatch.trimmed();
    QString searchStr = windowMatch.isEmpty() ? QString("Thunderbird") : windowMatch;
    searchStr.replace( "\\", "\\\\" ).replace( "\"", "\\\"" );

    const QString script = QString(
        "var windows = workspace.windowList();\n"
        "for (var i = 0; i < windows.length; i++) {\n"
        "    var w = windows[i];\n"
        "    if (w.caption.indexOf(\"%1\") >= 0) {\n"
        "%2"
        "        break;\n"
        "    }\n"
        "}\n" ).arg( searchStr, action );

    const QString scriptPath = QStandardPaths::writableLocation( QStandardPaths::RuntimeLocation )
                               + "/birdtray-lite-kwin.js";
    QFile f( scriptPath );
    if ( !f.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        Log::debug("runKWinScript: failed to write script" );
        return;
    }
    f.write( script.toUtf8() );
    f.close();

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusMessage unload = QDBusMessage::createMethodCall(
        "org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "unloadScript" );
    unload << KWIN_SCRIPT_NAME;

    // Unload a leftover script first, KWin refuses to load a name twice
    bus.call( unload, QDBus::Block, 2000 );

    QDBusMessage load = QDBusMessage::createMethodCall(
        "org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "loadScript" );
    load << scriptPath << KWIN_SCRIPT_NAME;
    const QDBusMessage reply = bus.call( load, QDBus::Block, 2000 );

    const QList<QVariant> args = reply.arguments();
    const int scriptId = args.isEmpty() ? -1 : args.first().toInt();
    if ( reply.type() != QDBusMessage::ReplyMessage || scriptId < 0 )
    {
        Log::debug("runKWinScript: loadScript failed: %s", qPrintable( reply.errorMessage() ) );
        QFile::remove( scriptPath );
        return;
    }

    // KWin replies once the script has run, so it is safe to clean up afterwards
    const QDBusMessage run = bus.call( QDBusMessage::createMethodCall(
        "org.kde.KWin", QString("/Scripting/Script%1").arg( scriptId ), "org.kde.kwin.Script", "run" ),
        QDBus::Block, 2000 );
    if ( run.type() != QDBusMessage::ReplyMessage )
        Log::debug("runKWinScript: run failed: %s", qPrintable( run.errorMessage() ) );

    bus.call( unload, QDBus::Block, 2000 );
    QFile::remove( scriptPath );
}
