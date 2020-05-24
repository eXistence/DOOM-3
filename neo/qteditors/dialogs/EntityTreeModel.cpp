#include "EntityTreeModel.h"

fhEntityTreeModel::fhEntityTreeModel(QObject *parent) : QAbstractItemModel(parent) {
	rootItem = new fhEntityTreeModelItem("name", "class", "location");

	int num = declManager->GetNumDecls(DECL_ENTITYDEF);
	for (int i = 0; i < num; ++i) {
		auto decl = declManager->DeclByIndex(DECL_ENTITYDEF, i);
		QString name = decl->GetName();
		QString location = QString("%1:%2").arg(decl->GetFileName()).arg(decl->GetLineNum());
		int index = name.indexOf("_");
		if (index < 0) {
			rootItem->addChild(new fhEntityTreeModelItem(name, name, location));
		} else {
			QString parentName = name.left(index);
			QString remaining = name.mid(index + 1);

			auto parent = rootItem->findItemByName(parentName);
			if (!parent) {
				parent = new fhEntityTreeModelItem(parentName, "", "");
				rootItem->addChild(parent);
			}
			parent->addChild(new fhEntityTreeModelItem(remaining, name, location));
		}
	}
}

fhEntityTreeModel::~fhEntityTreeModel() { delete rootItem; }

QVariant fhEntityTreeModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	fhEntityTreeModelItem *item = static_cast<fhEntityTreeModelItem *>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags fhEntityTreeModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index);
}

QVariant fhEntityTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}

QModelIndex fhEntityTreeModel::index(int row, int column, const QModelIndex &parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	fhEntityTreeModelItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<fhEntityTreeModelItem *>(parent.internalPointer());

	fhEntityTreeModelItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);

	return QModelIndex();
}

QModelIndex fhEntityTreeModel::parent(const QModelIndex &index) const {
	if (!index.isValid())
		return QModelIndex();

	fhEntityTreeModelItem *childItem = static_cast<fhEntityTreeModelItem *>(index.internalPointer());
	fhEntityTreeModelItem *parentItem = childItem->parentItem();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int fhEntityTreeModel::rowCount(const QModelIndex &parent) const {
	fhEntityTreeModelItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<fhEntityTreeModelItem *>(parent.internalPointer());

	return parentItem->childCount();
}

int fhEntityTreeModel::columnCount(const QModelIndex &parent) const {
	if (parent.isValid())
		return static_cast<fhEntityTreeModelItem *>(parent.internalPointer())->columnCount();
	return rootItem->columnCount();
}
