#include "Radiant.h"
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>

#include "dialogs/EntityBrowser.h"
#include "dialogs/OrthographicView.h"
#include "dialogs/RenderPreview.h"

void Select_Deselect(bool = true);
void Select_Delete();

fhRadiant::fhRadiant(RenderCamera *renderCamera, QWidget *parent) : QMainWindow(parent) {
	dockManager = new ads::CDockManager(this);
	dockManager->setStyleSheet("");
	cameraView = new fhPreviewCamera(renderCamera, this);
	orthographicView = new fhOrthographicView(this);
	entityBrowser = new fhEntityBrowser(this);

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

	QObject::connect(deselect, &QAction::triggered, [=]() { Select_Deselect(); });
	QObject::connect(del, &QAction::triggered, [=]() { Select_Delete(); });

	addDockableView(cameraView->windowTitle(), cameraView, ads::LeftDockWidgetArea);
	addDockableView(orthographicView->windowTitle(), orthographicView, ads::RightDockWidgetArea);
	addDockableView(entityBrowser->windowTitle(), entityBrowser, ads::RightDockWidgetArea);
}

fhRadiant::~fhRadiant() { delete dockManager; }

void fhRadiant::update() {
	cameraView->draw();
	cameraView->repaint();

	orthographicView->draw();
	orthographicView->repaint();
}

ads::CDockAreaWidget *fhRadiant::addDockableView(const QString &name, QWidget *widget, ads::DockWidgetArea area) {
	ads::CDockWidget *dockWidget = new ads::CDockWidget(name);
	dockWidget->setWidget(widget);
	return dockManager->addDockWidget(area, dockWidget);
}
