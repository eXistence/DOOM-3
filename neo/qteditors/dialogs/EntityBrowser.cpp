#include "EntityBrowser.h"
#include <QDrag>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

fhEntityBrowser::fhEntityBrowser(QWidget *parent) : QWidget(parent) {
	setWindowTitle("Entity Browser");
	model = new fhEntityTreeModel();
	this->setLayout(new QVBoxLayout());

	auto filterLayout = new QHBoxLayout();

	filter = new QLineEdit(this);
	filterLayout->addWidget(filter);
	filterLayout->addWidget(new QPushButton("clear", this));
	layout()->addItem(filterLayout);

	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);

	proxyModel->setSortRole(Qt::DisplayRole);
	proxyModel->setFilterKeyColumn(static_cast<int>(EntityColumns::EntityClassName));
	proxyModel->setSourceModel(model);
	proxyModel->setRecursiveFilteringEnabled(true);

	auto view = new QTreeView(this);
	view->setSortingEnabled(true);
	view->setDragDropMode(QAbstractItemView::DragOnly);
	view->setModel(proxyModel);
	view->setSelectionBehavior(QAbstractItemView::SelectRows);

	layout()->addWidget(view);

	QObject::connect(view, &QAbstractItemView::pressed, [this, proxyModel](const QModelIndex &index) {
		auto sourceIndex = proxyModel->mapToSource(index);

		QVariant data = model->data(
			model->index(sourceIndex.row(), static_cast<int>(EntityColumns::EntityClassName), sourceIndex.parent()),
			Qt::DisplayRole);
		QByteArray ba = data.toString().toLocal8Bit();
		common->Printf("pressed %s\n", ba.data());

		QDrag *drag = new QDrag(this);
		QMimeData *mimeData = new QMimeData;
		mimeData->setText(ba.data());
		drag->setMimeData(mimeData);

		Qt::DropAction dropAction = drag->exec();
	});

	QObject::connect(filter, &QLineEdit::textChanged, [proxyModel](const QString &s) {
		auto tmp = s.trimmed();
		proxyModel->setFilterRegExp(".*" + s.trimmed() + ".*");
	});
}

fhEntityBrowser::~fhEntityBrowser() { delete model; }