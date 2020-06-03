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

#include "MaterialsView.h"
#include "../tools/radiant/QE3.H"
#include "../tools/radiant/XYWnd.h"
#include "EditorSession.h"
#include "MaterialWindow.h"
#include <QDropEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTabBar>
#include <QToolBar>

namespace {

const int maxRecentlyUsed = 20;

template <typename T> QVector<fhMaterialRenderWindow::Entry> createEntries(const T &materials) {
	QVector<fhMaterialRenderWindow::Entry> ret;
	ret.reserve(materials.size());

	for (const auto &s : materials) {
		if (s.isEmpty()) {
			continue;
		}

		QByteArray ba = s.toLocal8Bit();
		const char *name = ba.data();

		auto material = declManager->FindMaterial(name);
		if (material) {
			declManager->FindMaterial(material->GetName());
			ret.append(fhMaterialRenderWindow::Entry(material));
		}
	}

	return ret;
}

} // namespace

fhMaterialsView::fhMaterialsView(fhEditorSession *session, QWidget *parent) : QWidget(parent), session(session) {
	this->setWindowTitle("Materials");
	scrollbar = new QScrollBar(Qt::Vertical, this);
	scrollbar->setMaximum(100);
	scrollbar->setMinimum(0);
	scrollbar->setValue(0);

	QVBoxLayout *verticalLayout = new QVBoxLayout(this);
	verticalLayout->setMargin(0);
	verticalLayout->setSpacing(0);
	this->setLayout(verticalLayout);

	tabbar = new QTabBar(this);
	tabbar->setExpanding(false);
	tabbar->insertTab((int)Tab::CurrentlyUsed, "Currently Used");
	tabbar->insertTab((int)Tab::RecentlyUsed, "Recently Used");
	tabbar->insertTab((int)Tab::LoadedDirectory, "Loaded Directory");

	verticalLayout->addWidget(tabbar);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	verticalLayout->addLayout(layout);

	renderWindow = new fhMaterialRenderWindow();
	auto renderWindowWidget = renderWindow->createContainingWidget(this);
	layout->addWidget(renderWindowWidget);
	layout->addWidget(scrollbar);

	QObject::connect(scrollbar, &QScrollBar::valueChanged,
					 [this](int value) { renderWindow->setScrollPosition(value); });

	QObject::connect(renderWindow, &fhMaterialRenderWindow::scrollHeightChanged, [this](int value) {
		common->Printf("new max height: %d\n", value);
		const auto previousValue = scrollbar->value();
		scrollbar->setMaximum(value);
		scrollbar->setMinimum(0);
		if (previousValue > value) {
			scrollbar->setValue(value);
		}
	});

	QObject::connect(renderWindow, &fhMaterialRenderWindow::scrollPositionChanged,
					 [this](int value) { scrollbar->setValue(value); });

	QObject::connect(renderWindow, &fhMaterialRenderWindow::materialSelected,
					 [this, session](const idMaterial *material) {
						 if (!material) {
							 return;
						 }

						 session->applyMaterial(material);

						 setMaterials(Tab::CurrentlyUsed, session->getMaterialsInUse());

						 auto &recentlyUsed = data[(int)Tab::RecentlyUsed];
						 auto entry = fhMaterialRenderWindow::Entry(material);
						 recentlyUsed.materials.removeAll(entry);

						 while (recentlyUsed.materials.size() >= maxRecentlyUsed) {
							 recentlyUsed.materials.removeLast();
						 }

						 recentlyUsed.materials.prepend(entry);
						 if (tabbar->currentIndex() == (int)(int)Tab::RecentlyUsed) {
							 renderWindow->setMaterials(recentlyUsed.materials);
						 }
					 });

	QObject::connect(tabbar, &QTabBar::currentChanged, [this](int index) {
		if (index < 0 || index >= (int)Tab::COUNT) {
			return;
		}

		renderWindow->setMaterials(data[index].materials);
	});

	resize(QSize(600, 700));
}

fhMaterialsView::~fhMaterialsView() {}

void fhMaterialsView::draw() { renderWindow->requestUpdate(); }

template <typename T> void fhMaterialsView::setMaterials(Tab tab, const T &materials) {
	const int index = (int)tab;
	data[index].materials = createEntries(materials);
	if (tabbar->currentIndex() == index) {
		renderWindow->setMaterials(data[index].materials);
	}
}

void fhMaterialsView::setLoadedMaterials(const QVector<QString> &materials) {
	setMaterials(Tab::LoadedDirectory, materials);
}
