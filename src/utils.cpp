#include "utils.h"
#include "version.h"
#include "birdtrayapp.h"

#include <QApplication>
#include <QStringConverter>
#include <QtCore/QDir>
#include <QtCore/QBuffer>

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

QString Utils::getBirdtrayVersion() {
    return QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH);
}

QString Utils::stdWToQString(const std::wstring &str) {
    return QString::fromStdWString(str);
}

std::wstring Utils::qToStdWString(const QString &str) {
    return str.toStdWString();
}

QStringList Utils::getThunderbirdProfilesPaths() {
    return {"~/.thunderbird", "~/snap/thunderbird/common/.thunderbird", "~/.var/app/org.mozilla.Thunderbird/.thunderbird"};
}

QStringList Utils::getDefaultThunderbirdCommand() {
    return { "/usr/bin/thunderbird" };
}

QStringList Utils::splitCommandLine(const QString &src)
{
    // This must handle strings like "C:\Program Files\exe\thunderbird.exe" --profile test --title "My test profile" test "a 'bb' \"V\" c"
    // must return an array with 7 elements
    QChar sep = QChar::Null;
    QStringList out;
    QString current;

    for ( int i = 0; i < src.length(); i++ )
    {
        QChar ch = src[i];

        // If we're inside a separator, we reset sep if we encounter a separator.
        // This ensures things like "aa 'bb' cc" are handled properly
        if ( sep != QChar::Null && ch == sep )
        {
            sep = QChar::Null;
            continue;
        }
        else if ( sep == QChar::Null && (ch == '\'' || ch == '"') )
        {
            sep = ch;
            continue;
        }

        // Space separates strings, but only if we're not inside a separator
        if ( sep == QChar::Null && ch.isSpace() )
        {
            out.push_back( current );
            current.clear();
        }
        else
            current.append( ch );
    }

    if ( !current.isEmpty() )
        out.push_back( current );

    return out;
}

QString Utils::getMailFolderName(const QFileInfo &morkFile) {
    QString dirName;
    QDir parentDir = morkFile.dir();
    QString morkBaseName = morkFile.completeBaseName();
    if (morkBaseName == "INBOX") {
        morkBaseName = "Inbox";
    }
    QString name = QCoreApplication::translate("EmailFolders", morkBaseName.toUtf8().constData());
    while ((dirName = parentDir.dirName()).endsWith(".sbd")) {
        dirName.chop(4);
        if (dirName == "INBOX") {
            dirName = "Inbox";
        }
        name = QCoreApplication::translate(
                "EmailFolders", dirName.toUtf8().constData()) + '/' + name;
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
    return name;
}

QString Utils::pixmapToString(const QPixmap &pixmap) {
    QByteArray iconData;
    QBuffer buffer(&iconData);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    buffer.close();
    return QString::fromLatin1(iconData.toBase64());
}

QPixmap Utils::pixmapFromString(const QString &data) {
    QPixmap pixmap;
    if (!data.isEmpty()) {
        pixmap.loadFromData(QByteArray::fromBase64(data.toLatin1()), "PNG");
    }
    return pixmap;
}
