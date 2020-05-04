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

#include "RenderPreview.h"
#include "../tools/radiant/GLDrawable.h"
#include "../tools/radiant/QE3.H"
#include "../tools/radiant/RenderCamera.h"
#include "../widgets/RenderWidget.h"
#include "RenderPreview.h"
#include <QToolBar>

fhPreviewCamera::fhPreviewCamera(RenderCamera *renderCamera, QWidget *parent) : QWidget(parent) {
	this->setWindowTitle("fhPreviewCamera");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	this->setLayout(layout);	

	QToolBar *toolbar = new QToolBar(this);
	layout->addWidget(toolbar);

	QAction *deselect = toolbar->addAction("deselect");
	deselect->setShortcut(QKeySequence(Qt::Key_Escape));

	QAction *remove = toolbar->addAction("remove");	
	QList<QKeySequence> shortcuts;
	shortcuts << QKeySequence(Qt::Key_Backspace) << QKeySequence(Qt::Key_Delete);
	remove->setShortcuts(shortcuts);	

	QObject::connect(deselect, &QAction::triggered, [=]() { Select_Deselect(); });

	QObject::connect(remove, &QAction::triggered, [=]() { Select_Deselect(); });

	this->renderWidget = new fhRenderWidget(this);
	this->renderWidget->setCamera(renderCamera);
	layout->addWidget(renderWidget);
	renderWidget->setDrawable(new idGLDrawablePreview);
}

fhPreviewCamera::~fhPreviewCamera() {}

void fhPreviewCamera::draw() { renderWidget->updateDrawable(); }
