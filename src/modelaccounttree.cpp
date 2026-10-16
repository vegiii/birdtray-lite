#include <QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include "modelaccounttree.h"
#include "utils.h"
#include "birdtrayapp.h"
#include "unreadmonitor.h"

ModelAccountTree::ModelAccountTree(QObject* parent, QTreeView* treeView)
        : QAbstractItemModel(parent) {
    mAccounts = BirdtrayApp::get()->getSettings()->watchedMorkFiles;
    treeView->setModel(this);
}

int ModelAccountTree::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant ModelAccountTree::data(const QModelIndex &index, int role) const {
    if (index.row() >= 0 && index.row() < mAccounts.size() && index.column() == 0) {
        switch (role) {
        case Qt::DisplayRole: {
            QFileInfo accountMorkFile(mAccounts[index.row()]);
            QString folderName = Utils::getMailFolderName(accountMorkFile);
            QString accountName = Utils::getMailAccountName(accountMorkFile);
            if (accountName.isNull()) {
                accountName = accountMorkFile.fileName();
            }
            if (folderName.isNull()) {
                return accountName;
            }
            return accountName + " [" + folderName + "]";
        }
        case Qt::ToolTipRole: {
            QString warning = BirdtrayApp::get()->getTrayIcon()->getUnreadMonitor()->getWarnings()
                                                .value(mAccounts[index.row()]);
            return warning.isNull() ? mAccounts[index.row()] : warning;
        }
        case Qt::ForegroundRole:
            if (BirdtrayApp::get()->getTrayIcon()->getUnreadMonitor()->getWarnings().contains(
                    mAccounts[index.row()])) {
                return QColor(255, 150, 0, 255);
            }
            return QVariant();
        default:
            break;
        }
    }
    return QVariant();
}

QModelIndex ModelAccountTree::index(int row, int column, const QModelIndex &) const
{
    if ( row < 0 || row >= mAccounts.size() || column != 0 )
        return QModelIndex();

    return createIndex( row, column );
}

QModelIndex ModelAccountTree::parent(const QModelIndex &) const
{
    // No item has parent
    return QModelIndex();
}

int ModelAccountTree::rowCount(const QModelIndex &) const
{
    return mAccounts.size();
}

Qt::ItemFlags ModelAccountTree::flags(const QModelIndex &) const
{
    // Same for all items
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ModelAccountTree::headerData(int section, Qt::Orientation , int role) const
{
    if ( role == Qt::DisplayRole && section == 0 )
        return QCoreApplication::translate("ModelAccountTree", "Account");

    return QVariant();
}

void ModelAccountTree::addAccount(const QString &path)
{
    if (path.isEmpty() || mAccounts.contains(path)) {
        return;
    }
    beginInsertRows( QModelIndex(), mAccounts.size(), mAccounts.size() );
    mAccounts.push_back( path );
    endInsertRows();
}

void ModelAccountTree::removeAccount(const QModelIndex &idx)
{
    beginRemoveRows( QModelIndex(), idx.row(), idx.row() );
    mAccounts.removeAt( idx.row() );
    endRemoveRows();
}

void ModelAccountTree::clear()
{
    if (mAccounts.isEmpty()) {
        return;
    }
    beginRemoveRows( QModelIndex(), 0, mAccounts.size() - 1 );
    mAccounts.clear();
    endRemoveRows();
}

void ModelAccountTree::applySettings() {
    BirdtrayApp::get()->getSettings()->watchedMorkFiles = mAccounts;
}
