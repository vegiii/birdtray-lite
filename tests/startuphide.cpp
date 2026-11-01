#include <QEventLoop>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>

#include "birdtrayapp.h"
#include "windowtools.h"

// Replace application startup and KWin access so the real TrayIcon can be tested
// without a desktop session, launching Thunderbird, or loading personal settings.
BirdtrayApp::BirdtrayApp(int &argc, char **argv) : QApplication(argc, argv)
{
    setQuitOnLastWindowClosed(false);
    settings = new Settings();
    settings->mKeepThunderbirdRunning = false;
    settings->mExitThunderbirdWhenQuit = false;
}
BirdtrayApp::~BirdtrayApp() { delete settings; }
BirdtrayApp *BirdtrayApp::get() { return static_cast<BirdtrayApp *>(instance()); }
Settings *BirdtrayApp::getSettings() const { return settings; }
TrayIcon *BirdtrayApp::getTrayIcon() const { return trayIcon; }
void BirdtrayApp::runCommand(const QString &) {}
void BirdtrayApp::thunderbirdWindowAdded() {}
void BirdtrayApp::thunderbirdWindowHidden() {}
void BirdtrayApp::thunderbirdWindowShown() {}

static int hideRequests = 0;
WindowTools::WindowTools() : mHidden(false), mFound(true) {}
WindowTools *WindowTools::create() { static WindowTools tools; return &tools; }
bool WindowTools::lookup() { return true; }
bool WindowTools::hide() { ++hideRequests; windowHidden(); return true; }
bool WindowTools::show() { windowShown(); return true; }
bool WindowTools::isHidden() { return mHidden; }
bool WindowTools::closeWindow() { return true; }
void WindowTools::startWatcher(bool) {}
void WindowTools::stopWatcher() {}
void WindowTools::windowAdded() { emit onWindowAdded(); }
void WindowTools::windowHidden() { mHidden = true; emit onWindowHidden(); }
void WindowTools::windowShown() { mHidden = false; emit onWindowShown(); }

static void check(bool passed, const char *message)
{
    if (!passed) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void tick(TrayIcon &tray)
{
    check(QMetaObject::invokeMethod(&tray, "updateState", Qt::DirectConnection),
          "updateState must be invokable");
}

int main(int argc, char **argv)
{
    QTemporaryDir config;
    check(config.isValid(), "temporary config directory");
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    BirdtrayApp app(argc, argv);
    WindowTools *windows = WindowTools::create();

    {
        TrayIcon tray(false);
        check(windows->isHidden(), "hide an already-running Thunderbird at startup");
        int before = hideRequests;
        tick(tray);
        check(hideRequests > before, "hide confirmation must not stop startup retries");

        // Reproduce Thunderbird restoring its window after the first hide.
        windows->windowShown();
        before = hideRequests;
        tick(tray);
        check(hideRequests > before && windows->isHidden(),
              "hide Thunderbird again after a startup restore");

        before = hideRequests;
        windows->windowAdded();
        check(hideRequests > before, "hide additional startup windows");

        tray.showThunderbird();
        before = hideRequests;
        windows->windowAdded();
        tick(tray);
        check(hideRequests == before && !windows->isHidden(),
              "explicit Show must cancel startup hiding");
    }

    {
        TrayIcon tray(false);
        // Exercise the real 30-second timer, including repeated confirmations.
        QEventLoop loop;
        QTimer::singleShot(32000, Qt::PreciseTimer, &loop, &QEventLoop::quit);
        loop.exec();
        windows->windowShown();
        int before = hideRequests;
        windows->windowAdded();
        tick(tray);
        check(hideRequests == before && !windows->isHidden(),
              "startup hiding must expire after 30 seconds");
    }

    app.getSettings()->mStartThunderbirdHidden = false;
    {
        int before = hideRequests;
        TrayIcon tray(false);
        windows->windowAdded();
        tick(tray);
        check(hideRequests == before, "respect disabled startup hiding");
    }
    std::puts("PASS: startup hiding, restore, explicit Show, expiry, and disabled setting");
}
