#pragma once
#include <QAbstractItemModel>
#include <QVector>

enum class MaterialColumns { Name, Location, COUNT };

class fhMaterialTreeModelItem {
public:
	explicit fhMaterialTreeModelItem(const QString& name, const QString &displayName, const QString &location)
		: displayName(displayName), name(name), location(location) {}

	void addChild(fhMaterialTreeModelItem *child) {
		if (child->parent) {
			child->parent->childs.removeAll(child);
		}
		child->parent = this;
		childs.append(child);
	}

	fhMaterialTreeModelItem *child(int row) {
		if (row < 0 || row >= childs.size()) {
			return nullptr;
		}
		return childs[row];
	}

	int childCount() const { return childs.size(); }
	int columnCount() const { return static_cast<int>(MaterialColumns::COUNT); }

	QVariant data(int column) const {
		if (column == static_cast<int>(MaterialColumns::Name)) {
			return QVariant(displayName);
		}

		if (column == static_cast<int>(MaterialColumns::Location)) {
			return QVariant(location);
		}

		return QVariant();
	}

	int row() const {
		if (!parent) {
			return 0;
		}

		return parent->childs.indexOf(const_cast<fhMaterialTreeModelItem *>(this));
	}

	fhMaterialTreeModelItem *parentItem() { return parent; }

	fhMaterialTreeModelItem *findItemByName(const QString &displayName) {
		for (auto c : childs) {
			if (c->displayName == displayName) {
				return c;
			}
		}
		return nullptr;
	}

	const QString& getName() const { return name; }

private:
	QString displayName;
	QString name;
	QString location;
	fhMaterialTreeModelItem *parent = nullptr;
	QVector<fhMaterialTreeModelItem *> childs;
};

class fhMaterialTreeModel : public QAbstractItemModel {
	Q_OBJECT

public:
	explicit fhMaterialTreeModel(QObject *parent = nullptr);
	~fhMaterialTreeModel();

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVector<QString> getChildMaterials(const QModelIndex &parent) const;

private:
	void insertNewMaterial(fhMaterialTreeModelItem *parent, fhMaterialTreeModelItem *item);

	fhMaterialTreeModelItem *rootItem;
};