#pragma once
#include <QAbstractItemModel>
#include <QVector>

enum class EntityColumns { Name, EntityClassName, Location, COUNT };

template <typename ItemType, typename Columns> class fhDeclTreeItem {
public:
	explicit fhDeclTreeItem(const QString &name, const QString &file) : name(name), file(file) {}

	void addChild(ItemType *child) {
		if (child->parent) {
			child->parent->childs.removeAll(child);
		}
		child->parent = static_cast<ItemType *>(this);
		childs.append(child);
	}

	ItemType *child(int row) {
		if (row < 0 || row >= childs.size()) {
			return nullptr;
		}
		return childs[row];
	}

	int childCount() const { return childs.size(); }
	int columnCount() const { return static_cast<int>(Columns::COUNT); }

	virtual QVariant data(Columns column) const = 0;

	int row() const {
		if (!parent) {
			return 0;
		}

		return parent->childs.indexOf(const_cast<ItemType *>(static_cast<const ItemType *>(this)));
	}

	ItemType *parentItem() { return parent; }

	ItemType *findItemByName(const QString &name) {
		for (auto c : childs) {
			if (c->name == name) {
				return c;
			}
		}
		return nullptr;
	}

protected:
	QString name;
	QString file;
	ItemType *parent = nullptr;
	QVector<ItemType *> childs;
};

class fhEntityTreeModelItem : public fhDeclTreeItem<fhEntityTreeModelItem, EntityColumns> {
public:
	explicit fhEntityTreeModelItem(const QString &displayName, const QString &entityClassName, const QString &file)
		: fhDeclTreeItem(displayName, file), entityClassName(entityClassName) {}

	QVariant data(EntityColumns column) const override {
		if (column == EntityColumns::Name) {
			return QVariant(name);
		}

		if (column == EntityColumns::EntityClassName) {
			return QVariant(entityClassName);
		}

		if (column == EntityColumns::Location) {
			return QVariant(file);
		}

		return QVariant();
	}

private:
	QString entityClassName;
};

class fhEntityTreeModel : public QAbstractItemModel {
public:
	explicit fhEntityTreeModel(QObject *parent = nullptr);
	~fhEntityTreeModel();

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
	fhEntityTreeModelItem *rootItem;
};