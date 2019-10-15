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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "RenderPreview.h"
#include "../../renderer/tr_local.h"
#include "../../renderer/model_local.h"	// for idRenderModelMD5
#include "../../renderer/ImmediateMode.h"
#include "../../tools/radiant/qe3.h"

extern void DrawPathLines();
extern void Select_RotateTexture(float amt, bool absolute);
extern void glLabeledPoint(idVec4 &color, idVec3 &point, float size, const char *label);
extern int Brush_ToTris(brush_t *brush, idTriList *tris, idMatList *mats, bool models, bool bmodel);

static bool IsBModel(brush_t *b) {
	const char *v = b->owner->ValueForKey("model");
	if (v && *v) {
		const char *n = b->owner->ValueForKey("name");
		return (stricmp(n, v) == 0);
	}
	return false;
}

int g_axialAnchor = -1;
int g_axialDest = -1;
bool g_bAxialMode = false;
brush_t *g_pSplitList = NULL;

void ValidateAxialPoints() {
	int faceCount = g_selectedFaces.Num();
	if (faceCount > 0) {
		face_t	*selFace = g_selectedFaces[0].GetFace();
		if (g_axialAnchor >= selFace->face_winding->GetNumPoints()) {
			g_axialAnchor = 0;
		}
		if (g_axialDest >= selFace->face_winding->GetNumPoints()) {
			g_axialDest = 0;
		}
	}
	else {
		g_axialDest = 0;
		g_axialAnchor = 0;
	}
}

RenderPreview::RenderPreview() {
	memset(&m_Camera, 0, sizeof(camera_t));
	worldDirty = true;
	worldModel = NULL;
	renderMode = false;
	rebuildMode = false;
	entityMode = false;
	animationMode = false;
	selectMode = false;
	soundMode = false;
	saveValid = false;
	Cam_Init();
}

RenderPreview::~RenderPreview() {
}

#if 0

void RenderPreview::OnPaint() {
	CPaintDC	dc(this);	// device context for painting
	bool		bPaint = true;

	if (!wglMakeCurrent(dc.m_hDC, win32.hGLRC)) {
		common->Printf("ERROR: wglMakeCurrent failed..\n ");
		common->Printf("Please restart " EDITOR_WINDOWTEXT " if the camera view is not working\n");
		return;
	}

	glViewport(0, 0, m_Camera.width, m_Camera.height);
	glScissor(0, 0, m_Camera.width, m_Camera.height);
	glClearColor(g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][0], g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][1], g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fhImmediateMode::ResetStats();
	QE_CheckOpenGLForErrors();

	g_pSplitList = NULL;
	if (g_bClipMode) {
		if (g_Clip1.Set() && g_Clip2.Set()) {
			g_pSplitList = ((g_pParentWnd->ActiveXY()->GetViewType() == XZ) ? !g_bSwitch : g_bSwitch) ? &g_brBackSplits : &g_brFrontSplits;
		}
	}

	Cam_Draw();
	QE_CheckOpenGLForErrors();
	wglSwapBuffers(dc.m_hDC);

}

#endif
/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_Init() {
	// m_Camera.draw_mode = cd_texture;
	m_Camera.origin[0] = 0.0f;
	m_Camera.origin[1] = 20.0f;
	m_Camera.origin[2] = 72.0f;
	m_Camera.color[0] = 0.3f;
	m_Camera.color[1] = 0.3f;
	m_Camera.color[2] = 0.3f;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_BuildMatrix() {

	const float xa = ((renderMode) ? -m_Camera.angles[PITCH] : m_Camera.angles[PITCH]) * idMath::M_DEG2RAD;
	const float ya = m_Camera.angles[YAW] * idMath::M_DEG2RAD;

	// the movement matrix is kept 2d
	m_Camera.forward[0] = cos(ya);
	m_Camera.forward[1] = sin(ya);
	m_Camera.right[0] = m_Camera.forward[1];
	m_Camera.right[1] = -m_Camera.forward[0];

	float	matrix[4][4];
	GL_ProjectionMatrix.Get(&matrix[0][0]);

	for (int i = 0; i < 3; i++) {
		m_Camera.vright[i] = matrix[i][0];
		m_Camera.vup[i] = matrix[i][1];
		m_Camera.vpn[i] = matrix[i][2];
	}

	m_Camera.vright.Normalize();
	m_Camera.vup.Normalize();
	m_Camera.vpn.Normalize();
	InitCull();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void RenderPreview::Cam_ChangeFloor(bool up) {
	brush_t *b;
	float	d, bestd, current;
	idVec3	start, dir;

	start[0] = m_Camera.origin[0];
	start[1] = m_Camera.origin[1];
	start[2] = HUGE_DISTANCE;
	dir[0] = dir[1] = 0;
	dir[2] = -1;

	current = HUGE_DISTANCE - (m_Camera.origin[2] - 72);
	if (up) {
		bestd = 0;
	}
	else {
		bestd = HUGE_DISTANCE * 2;
	}

	for (b = active_brushes.next; b != &active_brushes; b = b->next) {
		if (!Brush_Ray(start, dir, b, &d)) {
			continue;
		}

		if (up && d < current && d > bestd) {
			bestd = d;
		}

		if (!up && d > current && d < bestd) {
			bestd = d;
		}
	}

	if (bestd == 0 || bestd == HUGE_DISTANCE * 2) {
		return;
	}

	m_Camera.origin[2] += current - bestd;
	Sys_UpdateWindows(W_CAMERA | W_Z_OVERLAY);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_PositionDrag() {
	int x, y;
	Sys_GetCursorPos(&x, &y);
	if (x != m_ptCursor.x || y != m_ptCursor.y) {
		x -= m_ptCursor.x;
		VectorMA(m_Camera.origin, x, m_Camera.vright, m_Camera.origin);
		y -= m_ptCursor.y;
		m_Camera.origin[2] -= y;
		SetCursorPos(m_ptCursor.x, m_ptCursor.y);
		Sys_UpdateWindows(W_CAMERA | W_XY_OVERLAY);
	}
}

void RenderPreview::Cam_MouseLook() {
	CPoint current;

	GetCursorPos(&current);
	if (current.x != m_ptCursor.x || current.y != m_ptCursor.y) {

		current.x -= m_ptCursor.x;
		current.y -= m_ptCursor.y;

		m_Camera.angles[PITCH] -= (float)((float)current.y * 0.25f);
		m_Camera.angles[YAW] -= (float)((float)current.x * 0.25f);

		SetCursorPos(m_ptCursor.x, m_ptCursor.y);

		Cam_BuildMatrix();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_MouseControl(float dtime) {
	int		xl, xh;
	int		yl, yh;
	float	xf, yf;
	if (g_PrefsDlg.m_nMouseButtons == 2) {
		if (m_nCambuttonstate != (MK_RBUTTON | MK_SHIFT)) {
			return;
		}
	}
	else {
		if (m_nCambuttonstate != MK_RBUTTON) {
			return;
		}
	}

	xf = (float)(m_ptButton.x - m_Camera.width / 2) / (m_Camera.width / 2);
	yf = (float)(m_ptButton.y - m_Camera.height / 2) / (m_Camera.height / 2);

	xl = m_Camera.width / 3;
	xh = xl * 2;
	yl = m_Camera.height / 3;
	yh = yl * 2;

	// common->Printf("xf-%f yf-%f xl-%i xh-i% yl-i% yh-i%\n",xf,yf,xl,xh,yl,yh);
#if 0

	// strafe
	if (buttony < yl && (buttonx < xl || buttonx > xh)) {
		VectorMA(camera.origin, xf * dtime * g_nMoveSpeed, camera.right, camera.origin);
	}
	else
#endif
	{
		xf *= 1.0f - idMath::Fabs(yf);
		if (xf < 0.0f) {
			xf += 0.1f;
			if (xf > 0.0f) {
				xf = 0.0f;
			}
		}
		else {
			xf -= 0.1f;
			if (xf < 0.0f) {
				xf = 0.0f;
			}
		}

		VectorMA(m_Camera.origin, yf * dtime * g_PrefsDlg.m_nMoveSpeed, m_Camera.forward, m_Camera.origin);
		m_Camera.angles[YAW] += xf * -dtime * g_PrefsDlg.m_nAngleSpeed;
	}

	Cam_BuildMatrix();
	Sys_UpdateWindows(W_CAMERA | W_XY);
	g_pParentWnd->PostMessage(WM_TIMER, 0, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_MouseDown(int x, int y, int buttons) {
	idVec3	dir;
	float	f, r, u;
	int		i;

	// calc ray direction
	u = (float)(y - m_Camera.height / 2) / (m_Camera.width / 2);
	r = (float)(x - m_Camera.width / 2) / (m_Camera.width / 2);
	f = 1;

	for (i = 0; i < 3; i++) {
		dir[i] = m_Camera.vpn[i] * f + m_Camera.vright[i] * r + m_Camera.vup[i] * u;
	}

	dir.Normalize();

	GetCursorPos(&m_ptCursor);

	m_nCambuttonstate = buttons;
	m_ptButton.x = x;
	m_ptButton.y = y;

	//
	// LBUTTON = manipulate selection shift-LBUTTON = select middle button = grab
	// texture ctrl-middle button = set entire brush to texture ctrl-shift-middle
	// button = set single face to texture
	//
	int nMouseButton = g_PrefsDlg.m_nMouseButtons == 2 ? MK_RBUTTON : MK_MBUTTON;
	if
		(
		(buttons == MK_LBUTTON) ||
			(buttons == (MK_LBUTTON | MK_SHIFT)) ||
			(buttons == (MK_LBUTTON | MK_CONTROL)) ||
			(buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) ||
			(buttons == nMouseButton) ||
			(buttons == (nMouseButton | MK_SHIFT)) ||
			(buttons == (nMouseButton | MK_CONTROL)) ||
			(buttons == (nMouseButton | MK_SHIFT | MK_CONTROL))
			) {
		if (g_PrefsDlg.m_nMouseButtons == 2 && (buttons == (MK_RBUTTON | MK_SHIFT))) {
			Cam_MouseControl(0.1f);
		}
		else {
			// something global needs to track which window is responsible for stuff
			Patch_SetView(W_CAMERA);
			Drag_Begin(x, y, buttons, m_Camera.vright, m_Camera.vup, m_Camera.origin, dir);
		}

		return;
	}

	if (buttons == MK_RBUTTON) {
		Cam_MouseControl(0.1f);
		return;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_MouseUp(int x, int y, int buttons) {
	m_nCambuttonstate = 0;
	Drag_MouseUp(buttons);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::Cam_MouseMoved(int x, int y, int buttons) {
	m_nCambuttonstate = buttons;
	if (!buttons) {
		return;
	}

	m_ptButton.x = x;
	m_ptButton.y = y;

	if (buttons == (MK_RBUTTON | MK_CONTROL)) {
		Cam_PositionDrag();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}
	else if (buttons == (MK_RBUTTON | MK_CONTROL | MK_SHIFT)) {
		Cam_MouseLook();
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
		return;
	}

	GetCursorPos(&m_ptCursor);

	if (buttons & (MK_LBUTTON | MK_MBUTTON)) {
		Drag_MouseMoved(x, y, buttons);
		Sys_UpdateWindows(W_XY | W_CAMERA | W_Z);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RenderPreview::InitCull() {
#if 1
	const float xfov = 90;
	const float yfov = 2 * atan((float)m_Camera.height / m_Camera.width) * idMath::M_RAD2DEG;

	const idVec3 vieworg = m_Camera.origin;
	//const idMat3 viewaxis = m_Camera.angles.ToMat3();
	const auto viewaxis = idAngles(-m_Camera.angles.pitch, m_Camera.angles.yaw, m_Camera.angles.roll).ToMat3();

	float	xs, xc;
	float	ang;

	ang = DEG2RAD(xfov) * 0.5f;
	idMath::SinCos(ang, xs, xc);

	m_viewPlanes[0] = xs * viewaxis[0] + xc * viewaxis[1];
	m_viewPlanes[1] = xs * viewaxis[0] - xc * viewaxis[1];

	ang = DEG2RAD(yfov) * 0.5f;
	idMath::SinCos(ang, xs, xc);

	m_viewPlanes[2] = xs * viewaxis[0] + xc * viewaxis[2];
	m_viewPlanes[3] = xs * viewaxis[0] - xc * viewaxis[2];

	// plane four is the front clipping plane
	m_viewPlanes[4] = /* vec3_origin - */ viewaxis[0];

	for (int i = 0; i < 5; i++) {
		// flip direction so positive side faces out (FIXME: globally unify this)
		m_viewPlanes[i] = -m_viewPlanes[i].Normal();
		m_viewPlanes[i][3] = -(vieworg * m_viewPlanes[i].Normal());
	}
#else
	int i;

	VectorSubtract(m_Camera.vpn, m_Camera.vright, m_vCull1);
	VectorAdd(m_Camera.vpn, m_Camera.vright, m_vCull2);

	for (i = 0; i < 3; i++) {
		if (m_vCull1[i] > 0) {
			m_nCullv1[i] = 3 + i;
		}
		else {
			m_nCullv1[i] = i;
		}

		if (m_vCull2[i] > 0) {
			m_nCullv2[i] = 3 + i;
		}
		else {
			m_nCullv2[i] = i;
		}
	}
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool RenderPreview::CullBrush(brush_t *b, bool cubicOnly) {

	if (b->forceVisibile) {
		return false;
	}

	idBounds bounds = idBounds(b->mins, b->maxs);
	editorModel_t editorModel = Brush_GetEditorModel(b);
	bounds.AddBounds(editorModel.bounds);
	bounds.AddBounds(editorModel.model->Bounds());

	idVec3 points[8];
	bounds.ToPoints(points);

	if (g_PrefsDlg.m_bCubicClipping) {
		const float maxDistance = g_PrefsDlg.m_nCubicScale * 64;
		const float maxDistanceSquared = maxDistance * maxDistance;
#if 0
		bool outside = true;
		for (int i = 0; i < 8; ++i) {
			const idVec3 distance = points[i] - m_Camera.origin;

			if (distance.LengthSqr() < maxDistanceSquared) {
				outside = false;
				break;
			}
		}

		if (outside) {
			return true;
		}
#else
		idVec3 distance = bounds.GetCenter() - m_Camera.origin;

		if (distance.LengthSqr() > maxDistanceSquared) {
			return true;
		}
#endif
	}

	if (cubicOnly) {
		return false;
	}

	for (int i = 0; i < 5; ++i) {
		bool outside = true;

		for (int j = 0; j < 8; ++j) {
			if (m_viewPlanes[i].Distance(points[j]) < 0) {
				outside = false;
				break;
			}
		}

		if (outside) {
			return true;
		}
	}

	return false;
	/*
	idVec3	point;

	for (int i = 0; i < 3; i++) {
		point[i] = bounds[0][m_nCullv1[i]] - m_Camera.origin[i];
	}

	if (DotProduct(point, m_vCull1) < -1) {
		return true;
	}

	for (int i = 0; i < 3; i++) {
		point[i] = bounds[0][m_nCullv2[i]] - m_Camera.origin[i];
	}

	if (DotProduct(point, m_vCull2) < -1) {
		return true;
	}

	return false;
	*/
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
static void setGLMode(RenderPreviewMode mode) {
	switch (mode)
	{
	case RenderPreviewMode::Wireframe:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		globalImages->BindNull();
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		break;

	case RenderPreviewMode::Solid:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		globalImages->BindNull();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		break;

	case RenderPreviewMode::Textures:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		break;

	default:
		assert(false && "unknown GL mode!?");
	}
}




static void DrawAxial(face_t *selFace) {
	if (g_bAxialMode) {
		idVec3 points[4];

		for (int j = 0; j < selFace->face_winding->GetNumPoints(); j++) {
			glLabeledPoint(idVec4(1, 1, 1, 1), (*selFace->face_winding)[j].ToVec3(), 3, va("%i", j));
		}

		ValidateAxialPoints();
		points[0] = (*selFace->face_winding)[g_axialAnchor].ToVec3();
		VectorMA(points[0], 1, selFace->plane, points[0]);
		VectorMA(points[0], 4, selFace->plane, points[1]);
		points[3] = (*selFace->face_winding)[g_axialDest].ToVec3();
		VectorMA(points[3], 1, selFace->plane, points[3]);
		VectorMA(points[3], 4, selFace->plane, points[2]);
		glLabeledPoint(idVec4(1, 0, 0, 1), points[1], 3, "Anchor");
		glLabeledPoint(idVec4(1, 1, 0, 1), points[2], 3, "Dest");
		fhImmediateMode im;
		im.Begin(GL_LINE_STRIP);
		im.Vertex3fv(points[0].ToFloatPtr());
		im.Vertex3fv(points[1].ToFloatPtr());
		im.Vertex3fv(points[2].ToFloatPtr());
		im.Vertex3fv(points[3].ToFloatPtr());
		im.End();
	}
}


/*
 =======================================================================================================================
	Cam_Draw
 =======================================================================================================================
 */
void RenderPreview::SetProjectionMatrix() {
	float xfov = 90;
	float yfov = 2 * atan((float)m_Camera.height / m_Camera.width) * idMath::M_RAD2DEG;

	float	xmin, xmax, ymin, ymax;
	float	width, height;
	float	zNear;
	float	projectionMatrix[16];

	//
	// set up projection matrix
	//
	zNear = r_znear.GetFloat();

	ymax = zNear * tan(yfov * idMath::PI / 360.0f);
	ymin = -ymax;

	xmax = zNear * tan(xfov * idMath::PI / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	projectionMatrix[0] = 2 * zNear / width;
	projectionMatrix[4] = 0;
	projectionMatrix[8] = (xmax + xmin) / width;	// normally 0
	projectionMatrix[12] = 0;

	projectionMatrix[1] = 0;
	projectionMatrix[5] = 2 * zNear / height;
	projectionMatrix[9] = (ymax + ymin) / height;	// normally 0
	projectionMatrix[13] = 0;

	// this is the far-plane-at-infinity formulation
	projectionMatrix[2] = 0;
	projectionMatrix[6] = 0;
	projectionMatrix[10] = -1;
	projectionMatrix[14] = -2 * zNear;

	projectionMatrix[3] = 0;
	projectionMatrix[7] = 0;
	projectionMatrix[11] = -1;
	projectionMatrix[15] = 0;

	GL_ProjectionMatrix.Load(projectionMatrix);
}

void RenderPreview::Cam_Draw() {
	if (!active_brushes.next) {
		return;					// not valid yet
	}

	// set the sound origin for both simple draw and rendered mode
	// the editor uses opposite pitch convention
	idMat3	axis = idAngles(-m_Camera.angles.pitch, m_Camera.angles.yaw, m_Camera.angles.roll).ToMat3();
	g_qeglobals.sw->PlaceListener(m_Camera.origin, axis, 0, Sys_Milliseconds(), "Undefined");

	if (renderMode) {
		Cam_Render();
	}

	SetProjectionMatrix();
	g_qeglobals.surfaceBuffer.Clear();
	g_qeglobals.lineBuffer.Clear();
	g_qeglobals.pointBuffer.Clear();

	GL_ProjectionMatrix.Rotate(-90.0f, 1.f, 0.f, 0.f); // put Z going up
	GL_ProjectionMatrix.Rotate(90.0f, 0.f, 0.f, 1.f); // put Z going up
	GL_ProjectionMatrix.Rotate(m_Camera.angles[0], 0, 1, 0);
	GL_ProjectionMatrix.Rotate(-m_Camera.angles[1], 0, 0, 1);
	GL_ProjectionMatrix.Translate(-m_Camera.origin[0], -m_Camera.origin[1], -m_Camera.origin[2]);

	Cam_BuildMatrix();

	const brush_t *pList = (g_bClipMode && g_pSplitList) ? g_pSplitList : &selected_brushes;

	g_qeglobals.surfaceBuffer.Clear();
	for (auto brush = active_brushes.next; brush != &active_brushes; brush = brush->next) {

		if (CullBrush(brush, false)) {
			continue;
		}

		if (FilterBrush(brush)) {
			continue;
		}

		if (renderMode) {
			if (!(entityMode && brush->owner->eclass->fixedsize)) {
				continue;
			}
		}

		setGLMode(m_Camera.mode);
		Brush_Draw(brush, false);
	}

	if (!renderMode) {
		// draw normally
		for (auto brush = pList->next; brush != pList; brush = brush->next) {
			setGLMode(m_Camera.mode);
			Brush_Draw(brush, true);
		}
	}

	g_qeglobals.surfaceBuffer.Commit();
	g_qeglobals.lineBuffer.Commit();
	g_qeglobals.pointBuffer.Commit();

	g_qeglobals.surfaceBuffer.Clear();
	g_qeglobals.lineBuffer.Clear();
	g_qeglobals.pointBuffer.Clear();


	const idVec4 color = idVec4(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES], 0.25f);

	setGLMode(m_Camera.mode);


	glEnable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	globalImages->BindNull();

	if (renderMode) {
		for (auto brush = pList->next; brush != pList; brush = brush->next) {
			if (brush->pPatch /*|| brush->modelHandle > 0*/) {
				DrawPatchMeshWireframe(brush->pPatch, idVec3(1, 1, 1));
			}
		}
	}

	for (auto brush = pList->next; brush != pList; brush = brush->next) {
		if (brush->pPatch) {
			DrawPatchMeshSolid(brush->pPatch, color);
			DrawPatchMeshWireframe(brush->pPatch, idVec3(1, 1, 1));
			DrawPatchMeshHandles(brush->pPatch);
			continue;
		}

		if (brush->modelHandle > 0) {
			continue;
		}

		if (brush->owner->eclass->entityModel) {
			continue;
		}

		for (auto face = brush->brush_faces; face; face = face->next) {
			Face_Draw(face, color);
		}
	}

	if (!renderMode) {
		for (const auto& selected : g_selectedFaces) {
			const face_t *selFace = selected.GetFace();
			Face_Draw(selFace, color);
		}
	}

	g_qeglobals.surfaceBuffer.Commit();
	g_qeglobals.surfaceBuffer.Clear();

	if (!renderMode) {
		for (const auto& selected : g_selectedFaces) {
			face_t	*selFace = selected.GetFace();
			DrawAxial(selFace);
		}
	}

	g_qeglobals.lineBuffer.Commit();

	// non-zbuffered outline
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	if (renderMode) {
		for (const auto& selected : g_selectedFaces) {
			const face_t *selFace = selected.GetFace();
			Face_DrawOutline(selFace, idVec3(1, 0, 0));
		}
	}

	for (auto brush = pList->next; brush != pList; brush = brush->next) {
		if (brush->pPatch || brush->modelHandle > 0) {
			continue;
		}

		for (auto face = brush->brush_faces; face; face = face->next) {
			Face_DrawOutline(face, idVec3(1, 1, 1));
		}
	}

	g_qeglobals.lineBuffer.Commit();
	g_qeglobals.lineBuffer.Clear();

	// edge / vertex flags
	fhImmediateMode im;
	if (g_qeglobals.d_select_mode == sel_vertex) {
		glPointSize(4);

		im.Color3f(0, 1, 0);
		im.Begin(GL_POINTS);
		for (int i = 0; i < g_qeglobals.d_numpoints; i++) {
			im.Vertex3fv(g_qeglobals.d_points[i].ToFloatPtr());
		}

		im.End();
		glPointSize(1);
	}
	else if (g_qeglobals.d_select_mode == sel_edge) {
		float	*v1, *v2;

		glPointSize(4);
		im.Color3f(0, 0, 1);
		im.Begin(GL_POINTS);
		for (int i = 0; i < g_qeglobals.d_numedges; i++) {
			v1 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p1].ToFloatPtr();
			v2 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p2].ToFloatPtr();
			im.Vertex3f((v1[0] + v2[0]) * 0.5f, (v1[1] + v2[1]) * 0.5f, (v1[2] + v2[2]) * 0.5f);
		}

		im.End();
		glPointSize(1);
	}

	g_splineList->draw(static_cast<bool>(g_qeglobals.d_select_mode == sel_addpoint || g_qeglobals.d_select_mode == sel_editpoint));

	if (g_qeglobals.selectObject && (g_qeglobals.d_select_mode == sel_addpoint || g_qeglobals.d_select_mode == sel_editpoint)) {
		g_qeglobals.selectObject->drawSelection();
	}

	// draw pointfile
	glEnable(GL_DEPTH_TEST);

	DrawPathLines();
	Pointfile_Draw();

	g_qeglobals.lineBuffer.Commit();

	//
	// bind back to the default texture so that we don't have problems elsewhere
	// using/modifying texture maps between contexts
	//
	globalImages->BindNull();

	glFinish();
	QE_CheckOpenGLForErrors();

	if (!renderMode) {
		// clean up any deffered tri's
		R_ToggleSmpFrame();
	}
}

void RenderPreview::BuildEntityRenderState(entity_t *ent, bool update) {
	const char	*v;
	idDict		spawnArgs;
	const char	*name = NULL;

	ent->UpdateSoundEmitter();

	// delete the existing def if we aren't creating a brand new world
	if (!update) {
		if (ent->lightDef >= 0) {
			g_qeglobals.rw->FreeLightDef(ent->lightDef);
			ent->lightDef = -1;
		}

		if (ent->modelDef >= 0) {
			g_qeglobals.rw->FreeEntityDef(ent->modelDef);
			ent->modelDef = -1;
		}
	}

	// if an entity doesn't have any brushes at all, don't do anything
	if (ent->brushes.onext == &ent->brushes) {
		return;
	}

	// if the brush isn't displayed (filtered or culled), don't do anything
	if (FilterBrush(ent->brushes.onext)) {
		return;
	}

	spawnArgs = ent->epairs;
	if (ent->eclass->defArgs.FindKey("model")) {
		spawnArgs.Set("model", ent->eclass->defArgs.GetString("model"));
	}

	// any entity can have a model
	name = ent->ValueForKey("name");
	v = spawnArgs.GetString("model");
	if (v && *v) {
		renderEntity_t	refent;

		refent.referenceSound = ent->soundEmitter;

		if (!stricmp(name, v)) {
			// build the model from brushes
			idTriList tris(1024);
			idMatList mats(1024);

			for (brush_t *b = ent->brushes.onext; b != &ent->brushes; b = b->onext) {
				Brush_ToTris(b, &tris, &mats, false, true);
			}

			if (ent->modelDef >= 0) {
				g_qeglobals.rw->FreeEntityDef(ent->modelDef);
				ent->modelDef = -1;
			}

			idRenderModel *bmodel = renderModelManager->FindModel(name);

			if (bmodel) {
				renderModelManager->RemoveModel(bmodel);
				renderModelManager->FreeModel(bmodel);
			}

			bmodel = renderModelManager->AllocModel();

			bmodel->InitEmpty(name);

			// add the surfaces to the renderModel
			modelSurface_t	surf;
			for (int i = 0; i < tris.Num(); i++) {
				surf.geometry = tris[i];
				surf.shader = mats[i];
				bmodel->AddSurface(surf);
			}

			bmodel->FinishSurfaces();

			renderModelManager->AddModel(bmodel);

			// FIXME: brush entities
			gameEdit->ParseSpawnArgsToRenderEntity(&spawnArgs, &refent);

			ent->modelDef = g_qeglobals.rw->AddEntityDef(&refent);

		}
		else {
			// use the game's epair parsing code so
			// we can use the same renderEntity generation
			gameEdit->ParseSpawnArgsToRenderEntity(&spawnArgs, &refent);
			idRenderModelMD5 *md5 = dynamic_cast<idRenderModelMD5 *>(refent.hModel);
			if (md5) {
				idStr str;
				spawnArgs.GetString("anim", "idle", str);
				refent.numJoints = md5->NumJoints();
				if (update && refent.joints) {
					Mem_Free16(refent.joints);
				}
				refent.joints = (idJointMat *)Mem_Alloc16(refent.numJoints * sizeof(*refent.joints));
				const idMD5Anim *anim = gameEdit->ANIM_GetAnimFromEntityDef(spawnArgs.GetString("classname"), str);
				int frame = spawnArgs.GetInt("frame") + 1;
				if (frame < 1) {
					frame = 1;
				}
				const idVec3 &offset = gameEdit->ANIM_GetModelOffsetFromEntityDef(spawnArgs.GetString("classname"));
				gameEdit->ANIM_CreateAnimFrame(md5, anim, refent.numJoints, refent.joints, (frame * 1000) / 24, offset, false);
			}
			if (ent->modelDef >= 0) {
				g_qeglobals.rw->FreeEntityDef(ent->modelDef);
			}
			ent->modelDef = g_qeglobals.rw->AddEntityDef(&refent);
		}
	}

	// check for lightdefs
	if (!(ent->eclass->nShowFlags & ECLASS_LIGHT)) {
		return;
	}

	if (spawnArgs.GetBool("start_off")) {
		return;
	}
	// use the game's epair parsing code so
	// we can use the same renderLight generation

	renderLight_t	lightParms;

	gameEdit->ParseSpawnArgsToRenderLight(&spawnArgs, &lightParms);
	lightParms.referenceSound = ent->soundEmitter;

	if (update && ent->lightDef >= 0) {
		g_qeglobals.rw->UpdateLightDef(ent->lightDef, &lightParms);
	}
	else {
		if (ent->lightDef >= 0) {
			g_qeglobals.rw->FreeLightDef(ent->lightDef);
		}
		ent->lightDef = g_qeglobals.rw->AddLightDef(&lightParms);
	}
}


int Brush_TransformModel(brush_t *brush, idTriList *tris, idMatList *mats) {
	int ret = 0;
	if (brush->modelHandle > 0) {
		idRenderModel *model = brush->modelHandle;
		if (model) {
			float	a = brush->owner->FloatForKey("angle");
			float	s, c;
			//FIXME: support full rotation matrix
			bool matrix = false;
			if (a) {
				s = sin(DEG2RAD(a));
				c = cos(DEG2RAD(a));
			}
			idMat3 mat;
			if (brush->owner->GetMatrixForKey("rotation", mat)) {
				matrix = true;
			}


			for (int i = 0; i < model->NumSurfaces(); i++) {
				const modelSurface_t	*surf = model->Surface(i);
				srfTriangles_t	*tri = surf->geometry;
				srfTriangles_t *tri2 = R_CopyStaticTriSurf(tri);
				for (int j = 0; j < tri2->numVerts; j++) {
					idVec3	v;
					if (matrix) {
						v = tri2->verts[j].xyz * brush->owner->rotation + brush->owner->origin;
					}
					else {
						v = tri2->verts[j].xyz;
						VectorAdd(v, brush->owner->origin, v);
						float x = v[0];
						float y = v[1];
						if (a) {
							float	x2 = (((x - brush->owner->origin[0]) * c) - ((y - brush->owner->origin[1]) * s)) + brush->owner->origin[0];
							float	y2 = (((x - brush->owner->origin[0]) * s) + ((y - brush->owner->origin[1]) * c)) + brush->owner->origin[1];
							x = x2;
							y = y2;
						}
						v[0] = x;
						v[1] = y;
					}
					tri2->verts[j].xyz = v;
				}
				tris->Append(tri2);
				mats->Append(surf->shader);
			}
			return model->NumSurfaces();
		}
	}
	return ret;
}

int Brush_ToTris(brush_t *brush, idTriList *tris, idMatList *mats, bool models, bool bmodel) {
	int i, j;
	srfTriangles_t	*tri;
	//
	// patches
	//
	if (brush->modelHandle > 0) {
		if (!models) {
			return 0;
		}
		else {
			return Brush_TransformModel(brush, tris, mats);
		}
	}

	int numSurfaces = 0;

	if (brush->owner->eclass->fixedsize && !brush->entityModel) {
		return NULL;
	}

	if (brush->pPatch) {
		patchMesh_t *pm;
		int			width, height;

		pm = brush->pPatch;

		// build a patch mesh
		idSurface_Patch *cp = new idSurface_Patch(pm->width * 6, pm->height * 6);
		cp->SetSize(pm->width, pm->height);
		for (i = 0; i < pm->width; i++) {
			for (j = 0; j < pm->height; j++) {
				(*cp)[j*cp->GetWidth() + i].xyz = pm->ctrl(i, j).xyz;
				(*cp)[j*cp->GetWidth() + i].st = pm->ctrl(i, j).st;
			}
		}

		// subdivide it
		if (pm->explicitSubdivisions) {
			cp->SubdivideExplicit(pm->horzSubdivisions, pm->vertSubdivisions, true);
		}
		else {
			cp->Subdivide(DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true);
		}
		width = cp->GetWidth();
		height = cp->GetHeight();

		// convert to srfTriangles
		tri = R_AllocStaticTriSurf();
		tri->numVerts = width * height;
		tri->numIndexes = 6 * (width - 1) * (height - 1);
		R_AllocStaticTriSurfVerts(tri, tri->numVerts);
		R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);
		for (i = 0; i < tri->numVerts; i++) {
			tri->verts[i] = (*cp)[i];
			if (bmodel) {
				tri->verts[i].xyz -= brush->owner->origin;
			}
		}

		tri->numIndexes = 0;
		for (i = 1; i < width; i++) {
			for (j = 1; j < height; j++) {
				tri->indexes[tri->numIndexes++] = (j - 1) * width + i;
				tri->indexes[tri->numIndexes++] = (j - 1) * width + i - 1;
				tri->indexes[tri->numIndexes++] = j * width + i - 1;

				tri->indexes[tri->numIndexes++] = j * width + i;
				tri->indexes[tri->numIndexes++] = (j - 1) * width + i;
				tri->indexes[tri->numIndexes++] = j * width + i - 1;
			}
		}

		delete cp;

		tris->Append(tri);
		mats->Append(pm->d_texture);
		//surfaces[numSurfaces] = tri;
		//materials[numSurfaces] = pm->d_texture;
		return 1;
	}

	//
	// normal brush
	//
	for (face_t *face = brush->brush_faces; face; face = face->next) {
		idWinding *w;

		w = face->face_winding;
		if (!w) {
			continue;	// freed or degenerate face
		}

		tri = R_AllocStaticTriSurf();
		tri->numVerts = w->GetNumPoints();
		tri->numIndexes = (w->GetNumPoints() - 2) * 3;
		R_AllocStaticTriSurfVerts(tri, tri->numVerts);
		R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);

		for (i = 0; i < tri->numVerts; i++) {

			tri->verts[i].Clear();

			tri->verts[i].xyz[0] = (*w)[i][0];
			tri->verts[i].xyz[1] = (*w)[i][1];
			tri->verts[i].xyz[2] = (*w)[i][2];

			if (bmodel) {
				tri->verts[i].xyz -= brush->owner->origin;
			}

			tri->verts[i].st[0] = (*w)[i][3];
			tri->verts[i].st[1] = (*w)[i][4];

			tri->verts[i].normal = face->plane.Normal();
		}

		tri->numIndexes = 0;
		for (i = 2; i < w->GetNumPoints(); i++) {
			tri->indexes[tri->numIndexes++] = 0;
			tri->indexes[tri->numIndexes++] = i - 1;
			tri->indexes[tri->numIndexes++] = i;
		}

		tris->Append(tri);
		mats->Append(face->d_texture);
		numSurfaces++;
	}

	return numSurfaces;
}

void Select_ToOBJ() {
	int i;
	CFileDialog dlgFile(FALSE, "obj", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Wavefront object files (*.obj)|*.obj||", g_pParentWnd);
	if (dlgFile.DoModal() == IDOK) {
		idTriList tris(1024);
		idMatList mats(1024);

		for (brush_t *b = selected_brushes.next; b != &selected_brushes; b = b->next) {

			if (b->hiddenBrush) {
				continue;
			}

			if (FilterBrush(b)) {
				continue;
			}

			Brush_ToTris(b, &tris, &mats, true, false);
		}

		char objFilePath[MAX_PATH];
		char mtlFilePath[MAX_PATH];

		sprintf(objFilePath, "%s", dlgFile.GetPathName().GetBuffer(0));
		StripExtension(objFilePath);
		sprintf(mtlFilePath, "%s.mtl", objFilePath);
		sprintf(objFilePath, "%s", dlgFile.GetPathName().GetBuffer(0));

		idFile *objFile = fileSystem->OpenExplicitFileWrite(objFilePath);
		idFile *mtlFile = fileSystem->OpenExplicitFileWrite(mtlFilePath);

		Tris_ToOBJ(tris, mats, objFile, mtlFile);

		fileSystem->CloseFile(objFile);
		fileSystem->CloseFile(mtlFile);

		for (i = 0; i < tris.Num(); i++) {
			R_FreeStaticTriSurf(tris[i]);
		}
		tris.Clear();
	}
}

void Select_ToCM() {
	CFileDialog dlgFile(FALSE, "lwo, ase", NULL, 0, "(*.lwo)|*.lwo|(*.ase)|*.ase|(*.ma)|*.ma||", g_pParentWnd);

	if (dlgFile.DoModal() == IDOK) {
		idMapEntity *mapEnt;
		idMapPrimitive *p;
		idStr name;

		name = fileSystem->OSPathToRelativePath(dlgFile.GetPathName());
		name.BackSlashesToSlashes();

		mapEnt = new idMapEntity();
		mapEnt->epairs.Set("name", name.c_str());

		for (brush_t *b = selected_brushes.next; b != &selected_brushes; b = b->next) {

			if (b->hiddenBrush) {
				continue;
			}

			if (FilterBrush(b)) {
				continue;
			}

			p = BrushToMapPrimitive(b, b->owner->origin);
			if (p) {
				mapEnt->AddPrimitive(p);
			}
		}

		collisionModelManager->WriteCollisionModelForMapEntity(mapEnt, name.c_str());

		delete mapEnt;
	}
}


/*
=================
BuildRendererState

Builds models, lightdefs, and modeldefs for the current editor data
so it can be rendered by the game renderSystem
=================
*/
void RenderPreview::BuildRendererState() {
	renderEntity_t	worldEntity;
	entity_t	*ent;
	brush_t		*brush;

	FreeRendererState();

	// the renderWorld holds all the references and defs
	g_qeglobals.rw->InitFromMap(NULL);

	// create the raw model for all the brushes
	int numBrushes = 0;
	int numSurfaces = 0;

	// the renderModel for the world holds all the geometry that isn't in an entity
	worldModel = renderModelManager->AllocModel();
	worldModel->InitEmpty("EditorWorldModel");

	for (brush_t *brushList = &active_brushes; brushList;
		brushList = (brushList == &active_brushes) ? &selected_brushes : NULL) {

		for (brush = brushList->next; brush != brushList; brush = brush->next) {

			if (brush->hiddenBrush) {
				continue;
			}

			if (FilterBrush(brush)) {
				continue;
			}

			if (CullBrush(brush, true)) {
				continue;
			}

			idTriList tris(1024);
			idMatList mats(1024);

			if (!IsBModel(brush)) {
				numSurfaces += Brush_ToTris(brush, &tris, &mats, false, false);
			}

			// add the surfaces to the renderModel
			modelSurface_t	surf;
			for (int i = 0; i < tris.Num(); i++) {
				surf.geometry = tris[i];
				surf.shader = mats[i];
				worldModel->AddSurface(surf);
			}

			numBrushes++;
		}
	}

	// bound and clean the triangles
	worldModel->FinishSurfaces();

	// the worldEntity just has the handle for the worldModel
	memset(&worldEntity, 0, sizeof(worldEntity));
	worldEntity.hModel = worldModel;
	worldEntity.axis = mat3_default;
	worldEntity.shaderParms[0] = 1;
	worldEntity.shaderParms[1] = 1;
	worldEntity.shaderParms[2] = 1;
	worldEntity.shaderParms[3] = 1;

	worldModelDef = g_qeglobals.rw->AddEntityDef(&worldEntity);

	// create the light and model entities exactly the way the game code would
	for (ent = entities.next; ent != &entities; ent = ent->next) {
		if (ent->brushes.onext == &ent->brushes) {
			continue;
		}

		if (CullBrush(ent->brushes.onext, true)) {
			continue;
		}

		if (Map_IsBrushFiltered(ent->brushes.onext)) {
			continue;
		}

		BuildEntityRenderState(ent, false);
	}

	//common->Printf("Render data used %d brushes\n", numBrushes);
	worldDirty = false;
	UpdateCaption();
}

/*
============================
RenderPreview::FreeRendererState

  Frees the render state data
============================
*/
void RenderPreview::FreeRendererState() {

	for (entity_t *ent = entities.next; ent != &entities; ent = ent->next) {
		if (ent->lightDef >= 0) {
			g_qeglobals.rw->FreeLightDef(ent->lightDef);
			ent->lightDef = -1;
		}

		if (ent->modelDef >= 0) {
			renderEntity_t *refent = const_cast<renderEntity_t *>(g_qeglobals.rw->GetRenderEntity(ent->modelDef));
			if (refent) {
				if (refent->callbackData) {
					Mem_Free(refent->callbackData);
					refent->callbackData = NULL;
				}
				if (refent->joints) {
					Mem_Free16(refent->joints);
					refent->joints = NULL;
				}
			}
			g_qeglobals.rw->FreeEntityDef(ent->modelDef);
			ent->modelDef = -1;
		}
	}

	if (worldModel) {
		renderModelManager->FreeModel(worldModel);
		worldModel = NULL;
	}

}


/*
========================
RenderPreview::UpdateCaption

  updates the caption based on rendermode and whether the render mode needs updated
========================
*/
void RenderPreview::UpdateCaption() {

	idStr strCaption;

	if (worldDirty) {
		strCaption = "*";
	}
	// FIXME:
	strCaption += (renderMode) ? "RENDER" : "CAM";
	if (renderMode) {
		strCaption += (rebuildMode) ? " (Realtime)" : "";
		strCaption += (entityMode) ? " +lights" : "";
		strCaption += (selectMode) ? " +selected" : "";
		strCaption += (animationMode) ? " +anim" : "";
	}
	strCaption += (soundMode) ? " +snd" : "";
	this->setWindowTitle(strCaption.c_str());	
}

/*
===========================
RenderPreview::ToggleRenderMode

	Toggles the render mode
===========================
*/
void RenderPreview::ToggleRenderMode() {
	renderMode ^= 1;
	UpdateCaption();
}

/*
===========================
RenderPreview::ToggleRebuildMode

	Toggles the rebuild mode
===========================
*/
void RenderPreview::ToggleRebuildMode() {
	rebuildMode ^= 1;
	UpdateCaption();
}

/*
===========================
RenderPreview::ToggleEntityMode

	Toggles the entity mode
===========================
*/
void RenderPreview::ToggleEntityMode() {
	entityMode ^= 1;
	UpdateCaption();
}


/*
===========================
RenderPreview::ToggleRenderMode

	Toggles the render mode
===========================
*/
void RenderPreview::ToggleAnimationMode() {
	animationMode ^= 1;
	if (animationMode) {
		//SetTimer(0, 10, NULL); //FIXME
	}
	else {
		//KillTimer(0); //FIXME
	}
	UpdateCaption();
}

/*
===========================
RenderPreview::ToggleSoundMode

	Toggles the sound mode
===========================
*/
void RenderPreview::ToggleSoundMode() {
	soundMode ^= 1;

	UpdateCaption();

	for (entity_t *ent = entities.next; ent != &entities; ent = ent->next) {
		ent->UpdateSoundEmitter();
	}
}

/*
===========================
RenderPreview::ToggleRenderMode

	Toggles the render mode
===========================
*/
void RenderPreview::ToggleSelectMode() {
	selectMode ^= 1;
	UpdateCaption();
}

/*
=========================
RenderPreview::MarkWorldDirty

  marks the render world as dirty
=========================
*/
void RenderPreview::MarkWorldDirty() {
	worldDirty = true;
	UpdateCaption();
}


/*
 =======================================================================================================================
	Cam_Render

	This used the renderSystem to draw a fully lit view of the world
 =======================================================================================================================
 */
void RenderPreview::Cam_Render() {
	if (!active_brushes.next) {
		return;					// not valid yet
	}

	// create the model, using explicit normals
	if (rebuildMode && worldDirty) {
		BuildRendererState();
	}

	const auto oldWindowWidth = glConfig.windowWidth;
	const auto oldWindowHeight = glConfig.windowHeight;
	const auto oldVidWidth = glConfig.vidWidth;
	const auto oldVidHeight = glConfig.vidHeight;

	glConfig.windowWidth = m_Camera.width;
	glConfig.windowHeight = m_Camera.height;

	// render it
	renderSystem->BeginFrame(m_Camera.width, m_Camera.height);

	renderView_t refdef;
	memset(&refdef, 0, sizeof(refdef));
	refdef.vieworg = m_Camera.origin;

	// the editor uses opposite pitch convention
	refdef.viewaxis = idAngles(-m_Camera.angles.pitch, m_Camera.angles.yaw, m_Camera.angles.roll).ToMat3();

	refdef.width = SCREEN_WIDTH;
	refdef.height = SCREEN_HEIGHT;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan((float)m_Camera.height / m_Camera.width) * idMath::M_RAD2DEG;

	// only set in animation mode to give a consistent look
	if (animationMode) {
		refdef.time = eventLoop->Milliseconds();
	}

	g_qeglobals.rw->RenderScene(&refdef);

	renderSystem->EndFrame();

	// get back to the editor state
	GL_ModelViewMatrix.LoadIdentity();
	glConfig.windowWidth = oldWindowWidth;
	glConfig.windowHeight = oldWindowHeight;
	glConfig.vidWidth = oldVidWidth;
	glConfig.vidHeight = oldVidHeight;
}

#if 0 //FIXME
void RenderPreview::OnTimer(UINT nIDEvent)
{
	if ((renderMode && animationMode) || nIDEvent == 1) {
		Sys_UpdateWindows(W_CAMERA);
	}
	if (nIDEvent == 1) {
		KillTimer(1);
	}

	if (!animationMode) {
		KillTimer(0);
	}
}
#endif

void RenderPreview::UpdateCameraView() {
	if (QE_SingleBrush(true, true)) {
		brush_t *b = selected_brushes.next;
		if (b->owner->eclass->nShowFlags & ECLASS_CAMERAVIEW) {
			// find the entity that targets this
			const char *name = b->owner->ValueForKey("name");
			entity_t *ent = FindEntity("target", name);
			if (ent) {
				if (!saveValid) {
					saveOrg = m_Camera.origin;
					saveAng = m_Camera.angles;
					saveValid = true;
				}
				idVec3 v = b->owner->origin - ent->origin;
				v.Normalize();
				idAngles ang = v.ToMat3().ToAngles();
				ang.pitch = -ang.pitch;
				ang.roll = 0.0f;
				SetView(ent->origin, ang);
				Cam_BuildMatrix();
				Sys_UpdateWindows(W_CAMERA);
				return;
			}
		}
	}
	if (saveValid) {
		SetView(saveOrg, saveAng);
		Cam_BuildMatrix();
		Sys_UpdateWindows(W_CAMERA);
		saveValid = false;
	}
}

