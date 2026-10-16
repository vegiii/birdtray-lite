#include <QDir>
#include <QDirIterator>
#include <QHeaderView>
#include <QPainter>

#include "foldertree.h"
#include "thunderbird.h"
#include "utils.h"
#include "birdtrayapp.h"
#include "unreadmonitor.h"

// The data role holding a folder's index file
static const int PathRole = Qt::UserRole;

FolderTree::FolderTree(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setColumnCount(1);
    connect(this, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem*, int column) {
        if (column == 0) {
            emit foldersChanged();
        }
    });
}

void FolderTree::load(const QString &profileDir, const QStringList &monitored)
{
    QSignalBlocker blocker(this);
    clear();
    const QMap<QString, QString> &warnings =
            BirdtrayApp::get()->getTrayIcon()->getUnreadMonitor()->getWarnings();

    for (const Thunderbird::Account &account : Thunderbird::accounts(profileDir)) {
        QList<QTreeWidgetItem*> folders;
        QDirIterator msfFiles(account.directory, {"*.msf"}, QDir::Files, QDirIterator::Subdirectories);
        while (msfFiles.hasNext()) {
            const QFileInfo msfFile(msfFiles.next());
            const QString name = Utils::getMailFolderName(msfFile);
            if (name.isNull()) {
                continue;
            }
            const QString path = msfFile.absoluteFilePath();
            auto* folder = new QTreeWidgetItem({name});
            folder->setData(0, PathRole, path);
            folder->setCheckState(0, monitored.contains(path) ? Qt::Checked : Qt::Unchecked);
            if (path == account.inbox) {
                QFont font = folder->font(0);
                font.setBold(true);
                folder->setFont(0, font);
            }
            if (warnings.contains(path)) {
                folder->setToolTip(0, warnings.value(path));
                folder->setForeground(0, QColor(255, 150, 0));
            }
            folders.append(folder);
        }
        if (folders.isEmpty()) {
            continue;
        }

        // The inbox comes first, then the other folders by name
        std::sort(folders.begin(), folders.end(), [&account](QTreeWidgetItem* a, QTreeWidgetItem* b) {
            bool aInbox = a->data(0, PathRole).toString() == account.inbox;
            bool bInbox = b->data(0, PathRole).toString() == account.inbox;
            if (aInbox != bInbox) {
                return aInbox;
            }
            return a->text(0).localeAwareCompare(b->text(0)) < 0;
        });

        auto* accountItem = new QTreeWidgetItem(this, {account.name.isEmpty()
                ? QDir(account.directory).dirName() : account.name});
        accountItem->setFlags(accountItem->flags() | Qt::ItemIsAutoTristate);
        accountItem->addChildren(folders);
    }
}

QStringList FolderTree::checkedFolders() const
{
    QStringList result;
    for (QTreeWidgetItemIterator it(const_cast<FolderTree*>(this),
            QTreeWidgetItemIterator::Checked | QTreeWidgetItemIterator::NoChildren); *it; ++it) {
        result.append((*it)->data(0, PathRole).toString());
    }
    return result;
}

void FolderTree::checkOnlyInboxes()
{
    for (QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::NoChildren); *it; ++it) {
        (*it)->setCheckState(0, (*it)->font(0).bold() ? Qt::Checked : Qt::Unchecked);
    }
}

QString FolderTree::profileOf(const QString &folder)
{
    // Folders are stored in <profile>/Mail, <profile>/ImapMail and similar directories
    QDir dir = QFileInfo(folder).dir();
    do {
        if (dir.dirName().endsWith("Mail") || dir.dirName() == "ExQuilla") {
            dir.cdUp();
            return dir.absolutePath();
        }
    } while (dir.cdUp());
    return QString();
}

void FolderTree::paintEvent(QPaintEvent* event)
{
    if (topLevelItemCount() > 0) {
        QTreeWidget::paintEvent(event);
        return;
    }
    QPainter painter(viewport());
    painter.setPen(palette().color(QPalette::PlaceholderText));
    painter.drawText(viewport()->rect().adjusted(12, 12, -12, -12), Qt::AlignCenter | Qt::TextWordWrap,
            tr("No mail accounts were found in this profile."));
}
