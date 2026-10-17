#include "utils.h"
#include "birdtrayapp.h"
#include "thunderbird.h"

#include <QtCore/QDir>

#include <wordexp.h>

QString Utils::expandPath(const QString &path) {
    QString rawPath;
    if (path.startsWith('"') && path.endsWith('"')) {
        rawPath = path.section('"', 1, 1);
    } else {
        rawPath = path;
    }
    wordexp_t result;
    if (wordexp(rawPath.toStdString().data(), &result, WRDE_NOCMD) != 0) {
        return rawPath;
    }
    QString expandedPath(result.we_wordv[0]);
    wordfree(&result);
    return expandedPath;
}

QStringList Utils::getThunderbirdProfilesPaths() {
    return {"~/.config/thunderbird", "~/.thunderbird",  "~/snap/thunderbird/common/.thunderbird", "~/.var/app/org.mozilla.Thunderbird/.thunderbird"};
}

QStringList Utils::getDefaultThunderbirdCommand() {
    // Use the Flatpak if Thunderbird is not installed as a package
    if (!QFile::exists("/usr/bin/thunderbird")
        && (QDir("/var/lib/flatpak/app/org.mozilla.Thunderbird").exists()
            || QDir(expandPath("~/.local/share/flatpak/app/org.mozilla.Thunderbird")).exists())) {
        return { "flatpak", "run", "org.mozilla.Thunderbird" };
    }
    return { "/usr/bin/thunderbird" };
}

QString Utils::getMailFolderName(const QFileInfo &morkFile) {
    QString dirName;
    QDir parentDir = morkFile.dir();
    QString morkBaseName = morkFile.completeBaseName();
    if (morkBaseName == "INBOX") {
        morkBaseName = "Inbox";
    }
    QString name = morkBaseName;
    while ((dirName = parentDir.dirName()).endsWith(".sbd")) {
        dirName.chop(4);
        if (dirName == "INBOX") {
            dirName = "Inbox";
        }
        name = dirName + '/' + name;
        if (!parentDir.cdUp()) {
            return QString();
        }
    }
    return name;
}

QString Utils::getMailAccountName(const QFileInfo &morkFile) {
    QDir parentDir = morkFile.dir();
    QString name;
    while ((name = parentDir.dirName()).endsWith(".sbd")) {
        if (!parentDir.cdUp()) {
            return QString();
        }
    }
    QString accountName = Thunderbird::accountName(parentDir.absolutePath());
    return accountName.isEmpty() ? name : accountName;
}
