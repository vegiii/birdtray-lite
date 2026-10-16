#include <QDir>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QRegularExpression>
#include <QSettings>

#include "thunderbird.h"
#include "utils.h"


QString Thunderbird::profilesDir()
{
    for (const QString &path : Utils::getThunderbirdProfilesPaths()) {
        QDir dir(Utils::expandPath(path));
        if (dir.exists("profiles.ini")) {
            return dir.absolutePath();
        }
    }
    return QString();
}

QString Thunderbird::defaultProfile(const QString &profilesDir)
{
    QSettings ini(QDir(profilesDir).filePath("profiles.ini"), QSettings::IniFormat);
    QString defaultPath;
    QString firstPath;

    // The default of an installation wins over the profile marked as default
    for (const QString &group : ini.childGroups()) {
        if (group.startsWith("Install")) {
            QString path = ini.value(group + "/Default").toString();
            if (!path.isEmpty()) {
                return QDir(profilesDir).absoluteFilePath(path);
            }
        } else if (group.startsWith("Profile")) {
            QString path = ini.value(group + "/Path").toString();
            if (ini.value(group + "/IsRelative", 1).toInt() == 1) {
                path = QDir(profilesDir).absoluteFilePath(path);
            }
            if (firstPath.isEmpty()) {
                firstPath = path;
            }
            if (ini.value(group + "/Default").toInt() == 1) {
                defaultPath = path;
            }
        }
    }
    return defaultPath.isEmpty() ? firstPath : defaultPath;
}

QList<Thunderbird::Account> Thunderbird::accounts(const QString &profileDir)
{
    QFile prefsFile(QDir(profileDir).filePath("prefs.js"));
    if (!prefsFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    // Lines look like: user_pref("mail.server.server2.name", "me@example.com");
    static const QRegularExpression serverPref(
            R"re(user_pref\("mail\.server\.(server\d+)\.(name|type|directory|directory-rel)",\s*"(.*)"\);)re");
    QMap<QString, QHash<QString, QString>> servers;
    const QStringList lines = QString::fromUtf8(prefsFile.readAll()).split('\n');
    for (const QString &line : lines) {
        QRegularExpressionMatch match = serverPref.match(line);
        if (match.hasMatch()) {
            servers[match.captured(1)][match.captured(2)] = match.captured(3);
        }
    }

    QList<Account> result;
    for (const QHash<QString, QString> &server : servers) {
        const QString type = server.value("type");
        if (type == "none" || type == "rss") {
            continue;
        }
        QString directory = server.value("directory");
        QString relative = server.value("directory-rel");
        if (relative.startsWith("[ProfD]")) {
            directory = QDir(profileDir).filePath(relative.mid(7));
        }
        if (directory.isEmpty() || !QDir(directory).exists()) {
            continue;
        }
        QString inbox;
        for (const QString &name : {"INBOX.msf", "Inbox.msf"}) {
            if (QFile::exists(QDir(directory).filePath(name))) {
                inbox = QDir(directory).filePath(name);
                break;
            }
        }
        result.append({server.value("name"), QDir(directory).absolutePath(), inbox});
    }
    return result;
}

QStringList Thunderbird::inboxes()
{
    const QString dir = profilesDir();
    if (dir.isEmpty()) {
        return {};
    }
    QStringList result;
    for (const Account &account : accounts(defaultProfile(dir))) {
        if (!account.inbox.isEmpty()) {
            result.append(account.inbox);
        }
    }
    return result;
}

QString Thunderbird::accountName(const QString &accountDir)
{
    // Account names are shown often, so prefs.js is only read again for unknown accounts
    static QHash<QString, QString> names;
    const QString key = QDir(accountDir).absolutePath();
    if (!names.contains(key)) {
        QDir profileDir(key);
        profileDir.cdUp();
        profileDir.cdUp();
        for (const Account &account : accounts(profileDir.absolutePath())) {
            names.insert(account.directory, account.name);
        }
        if (!names.contains(key)) {
            names.insert(key, QString());
        }
    }
    return names.value(key);
}
