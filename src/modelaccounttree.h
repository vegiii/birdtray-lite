#ifndef ACCOUNTTREEITEM_H
#define ACCOUNTTREEITEM_H

#include <QStringList>
#include <QAbstractItemModel>
#include <QtWidgets/QTreeView>

class ModelAccountTree : public QAbstractItemModel
{
    public:
        /**
         * A model that contains information about mail accounts registered to watch.
         * @param parent The parent of this widget.
         * @param treeView The widget that displays this model.
         */
        explicit ModelAccountTree(QObject *parent = nullptr, QTreeView* treeView = nullptr);

        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        void    addAccount( const QString& path );
        void    removeAccount( const QModelIndex& idx );
        void    clear();

        // The paths of the monitored mork files
        const QStringList& accounts() const { return mAccounts; }

        // Moves the current accounts to settings
        void    applySettings();

    private:
        QStringList mAccounts;
};

#endif // ACCOUNTTREEITEM_H
