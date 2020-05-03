/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of
these additional terms immediately following the terms and conditions of the GNU General Public License which
accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software
LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "dialogs/EfxEditor.h"
#include "dialogs/LightEditor.h"
#include "dialogs/OrthographicView.h"
#include "dialogs/RenderPreview.h"

#include "DockManager.h"

#include <qstylefactory.h>
#include <QMainWindow>


class fhRadiant : public QMainWindow {	
public:
	explicit fhRadiant(RenderCamera* renderCamera, QWidget *parent = 0) : QMainWindow(parent) {
		dockManager = new ads::CDockManager(this);
		cameraView = new fhPreviewCamera(renderCamera, this);
		orthographicView = new fhOrthographicView(this);

		QLabel *l = new QLabel();
		l->setWordWrap(true);
		l->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		l->setText("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. ");

		addDockableView("label1", l, ads::LeftDockWidgetArea);
		addDockableView("Camera View", cameraView, ads::LeftDockWidgetArea);
		addDockableView("2D View", orthographicView, ads::RightDockWidgetArea);
	}

	~fhRadiant() { delete dockManager;
	}

	void update() {
		cameraView->draw();
		cameraView->repaint();

		orthographicView->draw();
		orthographicView->repaint();
	}

private:
	ads::CDockAreaWidget* addDockableView(const char* name, QWidget *widget, ads::DockWidgetArea area) {
		ads::CDockWidget* dockWidget = new ads::CDockWidget(name);
		dockWidget->setWidget(widget);
		return dockManager->addDockWidget(area, dockWidget);
	}

	fhPreviewCamera *cameraView;
	fhOrthographicView *orthographicView;

	ads::CDockManager *dockManager;
};


class RenderCamera;

static QApplication *app = nullptr;
static fhLightEditor *lightEditor = nullptr;
static fhEfxEditor *efxEditor = nullptr;
static fhRadiant *radiant = nullptr;

void QtRun() {
	if (!app) {
		static char *name = "foo";
		char **argv = &name;
		int argc = 1;
		app = new QApplication(argc, argv);
#if 0
		app->setStyle(QStyleFactory::create("Fusion"));

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
		darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::BrightText, Qt::red);
		darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::HighlightedText, Qt::black);

		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
		darkPalette.setColor(QPalette::Disabled, QPalette::BrightText, Qt::darkGray);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

		app->setPalette(darkPalette);

		app->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
#endif
	}

	if (efxEditor && efxEditor->isVisible()) {
		efxEditor->UpdateGui();
	}

	QApplication::processEvents();
}

void QtLightEditorInit(const idDict *spawnArgs) {
	if (!lightEditor) {
		lightEditor = new fhLightEditor(nullptr);
	}

	lightEditor->initFromSpawnArgs(spawnArgs);
	lightEditor->show();
	lightEditor->setFocus();
}

void EfxEditorInit() {
	if (!efxEditor) {
		efxEditor = new fhEfxEditor(nullptr);
	}

	efxEditor->Init();
	efxEditor->show();
	efxEditor->setFocus();
}

void QtPreviewCamera(RenderCamera *renderCamera) {
	if (!radiant) {
		radiant = new fhRadiant(renderCamera, nullptr);
	}
	radiant->show();
	radiant->setFocus();
}

void QtPreviewCameraUpdate() {
	if (radiant) {
		radiant->update();
		qApp->processEvents();
	}
}