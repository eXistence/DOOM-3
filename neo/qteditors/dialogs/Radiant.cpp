#include "Radiant.h"
#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

#include "DockAreaWidget.h"

#include "dialogs/EntityBrowser.h"
#include "dialogs/MaterialBrowser.h"
#include "dialogs/MaterialsView.h"
#include "dialogs/OrthographicView.h"
#include "dialogs/RenderPreview.h"

static ads::CDockWidget *createDockWidget(const QString &name, QWidget *widget) {
	ads::CDockWidget *dockWidget = new ads::CDockWidget(name);
	dockWidget->setWidget(widget);
	return dockWidget;
}

fhRadiant::fhRadiant(RenderCamera *renderCamera, QWidget *parent) : QMainWindow(parent) {
	dockManager = new ads::CDockManager(this);
	dockManager->setStyleSheet("");
	cameraView = new fhPreviewCamera(renderCamera, this);
	orthographicView = new fhOrthographicView(this);
	entityBrowser = new fhEntityBrowser(this);
	materialBrowser = new fhMaterialBrowser(this);
	materialsView = new fhMaterialsView(&session, this);

	QToolBar *toolbar = addToolBar(tr("File"));
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	QMenu *selectionMenu = menuBar()->addMenu(tr("&Selection"));

	QAction *deselect = selectionMenu->addAction("deselect");
	deselect->setShortcut(QKeySequence(Qt::Key_Escape));
	toolbar->addAction(deselect);

	QAction *del = selectionMenu->addAction("delete");
	QList<QKeySequence> shortcuts;
	shortcuts << QKeySequence(Qt::Key_Backspace) << QKeySequence(Qt::Key_Delete);
	del->setShortcuts(shortcuts);
	toolbar->addAction(del);

	QAction *open = new QAction(tr("&Open Map"), this);
	fileMenu->addAction(open);
	toolbar->addAction(open);

	QAction *save = new QAction(tr("&Save Map"), this);
	save->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
	fileMenu->addAction(save);
	toolbar->addAction(save);

	QAction *saveAs = new QAction(tr("&Save Map as"), this);
	fileMenu->addAction(saveAs);
	toolbar->addAction(saveAs);

	QObject::connect(deselect, &QAction::triggered, [this]() { session.deselect(); });
	QObject::connect(del, &QAction::triggered, [this]() { session.deleteSelected(); });
	QObject::connect(open, &QAction::triggered, [this]() { loadMapFile(); });
	QObject::connect(save, &QAction::triggered, [this]() { saveMapFile(); });
	QObject::connect(saveAs, &QAction::triggered, [this]() { saveAsMapFile(); });

	QObject::connect(materialBrowser, &fhMaterialBrowser::loadMaterialsRequested,
					 [this](const QVector<QString> &materials) { materialsView->setLoadedMaterials(materials); });

	auto left = addDockableView(cameraView->windowTitle(), cameraView, ads::LeftDockWidgetArea);
	auto middle = addDockableView(orthographicView->windowTitle(), orthographicView, ads::RightDockWidgetArea);
	auto right = addDockableView(entityBrowser->windowTitle(), entityBrowser, ads::RightDockWidgetArea);

	dockManager->addDockWidget(ads::CenterDockWidgetArea,
							   createDockWidget(materialBrowser->windowTitle(), materialBrowser), right);

	dockManager->addDockWidget(ads::BottomDockWidgetArea, createDockWidget(materialsView->windowTitle(), materialsView),
							   left);
}

fhRadiant::~fhRadiant() { delete dockManager; }

void fhRadiant::update() {
	cameraView->draw();
	cameraView->repaint();

	orthographicView->draw();
	orthographicView->repaint();
}

ads::CDockAreaWidget *fhRadiant::addDockableView(const QString &name, QWidget *widget, ads::DockWidgetArea area,
												 ads::CDockAreaWidget *dockAreaWidget) {
	return dockManager->addDockWidget(area, createDockWidget(name, widget), dockAreaWidget);
}

void fhRadiant::loadMapFile() {
	QFileDialog dialog(this);
	dialog.setNameFilter(tr("Maps (*.map)"));
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setFileMode(QFileDialog::ExistingFile);

	if (!dialog.exec())
		return;
	QStringList fileNames = dialog.selectedFiles();
	if (fileNames.isEmpty() || fileNames.first().isEmpty()) {
		return;
	}

	session.loadMap(fileNames.first());
}

void fhRadiant::saveAsMapFile() {
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Maps (*.map)"));
	dialog.setViewMode(QFileDialog::Detail);

	if (!dialog.exec())
		return;
	QStringList fileNames = dialog.selectedFiles();
	if (fileNames.isEmpty() || fileNames.first().isEmpty()) {
		return;
	}

	session.saveMap(fileNames.first());
}

void fhRadiant::saveMapFile() {
	const auto file = session.getMapFile();
	if (file.isEmpty()) {
		saveAsMapFile();
	} else {
		session.saveMap(file);
	}
}