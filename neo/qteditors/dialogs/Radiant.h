#pragma once

#include <QMainWindow>

#include "DockManager.h"

class RenderCamera;
class fhPreviewCamera;
class fhOrthographicView;
class fhEntityBrowser;
class fhMaterialBrowser;

class fhRadiant : public QMainWindow {
public:
	explicit fhRadiant(RenderCamera *renderCamera, QWidget *parent = 0);
	~fhRadiant();

	void update();

private:
	ads::CDockAreaWidget *addDockableView(const QString &name, QWidget *widget, ads::DockWidgetArea area);

	fhPreviewCamera *cameraView;
	fhOrthographicView *orthographicView;
	fhEntityBrowser *entityBrowser;
	fhMaterialBrowser *materialBrowser;

	ads::CDockManager *dockManager;
};