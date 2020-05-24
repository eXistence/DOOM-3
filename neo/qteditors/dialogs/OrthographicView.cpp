/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

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

#include "OrthographicView.h"
#include "../tools/radiant/QE3.H"
#include "../tools/radiant/XYWnd.h"
#include "OrthographicWindow.h"
#include <QDropEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QToolBar>


fhOrthographicView::fhOrthographicView(QWidget *parent) : QWidget(parent) {
	this->setWindowTitle("2D View");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	this->setLayout(layout);

	QToolBar *toolbar = new QToolBar(this);
	layout->addWidget(toolbar);


	QAction *cycleViewType = toolbar->addAction("cycle");
	cycleViewType->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab));


	QObject::connect(cycleViewType, &QAction::triggered, [this]() {
		renderWindow->cycleViewType();
		renderWindow->requestUpdate();
	});

	renderWindow = new fhOrthoRenderWindow();
	auto renderWindowWidget = renderWindow->createContainingWidget(this);
	renderWindowWidget->installEventFilter(renderWindow);
	renderWindowWidget->setAcceptDrops(true);		
	layout->addWidget(renderWindowWidget);

	QObject::connect(renderWindow, &fhOrthoRenderWindow::contextMenuRequested,
					 [this](QPoint position) { showContextMenu(position); });
	
	resize(QSize(600, 700));
}

fhOrthographicView::~fhOrthographicView() {}

void fhOrthographicView::draw() { renderWindow->requestUpdate(); }

void fhOrthographicView::showContextMenu(QPoint position) {

	QMenu contextMenu(tr("Context menu"), this);

	QAction action1("Remove Data Point", this);
	connect(&action1, &QAction::triggered, []() { common->Printf("foobar\n"); });
	contextMenu.addAction(&action1);

	contextMenu.exec(renderWindow->mapToGlobal(position));
}