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
#include "../tools/radiant/GLDrawable.h"
#include "../tools/radiant/QE3.H"
#include "../tools/radiant/RenderCamera.h"
#include "../tools/radiant/XYWnd.h"
#include "../widgets/RenderWidget.h"
#include <QToolBar>
#include <QWheelEvent>

void drawText(const char *text, float scale, const idVec3 &pos, const idVec4 &color);
void drawText(const char *text, float scale, const idVec3 &pos, const idVec4 &color, ViewType viewType);
void drawText(const char *text, float scale, const idVec3 &pos, const idVec3 &color, ViewType viewType);
void drawText(const char *text, float scale, const idVec3 &pos, const idVec3 &color);

static inline const char *viewTypeToString(ViewType type) {
	switch (type) {
	case ViewType::XY:
		return "XY";
	case ViewType::XZ:
		return "XZ";
	case ViewType::YZ:
		return "YZ";
	}
	return "";
}

class OrthographicDrable : public idGLDrawable {
public:
	OrthographicDrable(fhRenderWidget *parent) : parent(parent) {}

	void draw(int x, int y, int w, int h) override {
		m_nHeight = h;
		m_nWidth = w;
		draw();
	}

	void buttonDown(MouseButton button, float x, float y) override {
		if (button == MouseButton::Right) {
			dragPoint = idVec2(x, y);
			drag = true;
		}
	}
	void buttonUp(MouseButton button, float x, float y) override {
		if (button == MouseButton::Right) {
			drag = false;
		}
	}
	void mouseMove(float x, float y) override {
		if (drag) {
			
			idVec3 delta = idVec3(0, 0, 0);
			switch (m_nViewType) {
			case ViewType::XY:
				delta.x = -(x - dragPoint.x);
				delta.y = y - dragPoint.y;
				break;
			case ViewType::XZ:
				delta.x = x - dragPoint.x;
				delta.z = y - dragPoint.y;
				break;
			case ViewType::YZ:
				delta.y = x - dragPoint.x;
				delta.z = y - dragPoint.y;
				break;
			}
			m_vOrigin += delta * (1.0 / m_fScale);
			dragPoint = idVec2(x, y);
			parent->updateDrawable();
		}
	}

	void mouseScroll(float r) override {
		const float minScale = 0.02;
		const float maxScale = 4.0;
		m_fScale += r * 0.01;
		m_fScale = idMath::ClampFloat(minScale, maxScale, m_fScale);
		parent->updateDrawable();
	}

private:
	void draw() {
		GL_State(GLS_DEFAULT);
		glViewport(0, 0, m_nWidth, m_nHeight);
		glScissor(0, 0, m_nWidth, m_nHeight);
		glClearColor(g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
					 g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
					 g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2], 0);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// set up viewpoint

		const float w = m_nWidth / 2 / m_fScale;
		const float h = m_nHeight / 2 / m_fScale;

		const int nDim1 = (m_nViewType == ViewType::YZ) ? 1 : 0;
		const int nDim2 = (m_nViewType == ViewType::XY) ? 1 : 2;

		idVec2 mins, maxs;   // 2D view port mins/max
		idBounds viewBounds; // 3D world space bounds
		if (m_nViewType == ViewType::XY) {
			viewBounds[0].x = m_vOrigin.x - w;
			viewBounds[1].x = m_vOrigin.x + w;
			viewBounds[0].y = m_vOrigin.y - h;
			viewBounds[1].y = m_vOrigin.y + h;
			viewBounds[0].z = MIN_WORLD_COORD;
			viewBounds[1].z = MAX_WORLD_COORD;

			mins.x = m_vOrigin.x - w;
			mins.y = m_vOrigin.y - h;
			maxs.x = m_vOrigin.x + w;
			maxs.y = m_vOrigin.y + h;
		} else if (m_nViewType == ViewType::XZ) {
			viewBounds[0].x = m_vOrigin.x - w;
			viewBounds[1].x = m_vOrigin.x + w;
			viewBounds[0].y = MIN_WORLD_COORD;
			viewBounds[1].y = MAX_WORLD_COORD;
			viewBounds[0].z = m_vOrigin.z - h;
			viewBounds[1].z = m_vOrigin.z + h;

			mins.x = m_vOrigin.x - w;
			mins.y = m_vOrigin.z - h;
			maxs.x = m_vOrigin.x + w;
			maxs.y = m_vOrigin.z + h;
		} else if (m_nViewType == ViewType::YZ) {
			viewBounds[0].x = MIN_WORLD_COORD;
			viewBounds[1].x = MAX_WORLD_COORD;
			viewBounds[0].y = m_vOrigin.y - h;
			viewBounds[1].y = m_vOrigin.y + h;
			viewBounds[0].z = m_vOrigin.z - h;
			viewBounds[1].z = m_vOrigin.z + h;

			mins.x = m_vOrigin.y - w;
			mins.y = m_vOrigin.z - h;
			maxs.x = m_vOrigin.y + w;
			maxs.y = m_vOrigin.z + h;
		}

		GL_ProjectionMatrix.LoadIdentity();
		GL_ProjectionMatrix.Ortho(mins[0], maxs[0], mins[1], maxs[1], MIN_WORLD_COORD, MAX_WORLD_COORD);

		// draw stuff
		globalImages->BindNull();
		// now draw the grid
		XY_DrawGrid();
	}

	void XY_DrawGrid() {
		float x, y, xb, xe, yb, ye;
		int w, h;
		char text[32];

		int startPos = std::max(64.0f, g_qeglobals.d_gridsize);

		w = m_nWidth / 2 / m_fScale;
		h = m_nHeight / 2 / m_fScale;

		int nDim1 = (m_nViewType == ViewType::YZ) ? 1 : 0;
		int nDim2 = (m_nViewType == ViewType::XY) ? 1 : 2;

		// int nDim1 = 0; int nDim2 = 1;
		xb = m_vOrigin[nDim1] - w;
		if (xb < region_mins[nDim1]) {
			xb = region_mins[nDim1];
		}

		xb = startPos * floor(xb / startPos);

		xe = m_vOrigin[nDim1] + w;
		if (xe > region_maxs[nDim1]) {
			xe = region_maxs[nDim1];
		}

		xe = startPos * ceil(xe / startPos);

		yb = m_vOrigin[nDim2] - h;
		if (yb < region_mins[nDim2]) {
			yb = region_mins[nDim2];
		}

		yb = startPos * floor(yb / startPos);

		ye = m_vOrigin[nDim2] + h;
		if (ye > region_maxs[nDim2]) {
			ye = region_maxs[nDim2];
		}

		ye = startPos * ceil(ye / startPos);

		// draw major blocks
		glLineWidth(0.25);
		fhImmediateMode im;
		im.Color3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR].ToFloatPtr());

		int stepSize = 64 * 0.1 / m_fScale;
		if (stepSize < 64) {
			stepSize = std::max(64.0f, g_qeglobals.d_gridsize);
		} else {
			int i;
			for (i = 1; i < stepSize; i <<= 1) {
			}

			stepSize = i;
		}

		if (g_qeglobals.d_showgrid) {
			im.Begin(GL_LINES);

			for (x = xb; x <= xe; x += stepSize) {
				im.Vertex2f(x, yb);
				im.Vertex2f(x, ye);
			}

			for (y = yb; y <= ye; y += stepSize) {
				im.Vertex2f(xb, y);
				im.Vertex2f(xe, y);
			}

			im.End();
		}

		// draw minor blocks
		if (m_fScale > .1 && g_qeglobals.d_showgrid && g_qeglobals.d_gridsize * m_fScale >= 4 &&
			!g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR].Compare(g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK])) {

			im.Color3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR].ToFloatPtr());

			im.Begin(GL_LINES);
			for (x = xb; x < xe; x += g_qeglobals.d_gridsize) {
				if (!((int)x & (startPos - 1))) {
					continue;
				}

				im.Vertex2f(x, yb);
				im.Vertex2f(x, ye);
			}

			for (y = yb; y < ye; y += g_qeglobals.d_gridsize) {
				if (!((int)y & (startPos - 1))) {
					continue;
				}

				im.Vertex2f(xb, y);
				im.Vertex2f(xe, y);
			}

			im.End();
		}

		// draw ZClip boundaries (if applicable)...
		//
		if (m_nViewType == ViewType::XZ || m_nViewType == ViewType::YZ) {
			if (g_pParentWnd->GetZWnd()->m_pZClip) // should always be the case at this point I think, but this is safer
			{
				if (g_pParentWnd->GetZWnd()->m_pZClip->IsEnabled()) {
					im.Color3f(ZCLIP_COLOUR);

					// TODO(johl): linewidth>1 is deprecated. WTF?
					glLineWidth(1 /*2*/);
					im.Begin(GL_LINES);

					im.Vertex2f(xb, g_pParentWnd->GetZWnd()->m_pZClip->GetTop());
					im.Vertex2f(xe, g_pParentWnd->GetZWnd()->m_pZClip->GetTop());

					im.Vertex2f(xb, g_pParentWnd->GetZWnd()->m_pZClip->GetBottom());
					im.Vertex2f(xe, g_pParentWnd->GetZWnd()->m_pZClip->GetBottom());

					im.End();
					glLineWidth(1);
				}
			}
		}

		// draw coordinate text if needed
		if (g_qeglobals.d_savedinfo.show_coordinates) {
			const float textScale = 1.0 / m_fScale;
			const idVec3 textColor = g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT];
			const float textPadding = 4.0f;

			for (x = xb; x < xe; x += stepSize) {
				sprintf(text, "%i", (int)x);
				drawText(text, textScale, idVec3(textPadding + x, m_vOrigin[nDim2] + h - 10 / m_fScale, 0), textColor);
			}

			for (y = yb; y < ye; y += stepSize) {
				sprintf(text, "%i", (int)y);
				drawText(text, textScale, idVec3(m_vOrigin[nDim1] - w + 1, textPadding + y, 0), textColor);
			}

			const idVec3 viewNameColor = g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME];
			drawText(viewTypeToString(m_nViewType), textScale,
					 idVec3(m_vOrigin[nDim1] - w + 35 / m_fScale, m_vOrigin[nDim2] + h - 20 / m_fScale, 0), textColor);
		}
	}

	fhRenderWidget *parent;

	ViewType m_nViewType = ViewType::XY;
	idVec3 m_vOrigin = idVec3(0, 0, 0);
	int m_nWidth;
	int m_nHeight;
	float m_fScale = 1;

	idVec2 dragPoint;
	bool drag;
};

fhOrthographicView::fhOrthographicView(QWidget *parent) : QWidget(parent) {
	this->setWindowTitle("fhOrthgraphicView");

	QVBoxLayout *layout = new QVBoxLayout(this);
	this->setLayout(layout);

	QToolBar *toolbar = new QToolBar(this);
	layout->addWidget(toolbar);
	QAction *deselect = toolbar->addAction("deselect");
	deselect->setShortcut(QKeySequence(Qt::Key_Escape));

	QObject::connect(deselect, &QAction::triggered, [=]() { Select_Deselect(); });

	this->renderWidget = new fhRenderWidget(this);
	layout->addWidget(renderWidget);

	this->drawable = new OrthographicDrable(this->renderWidget);
	renderWidget->setDrawable(drawable);
}

fhOrthographicView::~fhOrthographicView() {}

void fhOrthographicView::draw() {
	common->Printf("%d fhPreviewCamera requested\n", Sys_Milliseconds());
	renderWidget->updateDrawable();
}
