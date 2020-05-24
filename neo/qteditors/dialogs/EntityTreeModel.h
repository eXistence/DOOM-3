#pragma once
#include <QAbstractItemModel>
#include <QVector>

enum class EntityColumns { Name, EntityClassName, Location, COUNT };

class fhEntityTreeModelItem {
public:
	explicit fhEntityTreeModelItem(const QString &displayName, const QString &entityClassName, const QString &location)
		: name(displayName), entityClassName(entityClassName), location(location) {}

	void addChild(fhEntityTreeModelItem *child) {
		if (child->parent) {
			child->parent->childs.removeAll(child);
		}
		child->parent = this;
		childs.append(child);
	}

	fhEntityTreeModelItem *child(int row) {
		if (row < 0 || row >= childs.size()) {
			return nullptr;
		}
		return childs[row];
	}

	int childCount() const { return childs.size(); }
	int columnCount() const { return static_cast<int>(EntityColumns::COUNT); }

	QVariant data(int column) const {
		if (column == static_cast<int>(EntityColumns::Name)) {
			return QVariant(name);
		}

		if (column == static_cast<int>(EntityColumns::EntityClassName)) {
			return QVariant(entityClassName);
		}

		if (column == static_cast<int>(EntityColumns::Location)) {
			return QVariant(location);
		}

		return QVariant();
	}

	int row() const {
		if (!parent) {
			return 0;
		}

		return parent->childs.indexOf(const_cast<fhEntityTreeModelItem *>(this));
	}

	fhEntityTreeModelItem *parentItem() { return parent; }

	fhEntityTreeModelItem *findItemByName(const QString &name) {
		for (auto c : childs) {
			if (c->name == name) {
				return c;
			}
		}
		return nullptr;
	}

private:
	QString name;
	QString entityClassName;
	QString location;
	fhEntityTreeModelItem *parent = nullptr;
	QVector<fhEntityTreeModelItem *> childs;
};

class fhEntityTreeModel : public QAbstractItemModel {
	Q_OBJECT

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