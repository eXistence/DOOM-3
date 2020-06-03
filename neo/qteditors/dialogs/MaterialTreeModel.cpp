#include "MaterialTreeModel.h"

fhMaterialTreeModel::fhMaterialTreeModel(QObject *parent) : QAbstractItemModel(parent) {
	rootItem = new fhMaterialTreeModelItem("name", "name", "file");

	int num = declManager->GetNumDecls(DECL_MATERIAL);
	for (int i = 0; i < num; ++i) {		
		auto decl = declManager->DeclByIndex(DECL_MATERIAL, i, false);		

		QString name = decl->GetName();
		
		QString location = QString("%1:%2").arg(decl->GetFileName()).arg(decl->GetLineNum());
		auto path = name.split("/", QString::SkipEmptyParts);
		auto parent = rootItem;
		for (int i = 0; i < path.size(); ++i) {
			if (i == path.size() - 1) {
				parent->addChild(new fhMaterialTreeModelItem(name, path[i], location));		
			} else {
				auto newParent = parent->findItemByName(path[i]);
				if (!newParent) {
					newParent = new fhMaterialTreeModelItem("", path[i], "");
					parent->addChild(newParent);					
				}
				parent = newParent;
			}
		}
	}
}

void fhMaterialTreeModel::insertNewMaterial(fhMaterialTreeModelItem *parent, fhMaterialTreeModelItem *item) {

}

fhMaterialTreeModel::~fhMaterialTreeModel() { delete rootItem; }

QVariant fhMaterialTreeModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	fhMaterialTreeModelItem *item = static_cast<fhMaterialTreeModelItem *>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags fhMaterialTreeModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index);
}

QVariant fhMaterialTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}

QModelIndex fhMaterialTreeModel::index(int row, int column, const QModelIndex &parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	fhMaterialTreeModelItem *parentItem = nullptr;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<fhMaterialTreeModelItem *>(parent.internalPointer());

	fhMaterialTreeModelItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);

	return QModelIndex();
}

QModelIndex fhMaterialTreeModel::parent(const QModelIndex &index) const {
	if (!index.isValid())
		return QModelIndex();

	fhMaterialTreeModelItem *childItem = static_cast<fhMaterialTreeModelItem *>(index.internalPointer());
	fhMaterialTreeModelItem *parentItem = childItem->parentItem();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int fhMaterialTreeModel::rowCount(const QModelIndex &parent) const {
	fhMaterialTreeModelItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<fhMaterialTreeModelItem *>(parent.internalPointer());
	 
	return parentItem->childCount();
}

int fhMaterialTreeModel::columnCount(const QModelIndex &parent) const {
	if (parent.isValid())
		return static_cast<fhMaterialTreeModelItem *>(parent.internalPointer())->columnCount();
	return rootItem->columnCount();
}


QVector<QString> fhMaterialTreeModel::getChildMaterials(const QModelIndex &parent) const {
	if (!parent.isValid())
		return {};

	QVector<QString> materials;
	auto item = static_cast<fhMaterialTreeModelItem *>(parent.internalPointer());
	if (!item->getName().isEmpty()) {
		materials.append(item->getName());
	} else {
		for (int i = 0; i < item->childCount(); ++i) {
			auto child = item->child(i);			

			auto material = item->child(i)->getName();
			if (!material.isEmpty()) {
				materials.append(material);
			}
		}
	}

	return materials;
}