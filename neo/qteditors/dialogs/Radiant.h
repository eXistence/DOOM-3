#pragma once

#include <QMainWindow>

#include "DockManager.h"
#include "EditorSession.h"

class RenderCamera;
class fhPreviewCamera;
class fhOrthographicView;
class fhEntityBrowser;
class fhMaterialBrowser;
class fhMaterialsView;

class fhRadiant : public QMainWindow {
public:
	explicit fhRadiant(RenderCamera *renderCamera, QWidget *parent = 0);
	~fhRadiant();

	void update();

private:
	ads::CDockAreaWidget *addDockableView(const QString &name, QWidget *widget, ads::DockWidgetArea area,
										  ads::CDockAreaWidget *dockAreaWidget = nullptr);

	void loadMapFile();
	void saveAsMapFile();
	void saveMapFile();

	fhPreviewCamera *cameraView;
	fhOrthographicView *orthographicView;
	fhEntityBrowser *entityBrowser;
	fhMaterialBrowser *materialBrowser;
	fhMaterialsView *materialsView;

	ads::CDockManager *dockManager;
	fhEditorSession session;
};