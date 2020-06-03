#include "MaterialBrowser.h"
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

fhMaterialBrowser::fhMaterialBrowser(QWidget *parent) : QWidget(parent) {
	setWindowTitle("Material Browser");
	model = new fhMaterialTreeModel();
	this->setLayout(new QVBoxLayout());

	auto filterLayout = new QHBoxLayout();

	filter = new QLineEdit(this);
	filterLayout->addWidget(filter);
	filterLayout->addWidget(new QPushButton("clear", this));
	layout()->addItem(filterLayout);

	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);

	proxyModel->setSortRole(Qt::DisplayRole);
	proxyModel->setFilterKeyColumn(static_cast<int>(MaterialColumns::Name));
	proxyModel->setSourceModel(model);
	proxyModel->setRecursiveFilteringEnabled(true);

	auto view = new QTreeView(this);
	view->setSortingEnabled(true);
	view->setDragDropMode(QAbstractItemView::DragOnly);
	view->setModel(proxyModel);
	view->setSelectionBehavior(QAbstractItemView::SelectRows);
	view->setContextMenuPolicy(Qt::CustomContextMenu);

	contextMenu = new QMenu(view);
	loadMaterialsAction = new QAction("load");
	QObject::connect(loadMaterialsAction, &QAction::triggered, [this]() {
		auto index = loadMaterialsAction->data().toModelIndex();
		if (index.isValid()) {
			loadMaterialsRequested(model->getChildMaterials(index));
		}
	});
	contextMenu->addAction(loadMaterialsAction);

	QObject::connect(view, &QTreeView::customContextMenuRequested, [this, view, proxyModel](const QPoint &point) {
		const QModelIndex index = view->indexAt(point);
		if (!index.isValid()) {
			return;
		}

		loadMaterialsAction->setData(QVariant(proxyModel->mapToSource(index)));
		contextMenu->exec(view->viewport()->mapToGlobal(point));
	});

	layout()->addWidget(view);
#if 0
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
#endif
	QObject::connect(filter, &QLineEdit::textChanged, [proxyModel](const QString &s) {
		auto tmp = s.trimmed();
		proxyModel->setFilterRegExp(".*" + s.trimmed() + ".*");
	});
}

fhMaterialBrowser::~fhMaterialBrowser() { delete model; }