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
#include "../widgets/RenderWindow.h"
#include <QToolBar>
#include <QWheelEvent>

void drawText(const char *text, float scale, const idVec3 &pos, const idVec4 &color);
void drawText(const char *text, float scale, const idVec3 &pos, const idVec4 &color, ViewType viewType);
void drawText(const char *text, float scale, const idVec3 &pos, const idVec3 &color, ViewType viewType);
void drawText(const char *text, float scale, const idVec3 &pos, const idVec3 &color);
void DrawOrientedText(ViewType viewType, float scale, const char *text, const idVec3 &pos, const idVec4 &color);
void DrawOrientedText(ViewType viewType, float scale, const char *text, const idVec3 &pos, const idVec3 &color);
bool CullBrush(const brush_t *brush, const idBounds &viewBounds);
void DrawPathLines();
float Betwixt(float f1, float f2);
void DrawCameraIcon(ViewType viewType, float scale);
void DrawOrigin(ViewType viewType, float scale, const idVec3 &position, float originX, float originY, const char *axisX,
				const char *axisY, const idVec3 &color);
void DrawDimension(ViewType viewType, float scale, const idVec3 &position, float value, const char *label,
				   const idVec3 &color);
void MoveSelection(const idVec3 &orgMove);

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

class fhOrthoCamera {
public:
	explicit fhOrthoCamera(const QWindow *window) : window(window) {}

	idVec3 origin = idVec3(0, 0, 0);
	float scale = 1.0f;
	ViewType viewType = ViewType::XY;

	void move(idVec3 delta) { origin += delta * (1.0 / scale); }

	void cycleViewType() { viewType = static_cast<ViewType>(((int)viewType + 1) % 3); }

	idVec3 WindowCoordsToPoint(int x, int y, float xyz, bool alignToGrid) const {
		idVec3 point = idVec3(0, 0, 0);
		float fx = x;
		float fy = y;
		float fw = window->width();
		float fh = window->height();

		if (viewType == ViewType::XY) {
			point[0] = origin[0] + (fx - fw / 2) / scale;
			point[1] = origin[1] - (fy - fh / 2) / scale;
			point[2] = xyz;
		} else if (viewType == ViewType::YZ) {
			point[1] = origin[1] + (fx - fw / 2) / scale;
			point[2] = origin[2] - (fy - fh / 2) / scale;
			point[0] = xyz;
		} else {
			point[0] = origin[0] + (fx - fw / 2) / scale;
			point[2] = origin[2] - (fy - fh / 2) / scale;
			point[1] = xyz;
		}

		if (alignToGrid) {
			for (int i = 0; i < 3; ++i) {
				point[i] = floor(point[i] / g_qeglobals.d_gridsize + 0.5) * g_qeglobals.d_gridsize;
			}
		}

		return point;
	}

	idVec3 WindowCoordsToPoint(int x, int y, bool alignToGrid) const {
		return WindowCoordsToPoint(x, y, 0, alignToGrid);
	}

	idVec3 WindowCoordsToPoint(QPoint p, bool alignToGrid) const {
		return WindowCoordsToPoint(p.x(), p.y(), alignToGrid);
	}

	idVec3 WindowCoordsToPoint(QPoint p, float xyz, bool alignToGrid) const {
		return WindowCoordsToPoint(p.x(), p.y(), xyz, alignToGrid);
	}

	idVec3 MouseEventToPoint(const QMouseEvent &event, bool alignToGrid) const {
		return WindowCoordsToPoint(event.x(), event.y(), alignToGrid);
	}

	idVec3 GetDirection() const {
		switch (viewType) {
		case ViewType::XY:
			return idVec3(0, 0, -1);
		case ViewType::XZ:
			return idVec3(0, -1, 0);
		case ViewType::YZ:
			return idVec3(-1, 0, 0);
		}
		assert(false && "invalid view type");
		return idVec3();
	}

private:
	const QWindow *window;
};

class fhOrthoRenderWindow : public fhRenderWindow {
public:
	explicit fhOrthoRenderWindow() : camera(this) {}

	void DragNewBrush(QPoint dragStart, QPoint cursor) {
		// delete the current selection
		if (selected_brushes.next != &selected_brushes) {
			Brush_Free(selected_brushes.next);
		}

		idVec3 mins = camera.WindowCoordsToPoint(dragStart, true);

		int nDim = (camera.viewType == ViewType::XY) ? 2 : (camera.viewType == ViewType::YZ) ? 0 : 1;

		mins[nDim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_bottom[nDim] / g_qeglobals.d_gridsize));

		idVec3 maxs = camera.WindowCoordsToPoint(cursor, true);

		maxs[nDim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_top[nDim] / g_qeglobals.d_gridsize));
		if (maxs[nDim] <= mins[nDim]) {
			maxs[nDim] = mins[nDim] + g_qeglobals.d_gridsize;
		}

		for (int i = 0; i < 3; i++) {
			if (mins[i] == maxs[i]) {
				return; // don't create a degenerate brush
			}

			if (mins[i] > maxs[i]) {
				float temp = mins[i];
				mins[i] = maxs[i];
				maxs[i] = temp;
			}
		}

		brush_t *n = Brush_Create(mins, maxs, &g_qeglobals.d_texturewin.texdef);
		if (!n) {
			return;
		}

		idVec3 vSize;
		VectorSubtract(maxs, mins, vSize);
		// g_strStatus.Format("Size X:: %.1f  Y:: %.1f  Z:: %.1f", vSize[0], vSize[1], vSize[2]);
		// g_pParentWnd->SetStatusText(2, g_strStatus);

		Brush_AddToList(n, &selected_brushes);

		Entity_LinkBrush(world_entity, n);

		Brush_Build(n);

		// Sys_UpdateWindows (W_ALL);
		Sys_UpdateWindows(W_XY | W_CAMERA);
	}

	void mouseMoveEvent(QMouseEvent *event) override {
		if (event->buttons() & Qt::LeftButton && !leftDrag) {
			if ((event->pos() - leftButtonDown).manhattanLength() > 10) {
				leftDrag = true;
				leftButtonDown = event->pos();
				dragNewBrush = Select_IsEmpty();
				if (!dragNewBrush) {
					g_qeglobals.d_num_move_points = 0;
					Brush_SideSelect(selected_brushes.next,
									 camera.WindowCoordsToPoint(event->pos(), HUGE_DISTANCE, false),
									 camera.GetDirection(), false);
				}
			}
		}

		if (event->buttons() & Qt::RightButton && !rightDrag) {
			if ((event->pos() - rightButtonDown).manhattanLength() > 10) {
				rightDrag = true;
				rightButtonDown = event->pos();
			}
		}

		if (event->buttons() & Qt::RightButton) {
			if (rightDrag) {
				QPoint cursor = event->pos();
				idVec3 delta = idVec3(0, 0, 0);
				switch (camera.viewType) {
				case ViewType::XY:
					delta.x = -(cursor.x() - rightButtonDown.x());
					delta.y = cursor.y() - rightButtonDown.y();
					break;
				case ViewType::XZ:
					delta.x = -(cursor.x() - rightButtonDown.x());
					delta.z = cursor.y() - rightButtonDown.y();
					break;
				case ViewType::YZ:
					delta.y = -(cursor.x() - rightButtonDown.x());
					delta.z = cursor.y() - rightButtonDown.y();
					break;
				}
				camera.move(delta);
				rightButtonDown = cursor;
			}
		} else if (event->buttons() & Qt::LeftButton) {
			if (leftDrag) {
				if (dragNewBrush) {
					DragNewBrush(leftButtonDown, event->pos());
				} else {
					idVec3 previous = camera.WindowCoordsToPoint(leftButtonDown, false);
					idVec3 current = camera.WindowCoordsToPoint(event->pos(), false);
					idVec3 delta = current - previous;
					leftButtonDown = event->pos();

					MoveSelection(delta);
				}
			}
		}

		requestUpdate();
	}

	void mousePressEvent(QMouseEvent *event) override {
		if (event->button() == Qt::LeftButton) {
			leftButtonDown = event->pos();
			leftDrag = false;
		}

		if (event->button() == Qt::RightButton) {
			rightButtonDown = event->pos();
			rightDrag = false;
		}
	}

	void mouseReleaseEvent(QMouseEvent *event) override {
		if (event->button() == Qt::LeftButton) {
			if (!leftDrag && QApplication::keyboardModifiers() & Qt::ShiftModifier) {
				idVec3 origin = camera.MouseEventToPoint(*event, false);
				idVec3 dir;
				switch (camera.viewType) {
				case ViewType::XY:
					origin.z = HUGE_DISTANCE;
					dir = idVec3(0, 0, -1);
					break;
				case ViewType::XZ:
					origin.y = HUGE_DISTANCE;
					dir = idVec3(0, -1, 0);
					break;
				case ViewType::YZ:
					origin.x = HUGE_DISTANCE;
					dir = idVec3(-1, 0, 0);
					break;
				}

				int flags = 0;
				if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
					flags |= SF_SINGLEFACE;
				}

				Select_Ray(origin, dir, flags);
			}
			leftDrag = false;
		}

		if (event->button() == Qt::RightButton) {
			rightDrag = false;
		}
		requestUpdate();
	}

	void wheelEvent(QWheelEvent *event) override {
		QPoint numDegrees = event->angleDelta() / 8;

		const float minScale = 0.02;
		const float maxScale = 4.0;
		camera.scale += numDegrees.y() * 0.01;
		camera.scale = idMath::ClampFloat(minScale, maxScale, camera.scale);
		requestUpdate();

		event->accept();
	}

	void cycleViewType() { camera.cycleViewType(); }

private:
	void render() override {
		GL_State(GLS_DEFAULT);
		glViewport(0, 0, width(), height());
		glScissor(0, 0, width(), height());
		glClearColor(g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
					 g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
					 g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2], 0);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// set up viewpoint

		const float w = width() / 2 / camera.scale;
		const float h = height() / 2 / camera.scale;

		const int nDim1 = (camera.viewType == ViewType::YZ) ? 1 : 0;
		const int nDim2 = (camera.viewType == ViewType::XY) ? 1 : 2;

		idVec2 mins, maxs;   // 2D view port mins/max
		idBounds viewBounds; // 3D world space bounds
		if (camera.viewType == ViewType::XY) {
			viewBounds[0].x = camera.origin.x - w;
			viewBounds[1].x = camera.origin.x + w;
			viewBounds[0].y = camera.origin.y - h;
			viewBounds[1].y = camera.origin.y + h;
			viewBounds[0].z = MIN_WORLD_COORD;
			viewBounds[1].z = MAX_WORLD_COORD;

			mins.x = camera.origin.x - w;
			mins.y = camera.origin.y - h;
			maxs.x = camera.origin.x + w;
			maxs.y = camera.origin.y + h;
		} else if (camera.viewType == ViewType::XZ) {
			viewBounds[0].x = camera.origin.x - w;
			viewBounds[1].x = camera.origin.x + w;
			viewBounds[0].y = MIN_WORLD_COORD;
			viewBounds[1].y = MAX_WORLD_COORD;
			viewBounds[0].z = camera.origin.z - h;
			viewBounds[1].z = camera.origin.z + h;

			mins.x = camera.origin.x - w;
			mins.y = camera.origin.z - h;
			maxs.x = camera.origin.x + w;
			maxs.y = camera.origin.z + h;
		} else if (camera.viewType == ViewType::YZ) {
			viewBounds[0].x = MIN_WORLD_COORD;
			viewBounds[1].x = MAX_WORLD_COORD;
			viewBounds[0].y = camera.origin.y - h;
			viewBounds[1].y = camera.origin.y + h;
			viewBounds[0].z = camera.origin.z - h;
			viewBounds[1].z = camera.origin.z + h;

			mins.x = camera.origin.y - w;
			mins.y = camera.origin.z - h;
			maxs.x = camera.origin.y + w;
			maxs.y = camera.origin.z + h;
		}

		GL_ProjectionMatrix.LoadIdentity();
		GL_ProjectionMatrix.Ortho(mins[0], maxs[0], mins[1], maxs[1], MIN_WORLD_COORD, MAX_WORLD_COORD);

		// draw stuff
		globalImages->BindNull();
		// now draw the grid
		XY_DrawGrid();

		glLineWidth(0.5);
		int drawn = 0;
		int culled = 0;

		if (camera.viewType != ViewType::XY) {
			GL_ProjectionMatrix.Push();
			if (camera.viewType == ViewType::YZ) {
				GL_ProjectionMatrix.Rotate(-90.0f, 0.0f, 1.0f, 0.0f);
			}

			// else
			GL_ProjectionMatrix.Rotate(-90.0f, 1.0f, 0.0f, 0.0f);
		}

		entity_t *e = world_entity;

		for (brush_t *brush = active_brushes.next; brush != &active_brushes; brush = brush->next) {
			if (CullBrush(brush, viewBounds)) {
				culled++;
				continue;
			}

			if (FilterBrush(brush)) {
				continue;
			}

			drawn++;

			const idVec3 brushColor = (brush->owner != e && brush->owner)
										  ? brush->owner->eclass->color
										  : g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES];

			Brush_DrawXY(brush, camera.viewType, false, brushColor);
		}

		DrawPathLines();

		// draw pointfile
		Pointfile_Draw();

		if (!(camera.viewType == ViewType::XY)) {
			GL_ProjectionMatrix.Pop();
		}

		// draw block grid
		if (g_qeglobals.show_blocks) {
			XY_DrawBlockGrid();
		}

		// now draw selected brushes
		if (camera.viewType != ViewType::XY) {
			GL_ProjectionMatrix.Push();
			if (camera.viewType == ViewType::YZ) {
				GL_ProjectionMatrix.Rotate(-90.0f, 0.0f, 1.0f, 0.0f);
			}

			// else
			GL_ProjectionMatrix.Rotate(-90.0f, 1.0f, 0.0f, 0.0f);
		}

		GL_ProjectionMatrix.Push();

		idVec3 brushColor = g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES];

		/*
		if (RotateMode()) {
			brushColor.Set(0.8f, 0.1f, 0.9f);
		} else if (ScaleMode()) {
			brushColor.Set(0.1f, 0.8f, 0.1f);
		}
		*/

		glLineWidth(1);

		idVec3 vMinBounds;
		idVec3 vMaxBounds;
		vMinBounds[0] = vMinBounds[1] = vMinBounds[2] = 999999.9f;
		vMaxBounds[0] = vMaxBounds[1] = vMaxBounds[2] = -999999.9f;

		int nSaveDrawn = drawn;
		bool bFixedSize = false;

		glEnable(GL_DEPTH_TEST);
		g_qeglobals.lineBuffer.Commit();
		glDisable(GL_DEPTH_TEST);

		for (brush_t *brush = selected_brushes.next; brush != &selected_brushes; brush = brush->next) {
			drawn++;
			Brush_DrawXY(brush, camera.viewType, true, brushColor);

			if (!bFixedSize) {
				if (brush->owner->eclass->fixedsize) {
					bFixedSize = true;
				}

				if (g_PrefsDlg.m_bSizePaint) {
					for (int i = 0; i < 3; i++) {
						if (brush->mins[i] < vMinBounds[i]) {
							vMinBounds[i] = brush->mins[i];
						}

						if (brush->maxs[i] > vMaxBounds[i]) {
							vMaxBounds[i] = brush->maxs[i];
						}
					}
				}
			}
		}

		g_qeglobals.lineBuffer.Commit();
		glEnable(GL_DEPTH_TEST);

		glLineWidth(0.5);

		if (!bFixedSize && !RotateMode() && !ScaleMode() && drawn - nSaveDrawn > 0 && g_PrefsDlg.m_bSizePaint) {
			PaintSizeInfo(nDim1, nDim2, vMinBounds, vMaxBounds);
		}

		// edge / vertex flags
		if (g_qeglobals.d_select_mode == sel_vertex) {
			glPointSize(4);
			fhImmediateMode im;
			im.Color3f(0, 1, 0);
			im.Begin(GL_POINTS);
			for (int i = 0; i < g_qeglobals.d_numpoints; i++) {
				im.Vertex3fv(g_qeglobals.d_points[i].ToFloatPtr());
			}

			im.End();
			glPointSize(1);
		} else if (g_qeglobals.d_select_mode == sel_edge) {
			float *v1, *v2;

			glPointSize(4);
			fhImmediateMode im;
			im.Color3f(0, 0, 1);
			im.Begin(GL_POINTS);
			for (int i = 0; i < g_qeglobals.d_numedges; i++) {
				v1 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p1].ToFloatPtr();
				v2 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p2].ToFloatPtr();
				im.Vertex3f((v1[0] + v2[0]) * 0.5, (v1[1] + v2[1]) * 0.5, (v1[2] + v2[2]) * 0.5);
			}

			im.End();
			glPointSize(1);
		}

		g_splineList->draw(
			static_cast<bool>(g_qeglobals.d_select_mode == sel_editpoint || g_qeglobals.d_select_mode == sel_addpoint));

		if (g_pParentWnd->GetNurbMode() && g_pParentWnd->GetNurb()->GetNumValues()) {
			int maxage = g_pParentWnd->GetNurb()->GetNumValues();
			int time = 0;
			fhImmediateMode im;
			im.Color3f(0, 0, 1);
			glPointSize(1);
			im.Begin(GL_POINTS);
			g_pParentWnd->GetNurb()->SetOrder(3);
			for (int i = 0; i < 100; i++) {
				idVec2 v = g_pParentWnd->GetNurb()->GetCurrentValue(time);
				im.Vertex3f(v.x, v.y, 0.0f);
				time += 10;
			}
			im.End();
			glPointSize(4);
			im.Color3f(0, 0, 1);
			im.Begin(GL_POINTS);
			for (int i = 0; i < maxage; i++) {
				idVec2 v = g_pParentWnd->GetNurb()->GetValue(i);
				im.Vertex3f(v.x, v.y, 0.0f);
			}
			im.End();
			glPointSize(1);
		}

		GL_ProjectionMatrix.Pop();

		if (!(camera.viewType == ViewType::XY)) {
			GL_ProjectionMatrix.Pop();
		}

		// area selection hack
		if (g_qeglobals.d_select_mode == sel_area) {
			glEnable(GL_BLEND);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			const idVec3 size = g_qeglobals.d_vAreaTL - g_qeglobals.d_vAreaBR;
			const idVec3 tl = g_qeglobals.d_vAreaTL;
			const idVec3 tr = tl - idVec3(size.x, 0, 0);
			const idVec3 br = g_qeglobals.d_vAreaBR;
			const idVec3 bl = br + idVec3(size.x, 0, 0);

			fhImmediateMode im;
			im.Color4f(0.0, 0.0, 1.0, 0.25);
			im.Begin(GL_TRIANGLES);
			im.Vertex3fv(tl.ToFloatPtr());
			im.Vertex3fv(tr.ToFloatPtr());
			im.Vertex3fv(br.ToFloatPtr());
			im.Vertex3fv(br.ToFloatPtr());
			im.Vertex3fv(bl.ToFloatPtr());
			im.Vertex3fv(tl.ToFloatPtr());
			im.End();

			im.Color3f(1, 1, 1);
			im.Begin(GL_LINES);
			im.Vertex3fv(tl.ToFloatPtr());
			im.Vertex3fv(tr.ToFloatPtr());
			im.Vertex3fv(tr.ToFloatPtr());
			im.Vertex3fv(br.ToFloatPtr());
			im.Vertex3fv(br.ToFloatPtr());
			im.Vertex3fv(bl.ToFloatPtr());
			im.Vertex3fv(bl.ToFloatPtr());
			im.Vertex3fv(tl.ToFloatPtr());
			im.End();
		}

		// now draw camera point
		DrawCameraIcon(camera.viewType, camera.scale);

		if (RotateMode()) {
			// DrawRotateIcon();
		}

		g_qeglobals.pointBuffer.Commit();
		glFlush();

		R_ToggleSmpFrame();
	}

	void XY_DrawGrid() {
		float x, y, xb, xe, yb, ye;
		int w, h;
		char text[32];

		int startPos = std::max(64.0f, g_qeglobals.d_gridsize);

		w = width() / 2 / camera.scale;
		h = height() / 2 / camera.scale;

		int nDim1 = (camera.viewType == ViewType::YZ) ? 1 : 0;
		int nDim2 = (camera.viewType == ViewType::XY) ? 1 : 2;

		// int nDim1 = 0; int nDim2 = 1;
		xb = camera.origin[nDim1] - w;
		if (xb < region_mins[nDim1]) {
			xb = region_mins[nDim1];
		}

		xb = startPos * floor(xb / startPos);

		xe = camera.origin[nDim1] + w;
		if (xe > region_maxs[nDim1]) {
			xe = region_maxs[nDim1];
		}

		xe = startPos * ceil(xe / startPos);

		yb = camera.origin[nDim2] - h;
		if (yb < region_mins[nDim2]) {
			yb = region_mins[nDim2];
		}

		yb = startPos * floor(yb / startPos);

		ye = camera.origin[nDim2] + h;
		if (ye > region_maxs[nDim2]) {
			ye = region_maxs[nDim2];
		}

		ye = startPos * ceil(ye / startPos);

		// draw major blocks
		glLineWidth(0.25);
		fhImmediateMode im;
		im.Color3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR].ToFloatPtr());

		int stepSize = 64 * 0.1 / camera.scale;
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
		if (camera.scale > .1 && g_qeglobals.d_showgrid && g_qeglobals.d_gridsize * camera.scale >= 4 &&
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
		if (camera.viewType == ViewType::XZ || camera.viewType == ViewType::YZ) {
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
			const float textScale = 1.0 / camera.scale;
			const idVec3 textColor = g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT];
			const float textPadding = 4.0f;

			for (x = xb; x < xe; x += stepSize) {
				sprintf(text, "%i", (int)x);
				drawText(text, textScale, idVec3(textPadding + x, camera.origin[nDim2] + h - 10 / camera.scale, 0),
						 textColor);
			}

			for (y = yb; y < ye; y += stepSize) {
				sprintf(text, "%i", (int)y);
				drawText(text, textScale, idVec3(camera.origin[nDim1] - w + 1, textPadding + y, 0), textColor);
			}

			const idVec3 viewNameColor = g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME];
			drawText(
				viewTypeToString(camera.viewType), textScale,
				idVec3(camera.origin[nDim1] - w + 35 / camera.scale, camera.origin[nDim2] + h - 20 / camera.scale, 0),
				textColor);
		}
	}

	void XY_DrawBlockGrid() {
		float x, y, xb, xe, yb, ye;
		int w, h;
		char text[32];

		w = width() / 2 / camera.scale;
		h = height() / 2 / camera.scale;

		int nDim1 = (camera.viewType == ViewType::YZ) ? 1 : 0;
		int nDim2 = (camera.viewType == ViewType::XY) ? 1 : 2;

		xb = camera.origin[nDim1] - w;
		if (xb < region_mins[nDim1]) {
			xb = region_mins[nDim1];
		}

		xb = 1024 * floor(xb / 1024);

		xe = camera.origin[nDim1] + w;
		if (xe > region_maxs[nDim1]) {
			xe = region_maxs[nDim1];
		}

		xe = 1024 * ceil(xe / 1024);

		yb = camera.origin[nDim2] - h;
		if (yb < region_mins[nDim2]) {
			yb = region_mins[nDim2];
		}

		yb = 1024 * floor(yb / 1024);

		ye = camera.origin[nDim2] + h;
		if (ye > region_maxs[nDim2]) {
			ye = region_maxs[nDim2];
		}

		ye = 1024 * ceil(ye / 1024);

		// draw major blocks
		const idVec3 color = g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK];
		fhImmediateMode im;
		im.Color3fv(color.ToFloatPtr());
		glLineWidth(0.5);

		im.Begin(GL_LINES);

		for (x = xb; x <= xe; x += 1024) {
			im.Vertex2f(x, yb);
			im.Vertex2f(x, ye);
		}

		for (y = yb; y <= ye; y += 1024) {
			im.Vertex2f(xb, y);
			im.Vertex2f(xe, y);
		}

		im.End();
		glLineWidth(0.25);

		// draw coordinate text if needed
		for (x = xb; x < xe; x += 1024) {
			for (y = yb; y < ye; y += 1024) {
				sprintf(text, "%i,%i", (int)floor(x / 1024), (int)floor(y / 1024));
				drawText(text, 1.0 / camera.scale, idVec3(x + 512, y + 512, 0), color);
			}
		}
	}

	void PaintSizeInfo(int nDim1, int nDim2, idVec3 vMinBounds, idVec3 vMaxBounds) {
		const idVec3 vSize = vMaxBounds - vMinBounds;
		const idVec3 color = g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES] * 0.65f;

		fhImmediateMode im;
		im.Color3fv(color.ToFloatPtr());

		float scale = camera.scale;

		if (camera.viewType == ViewType::XY) {

			im.Begin(GL_LINES);

			im.Vertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f / scale, 0.0f);
			im.Vertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);

			im.Vertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);
			im.Vertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);

			im.Vertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f / scale, 0.0f);
			im.Vertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale, 0.0f);

			im.Vertex3f(vMaxBounds[nDim1] + 6.0f / scale, vMinBounds[nDim2], 0.0f);
			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2], 0.0f);

			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2], 0.0f);
			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2], 0.0f);

			im.Vertex3f(vMaxBounds[nDim1] + 6.0f / scale, vMaxBounds[nDim2], 0.0f);
			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2], 0.0f);

			im.End();

			DrawOrigin(camera.viewType, scale, idVec3(vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / scale, 0),
					   vMinBounds[nDim1], vMaxBounds[nDim2], "x", "y", color);

			DrawDimension(camera.viewType, scale,
						  idVec3(Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), vMinBounds[nDim2] - 20.0 / scale, 0.0f),
						  vSize[nDim1], "x", color);

			DrawDimension(camera.viewType, scale,
						  idVec3(vMaxBounds[nDim1] + 16.0 / scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]), 0.0f),
						  vSize[nDim2], "y", color);
		} else if (camera.viewType == ViewType::XZ) {
			im.Begin(GL_LINES);

			im.Vertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 6.0f / scale);
			im.Vertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / scale);

			im.Vertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / scale);
			im.Vertex3f(vMaxBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / scale);

			im.Vertex3f(vMaxBounds[nDim1], 0, vMinBounds[nDim2] - 6.0f / scale);
			im.Vertex3f(vMaxBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / scale);

			im.Vertex3f(vMaxBounds[nDim1] + 6.0f / scale, 0, vMinBounds[nDim2]);
			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0, vMinBounds[nDim2]);

			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0, vMinBounds[nDim2]);
			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0, vMaxBounds[nDim2]);

			im.Vertex3f(vMaxBounds[nDim1] + 6.0f / scale, 0, vMaxBounds[nDim2]);
			im.Vertex3f(vMaxBounds[nDim1] + 10.0f / scale, 0, vMaxBounds[nDim2]);

			im.End();

			DrawOrigin(camera.viewType, scale, idVec3(vMinBounds[nDim1] + 4, 0, vMaxBounds[nDim2] + 8 / scale),
					   vMinBounds[nDim1], vMaxBounds[nDim2], "x", "z", color);

			DrawDimension(camera.viewType, scale,
						  idVec3(Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), 0, vMinBounds[nDim2] - 20.0 / scale),
						  vSize[nDim1], "x", color);

			DrawDimension(camera.viewType, scale,
						  idVec3(vMaxBounds[nDim1] + 16.0 / scale, 0, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2])),
						  vSize[nDim2], "z", color);
		} else {
			im.Begin(GL_LINES);

			im.Vertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f / scale);
			im.Vertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);

			im.Vertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);
			im.Vertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);

			im.Vertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f / scale);
			im.Vertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / scale);

			im.Vertex3f(0, vMaxBounds[nDim1] + 6.0f / scale, vMinBounds[nDim2]);
			im.Vertex3f(0, vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2]);

			im.Vertex3f(0, vMaxBounds[nDim1] + 10.0f / scale, vMinBounds[nDim2]);
			im.Vertex3f(0, vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2]);

			im.Vertex3f(0, vMaxBounds[nDim1] + 6.0f / scale, vMaxBounds[nDim2]);
			im.Vertex3f(0, vMaxBounds[nDim1] + 10.0f / scale, vMaxBounds[nDim2]);

			im.End();

			DrawOrigin(camera.viewType, scale, idVec3(0, vMinBounds[nDim1] + 4.0, vMaxBounds[nDim2] + 8 / scale),
					   vMinBounds[nDim1], vMaxBounds[nDim2], "y", "z", color);

			DrawDimension(camera.viewType, scale,
						  idVec3(0, Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), vMinBounds[nDim2] - 20.0 / scale),
						  vSize[nDim1], "y", color);

			DrawDimension(camera.viewType, scale,
						  idVec3(0, vMaxBounds[nDim1] + 16.0 / scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2])),
						  vSize[nDim2], "z", color);
		}
	}

	bool RotateMode() { return false; }

	/*
	 =======================================================================================================================
	 =======================================================================================================================
	 */
	bool ScaleMode() { return false; }

	fhOrthoCamera camera;

	QPoint rightButtonDown;
	bool rightDrag = false;

	QPoint leftButtonDown;
	bool leftDrag = false;
	bool dragNewBrush = false;
};

fhOrthographicView::fhOrthographicView(QWidget *parent) : QWidget(parent) {
	this->setWindowTitle("fhOrthgraphicView");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	this->setLayout(layout);

	QToolBar *toolbar = new QToolBar(this);
	layout->addWidget(toolbar);

	QAction *deselect = toolbar->addAction("deselect");
	deselect->setShortcut(QKeySequence(Qt::Key_Escape));

	QAction *cycleViewType = toolbar->addAction("cycle");
	cycleViewType->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab));

	QAction *remove = toolbar->addAction("remove");
	QList<QKeySequence> shortcuts;
	shortcuts << QKeySequence(Qt::Key_Backspace) << QKeySequence(Qt::Key_Delete);
	remove->setShortcuts(shortcuts);

	QObject::connect(deselect, &QAction::triggered, [=]() { Select_Deselect(); });

	QObject::connect(remove, &QAction::triggered, [=]() { Select_Delete(); });

	QObject::connect(cycleViewType, &QAction::triggered, [this]() {
		renderWindow->cycleViewType();
		renderWindow->requestUpdate();
	});

	renderWindow = new fhOrthoRenderWindow();
	layout->addWidget(renderWindow->createContainingWidget(this));

	resize(QSize(600, 700));
}

fhOrthographicView::~fhOrthographicView() {}

void fhOrthographicView::draw() { renderWindow->requestUpdate(); }
