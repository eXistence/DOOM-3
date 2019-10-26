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

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "XYWnd.h"
#include "CamWnd.h"
#include "splines.h"

#include "../../renderer/tr_local.h"
#include "../../renderer/model_local.h"	// for idRenderModelMD5
#include "../../renderer/ImmediateMode.h"

void DrawPathLines();

int g_axialAnchor = -1;
int g_axialDest = -1;
bool g_bAxialMode = false;

static brush_t *g_pSplitList = NULL;

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
	} else {
		g_axialDest = 0;
		g_axialAnchor = 0;
	}
}

// CCamWnd
IMPLEMENT_DYNCREATE(CCamWnd, CWnd);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CCamWnd::CCamWnd() {
	worldDirty = true;
	worldModel = NULL;
	renderMode = false;
	rebuildMode = false;
	entityMode = false;
	animationMode = false;
	selectMode = false;
	soundMode = false;
	saveValid = false;	
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CCamWnd::~CCamWnd() {
}

BEGIN_MESSAGE_MAP(CCamWnd, CWnd)
//{{AFX_MSG_MAP(CCamWnd)
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_KEYUP()
	ON_WM_NCCALCSIZE()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/*
 =======================================================================================================================
 =======================================================================================================================
 */
LONG WINAPI CamWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RECT	rect;

	GetClientRect(hWnd, &rect);

	switch (uMsg)
	{
		case WM_KILLFOCUS:
		case WM_SETFOCUS:
			SendMessage(hWnd, WM_NCACTIVATE, uMsg == WM_SETFOCUS, 0);
			return 0;

		case WM_NCCALCSIZE: // don't let windows copy pixels
			DefWindowProc(hWnd, uMsg, wParam, lParam);
			return WVR_REDRAW;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//
// =======================================================================================================================
//    CCamWnd message handlers
// =======================================================================================================================
//
BOOL CCamWnd::PreCreateWindow(CREATESTRUCT &cs) {
	WNDCLASS	wc;
	HINSTANCE	hInstance = AfxGetInstanceHandle();
	if (::GetClassInfo(hInstance, CAMERA_WINDOW_CLASS, &wc) == FALSE) {
		// Register a new class
		memset(&wc, 0, sizeof(wc));

		// wc.style = CS_NOCLOSE | CS_OWNDC;
		wc.style = CS_NOCLOSE;
		wc.lpszClassName = CAMERA_WINDOW_CLASS;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpfnWndProc = CamWndProc;
		if (AfxRegisterClass(&wc) == FALSE) {
			Error("CCamWnd RegisterClass: failed");
		}
	}

	cs.lpszClass = CAMERA_WINDOW_CLASS;
	cs.lpszName = "CAM";
	if (cs.style != QE3_CHILDSTYLE) {
		cs.style = QE3_SPLITTER_STYLE;
	}

	BOOL	bResult = CWnd::PreCreateWindow(cs);

	//
	// See if the class already exists and if not then we need to register our new
	// window class.
	//
	return bResult;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
	g_pParentWnd->HandleKey(nChar, nRepCnt, nFlags);
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnPaint() {
	CPaintDC	dc(this);	// device context for painting
	bool		bPaint = true;

	if (!wglMakeCurrent(dc.m_hDC, win32.hGLRC)) {
		common->Printf("ERROR: wglMakeCurrent failed..\n ");
		common->Printf("Please restart " EDITOR_WINDOWTEXT " if the camera view is not working\n");
		return;
	}

	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
	glClearColor(g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][0], g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][1], g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fhImmediateMode::ResetStats();
	QE_CheckOpenGLForErrors();

	g_pSplitList = NULL;
	if (g_bClipMode) {
		if (g_Clip1.Set() && g_Clip2.Set()) {
			g_pSplitList = ((g_pParentWnd->ActiveXY()->GetViewType() == ViewType::XZ) ? !g_bSwitch : g_bSwitch) ? &g_brBackSplits : &g_brFrontSplits;
		}
	}

	Cam_Draw();
	QE_CheckOpenGLForErrors();
	wglSwapBuffers(dc.m_hDC);

}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnDestroy() {
	CWnd::OnDestroy();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnClose() {
	CWnd::OnClose();
}

extern void Select_RotateTexture(float amt, bool absolute);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnMouseMove(UINT nFlags, CPoint point) {
	CRect	r;
	GetClientRect(r);
	if	(GetCapture() == this && (GetAsyncKeyState(VK_MENU) & 0x8000) && !((GetAsyncKeyState(VK_SHIFT) & 0x8000) || (GetAsyncKeyState(VK_CONTROL) & 0x8000))) {
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
			Select_RotateTexture((float)point.y - m_ptLastCursor.y);
		}
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
			Select_ScaleTexture((float)point.x - m_ptLastCursor.x, (float)m_ptLastCursor.y - point.y);
		}
		else {
			Select_ShiftTexture((float)point.x - m_ptLastCursor.x, (float)m_ptLastCursor.y - point.y);
		}
	}
	else {
		Cam_MouseMoved(point.x, r.bottom - 1 - point.y, nFlags);
	}

	m_ptLastCursor = point;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnLButtonDown(UINT nFlags, CPoint point) {
	m_ptLastCursor = point;
	OriginalMouseDown(nFlags, point);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnLButtonUp(UINT nFlags, CPoint point) {
	OriginalMouseUp(nFlags, point);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnMButtonDown(UINT nFlags, CPoint point) {
	OriginalMouseDown(nFlags, point);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnMButtonUp(UINT nFlags, CPoint point) {
	OriginalMouseUp(nFlags, point);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnRButtonDown(UINT nFlags, CPoint point) {
	OriginalMouseDown(nFlags, point);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnRButtonUp(UINT nFlags, CPoint point) {
	OriginalMouseUp(nFlags, point);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int CCamWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	if (CWnd::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	CDC *pDC = GetDC();
	HDC hDC = pDC->GetSafeHdc();

	QEW_SetupPixelFormat(hDC, true);

	// report OpenGL information
	common->Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	common->Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	common->Printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	//common->Printf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OriginalMouseUp(UINT nFlags, CPoint point) {
	CRect	r;
	GetClientRect(r);
	Cam_MouseUp(point.x, r.bottom - 1 - point.y, nFlags);
	if (!(nFlags & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON))) {
		ReleaseCapture();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OriginalMouseDown(UINT nFlags, CPoint point) {
	// if (GetTopWindow()->GetSafeHwnd() != GetSafeHwnd()) BringWindowToTop();
	CRect	r;
	GetClientRect(r);
	SetFocus();
	SetCapture();

	// if (!(GetAsyncKeyState(VK_MENU) & 0x8000))
	Cam_MouseDown(point.x, r.bottom - 1 - point.y, nFlags);
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */

void CCamWnd::Cam_ChangeFloor(bool up) {
	brush_t *b;
	float	d, bestd, current;
	idVec3	start, dir;

	idVec3 origin = m_Camera->GetOrigin();

	start[0] = origin[0];
	start[1] = origin[1];
	start[2] = HUGE_DISTANCE;
	dir[0] = dir[1] = 0;
	dir[2] = -1;

	current = HUGE_DISTANCE - (origin[2] - 72);
	if (up) {
		bestd = 0;
	}
	else {
		bestd = HUGE_DISTANCE*2;
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

	if (bestd == 0 || bestd == HUGE_DISTANCE*2) {
		return;
	}

	origin[2] += current - bestd;
	m_Camera->SetOrigin(origin);
	Sys_UpdateWindows(W_CAMERA | W_Z_OVERLAY);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_PositionDrag() {
	int x, y;
	Sys_GetCursorPos(&x, &y);
	if (x != m_ptCursor.x || y != m_ptCursor.y) {
		x -= m_ptCursor.x;
		y -= m_ptCursor.y;

		m_Camera->MoveCameraUpDown(-y);
		m_Camera->MoveCameraLeftRight(x);

		SetCursorPos(m_ptCursor.x, m_ptCursor.y);
		Sys_UpdateWindows(W_CAMERA | W_XY_OVERLAY);
	}
}

void CCamWnd::Cam_MouseLook() {
	CPoint current;

	GetCursorPos(&current);
	if (current.x != m_ptCursor.x || current.y != m_ptCursor.y) {
		current.x -= m_ptCursor.x;
		current.y -= m_ptCursor.y;

		const float sensitiviy = 0.25f;
		m_Camera->TurnCameraLeftRight(-current.x *sensitiviy);
		m_Camera->TurnCameraUpDown(current.y * sensitiviy);

		SetCursorPos(m_ptCursor.x, m_ptCursor.y);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_MouseControl(float dtime) {
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

	xf = (float)(m_ptButton.x - width / 2) / (width / 2);
	yf = (float)(m_ptButton.y - height / 2) / (height / 2);

	xl = width / 3;
	xh = xl * 2;
	yl = height / 3;
	yh = yl * 2;

	{
		xf *= 1.0f - idMath::Fabs(yf);
		if ( xf < 0.0f ) {
			xf += 0.1f;
			if ( xf > 0.0f ) {
				xf = 0.0f;
			}
		}
		else {
			xf -= 0.1f;
			if ( xf < 0.0f ) {
				xf = 0.0f;
			}
		}

		m_Camera->MoveCameraForwardBackward(yf * dtime * g_PrefsDlg.m_nMoveSpeed);
		m_Camera->TurnCameraLeftRight(xf * -dtime * g_PrefsDlg.m_nAngleSpeed);
	}

	Sys_UpdateWindows(W_CAMERA | W_XY);
	g_pParentWnd->PostMessage(WM_TIMER, 0, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_MouseDown(int x, int y, int buttons) {
	const float u = (float)(y - height / 2) / (width / 2);
	const float r = (float)(x - width / 2) / (width / 2);
	const idVec3 dir = m_Camera->GetRayFromPoint(r, u);

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
			Cam_MouseControl( 0.1f );
		}
		else {
			// something global needs to track which window is responsible for stuff
			Patch_SetView(W_CAMERA);
			Drag_Begin(x, y, buttons, m_Camera->GetViewRight(), m_Camera->GetViewUp(), m_Camera->GetOrigin(), dir);
		}

		return;
	}

	if ( buttons == MK_RBUTTON ) {
		Cam_MouseControl( 0.1f );
		return;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_MouseUp(int x, int y, int buttons) {
	m_nCambuttonstate = 0;
	Drag_MouseUp(buttons);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::Cam_MouseMoved(int x, int y, int buttons) {
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
	else if ( buttons == (MK_RBUTTON | MK_CONTROL | MK_SHIFT) ) {
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
void CCamWnd::InitCull() {
	const float xfov = 90;
	const float yfov = 2 * atan((float)height / width) * idMath::M_RAD2DEG;

	const idVec3 vieworg = m_Camera->GetOrigin();
	const idAngles viewAngles = m_Camera->GetAngles();
	//const idMat3 viewaxis = m_Camera.angles.ToMat3();
	const auto viewaxis = idAngles(-viewAngles.pitch, viewAngles.yaw, viewAngles.roll).ToMat3();

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
		}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool CCamWnd::CullBrush(brush_t *b, bool cubicOnly) {

	if ( b->forceVisibile ) {
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
		idVec3 distance = bounds.GetCenter() - m_Camera->GetOrigin();

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
static void setGLMode(CameraDrawMode mode) {
	switch (mode)
	{
	case CameraDrawMode::Wireframe:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		globalImages->BindNull();
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		break;

	case CameraDrawMode::Solid:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		globalImages->BindNull();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		break;

	case CameraDrawMode::Textures:
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


extern void glLabeledPoint(idVec4 &color, idVec3 &point, float size, const char *label);
static void DrawAxial(face_t *selFace) {
	if (g_bAxialMode) {
		idVec3 points[4];

		for (int j = 0; j < selFace->face_winding->GetNumPoints(); j++) {
			glLabeledPoint(idVec4(1, 1, 1, 1), (*selFace->face_winding)[j].ToVec3(), 3, va("%i", j));
		}

		ValidateAxialPoints();
		points[0] = (*selFace->face_winding)[g_axialAnchor].ToVec3();
		VectorMA (points[0], 1, selFace->plane, points[0]);
		VectorMA (points[0], 4, selFace->plane, points[1]);
		points[3] = (*selFace->face_winding)[g_axialDest].ToVec3();
		VectorMA (points[3], 1, selFace->plane, points[3]);
		VectorMA (points[3], 4, selFace->plane, points[2]);
		glLabeledPoint(idVec4(1, 0, 0, 1), points[1], 3, "Anchor");
		glLabeledPoint(idVec4(1, 1, 0, 1), points[2], 3, "Dest");
    fhImmediateMode im;
		im.Begin (GL_LINE_STRIP);
		im.Vertex3fv( points[0].ToFloatPtr() );
		im.Vertex3fv( points[1].ToFloatPtr() );
		im.Vertex3fv( points[2].ToFloatPtr() );
		im.Vertex3fv( points[3].ToFloatPtr() );
		im.End();
	}
}


/*
 =======================================================================================================================
    Cam_Draw
 =======================================================================================================================
 */
fhRenderMatrix CCamWnd::CreateProjectionMatrix() const {
	float xfov = 90;
	float yfov = 2 * atan((float)height / width) * idMath::M_RAD2DEG;

	float	xmin, xmax, ymin, ymax;
	float	width, height;
	float	zNear;
	float	projectionMatrix[16];

	//
	// set up projection matrix
	//
	zNear	= r_znear.GetFloat();

	ymax = zNear * tan( yfov * idMath::PI / 360.0f );
	ymin = -ymax;

	xmax = zNear * tan( xfov * idMath::PI / 360.0f );
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	projectionMatrix[0] = 2 * zNear / width;
	projectionMatrix[4] = 0;
	projectionMatrix[8] = ( xmax + xmin ) / width;	// normally 0
	projectionMatrix[12] = 0;

	projectionMatrix[1] = 0;
	projectionMatrix[5] = 2 * zNear / height;
	projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
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

	
	return fhRenderMatrix(projectionMatrix);
}

void CCamWnd::Cam_Draw() {
	if (!active_brushes.next) {
		return;					// not valid yet
	}

	g_qeglobals.sw->PlaceListener(m_Camera->GetOrigin(), m_Camera->GetRotationMatrix(), 0, Sys_Milliseconds(), "Undefined" );

	if (renderMode) {
		Cam_Render();
	}

	g_qeglobals.surfaceBuffer.Clear();
	g_qeglobals.lineBuffer.Clear();
	g_qeglobals.pointBuffer.Clear();

	const auto projectionMatrix = CreateProjectionMatrix();	
	const auto viewMatrix = m_Camera->CreateViewMatrix();	
	const auto viewProjectionMatrix = projectionMatrix * viewMatrix;
	GL_ProjectionMatrix.Load(viewProjectionMatrix.ToFloatPtr());	

	InitCull();

	const brush_t *pList = (g_bClipMode && g_pSplitList) ? g_pSplitList : &selected_brushes;

	g_qeglobals.surfaceBuffer.Clear();
	for (auto brush = active_brushes.next; brush != &active_brushes; brush = brush->next) {

		if ( CullBrush(brush, false) ) {
			continue;
		}

		if ( FilterBrush(brush) ) {
			continue;
		}

		if (renderMode) {
			if (!(entityMode && brush->owner->eclass->fixedsize)) {
				continue;
			}
		}

		setGLMode(m_Camera->GetDrawMode());
		Brush_Draw(brush, false);
	}

	if (!renderMode) {
		// draw normally
		for (auto brush = pList->next; brush != pList; brush = brush->next) {
			setGLMode(m_Camera->GetDrawMode());
			Brush_Draw(brush, true);
		}
	}

	g_qeglobals.surfaceBuffer.Commit();
	g_qeglobals.lineBuffer.Commit();
	g_qeglobals.pointBuffer.Commit();

	g_qeglobals.surfaceBuffer.Clear();
	g_qeglobals.lineBuffer.Clear();
	g_qeglobals.pointBuffer.Clear();


	const idVec4 color = idVec4(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES], 0.25f );

	setGLMode(m_Camera->GetDrawMode());


	glEnable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	globalImages->BindNull();

	if(renderMode) {
		for (auto brush = pList->next; brush != pList; brush = brush->next) {
			if (brush->pPatch /*|| brush->modelHandle > 0*/) {
				DrawPatchMeshWireframe(brush->pPatch, idVec3(1,1,1));
			}
		}
	}

	for (auto brush = pList->next; brush != pList; brush = brush->next) {
		if(brush->pPatch) {
			DrawPatchMeshSolid(brush->pPatch, color);
			DrawPatchMeshWireframe(brush->pPatch, idVec3(1,1,1));
			DrawPatchMeshHandles(brush->pPatch);
			continue;
		}

		if ( brush->modelHandle > 0) {
			continue;
		}

		if ( brush->owner->eclass->entityModel ) {
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
			Face_DrawOutline( selFace, idVec3(1,0,0));
		}
	}

	for (auto brush = pList->next; brush != pList; brush = brush->next) {
		if (brush->pPatch || brush->modelHandle > 0) {
			continue;
		}

		for (auto face = brush->brush_faces; face; face = face->next) {
			Face_DrawOutline( face, idVec3(1,1,1));
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
			im.Vertex3fv( g_qeglobals.d_points[i].ToFloatPtr() );
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
			im.Vertex3f( (v1[0] + v2[0]) * 0.5f, (v1[1] + v2[1]) * 0.5f, (v1[2] + v2[2]) * 0.5f );
		}

		im.End();
		glPointSize(1);
	}

	g_splineList->draw (static_cast<bool>(g_qeglobals.d_select_mode == sel_addpoint || g_qeglobals.d_select_mode == sel_editpoint));

	if ( g_qeglobals.selectObject && (g_qeglobals.d_select_mode == sel_addpoint || g_qeglobals.d_select_mode == sel_editpoint) ) {
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

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnSize(UINT nType, int cx, int cy) {
	CWnd::OnSize(nType, cx, cy);

	CRect	rect;
	GetClientRect(rect);
	width = rect.right;
	height = rect.bottom;
	InvalidateRect(NULL, false);
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CCamWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) {
	g_pParentWnd->HandleKey(nChar, nRepCnt, nFlags, false);
}

bool IsBModel(brush_t *b) {
	const char *v = b->owner->ValueForKey( "model" );
	if (v && *v) {
		const char *n = b->owner->ValueForKey("name");
		return (stricmp( n, v ) == 0);
	}
	return false;
}

/*
================
BuildEntityRenderState

Creates or updates modelDef and lightDef for an entity
================
*/
int Brush_ToTris(brush_t *brush, idTriList *tris, idMatList *mats, bool models, bool bmodel);

void CCamWnd::BuildEntityRenderState( entity_t *ent, bool update) {
	const char	*v;
	idDict		spawnArgs;
	const char	*name = NULL;

	ent->UpdateSoundEmitter();

	// delete the existing def if we aren't creating a brand new world
	if ( !update ) {
		if ( ent->lightDef >= 0 ) {
			g_qeglobals.rw->FreeLightDef( ent->lightDef );
			ent->lightDef = -1;
		}

		if ( ent->modelDef >= 0 ) {
			g_qeglobals.rw->FreeEntityDef( ent->modelDef );
			ent->modelDef = -1;
		}
	}

	// if an entity doesn't have any brushes at all, don't do anything
	if ( ent->brushes.onext == &ent->brushes ) {
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
	name = ent->ValueForKey( "name" );
	v = spawnArgs.GetString("model");
	if ( v && *v ) {
		renderEntity_t	refent;

		refent.referenceSound = ent->soundEmitter;

		if ( !stricmp( name, v ) ) {
			// build the model from brushes
			idTriList tris(1024);
			idMatList mats(1024);

			for (brush_t *b = ent->brushes.onext; b != &ent->brushes; b = b->onext) {
				Brush_ToTris( b, &tris, &mats, false, true);
			}

			if ( ent->modelDef >= 0 ) {
				g_qeglobals.rw->FreeEntityDef( ent->modelDef );
				ent->modelDef = -1;
			}

			idRenderModel *bmodel = renderModelManager->FindModel( name );

			if ( bmodel ) {
				renderModelManager->RemoveModel( bmodel );
				renderModelManager->FreeModel( bmodel );
			}

			bmodel = renderModelManager->AllocModel();

			bmodel->InitEmpty( name );

			// add the surfaces to the renderModel
			modelSurface_t	surf;
			for ( int i = 0 ; i < tris.Num() ; i++ ) {
				surf.geometry = tris[i];
				surf.shader = mats[i];
				bmodel->AddSurface( surf );
			}

			bmodel->FinishSurfaces();

			renderModelManager->AddModel( bmodel );

			// FIXME: brush entities
			gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &refent );

			ent->modelDef = g_qeglobals.rw->AddEntityDef( &refent );

		} else {
			// use the game's epair parsing code so
			// we can use the same renderEntity generation
			gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &refent );
			idRenderModelMD5 *md5 = dynamic_cast<idRenderModelMD5 *>( refent.hModel );
			if (md5) {
				idStr str;
				spawnArgs.GetString("anim", "idle", str);
				refent.numJoints = md5->NumJoints();
				if ( update && refent.joints ) {
					Mem_Free16( refent.joints );
				}
				refent.joints = ( idJointMat * )Mem_Alloc16( refent.numJoints * sizeof( *refent.joints ) );
				const idMD5Anim *anim = gameEdit->ANIM_GetAnimFromEntityDef(spawnArgs.GetString("classname"), str);
				int frame = spawnArgs.GetInt("frame") + 1;
				if ( frame < 1 ) {
					frame = 1;
				}
				const idVec3 &offset = gameEdit->ANIM_GetModelOffsetFromEntityDef( spawnArgs.GetString("classname") );
				gameEdit->ANIM_CreateAnimFrame( md5, anim, refent.numJoints, refent.joints, ( frame * 1000 ) / 24, offset, false );
			}
			if (ent->modelDef >= 0) {
				g_qeglobals.rw->FreeEntityDef( ent->modelDef );
			}
			ent->modelDef = g_qeglobals.rw->AddEntityDef( &refent );
		}
	}

	// check for lightdefs
	if (!(ent->eclass->nShowFlags & ECLASS_LIGHT)) {
		return;
	}

	if ( spawnArgs.GetBool( "start_off" ) ) {
		return;
	}
	// use the game's epair parsing code so
	// we can use the same renderLight generation

	renderLight_t	lightParms;

	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &lightParms );
	lightParms.referenceSound = ent->soundEmitter;

	if (update && ent->lightDef >= 0) {
		g_qeglobals.rw->UpdateLightDef( ent->lightDef, &lightParms );
	} else {
		if (ent->lightDef >= 0) {
			g_qeglobals.rw->FreeLightDef(ent->lightDef);
		}
		ent->lightDef = g_qeglobals.rw->AddLightDef( &lightParms );
	}
}


int Brush_TransformModel(brush_t *brush, idTriList *tris, idMatList *mats) {
	int ret = 0;
	if (brush->modelHandle > 0 ) {
		idRenderModel *model = brush->modelHandle;
		if (model) {
			float	a = brush->owner->FloatForKey("angle");
			float	s, c;
			//FIXME: support full rotation matrix
			bool matrix = false;
			if (a) {
				s = sin( DEG2RAD(a) );
				c = cos( DEG2RAD(a) );
			}
			idMat3 mat;
			if (brush->owner->GetMatrixForKey("rotation", mat)) {
				matrix = true;
			}


			for (int i = 0; i < model->NumSurfaces() ; i++) {
				const modelSurface_t	*surf = model->Surface( i );
				srfTriangles_t	*tri = surf->geometry;
				srfTriangles_t *tri2 = R_CopyStaticTriSurf(tri);
				for (int j = 0; j < tri2->numVerts; j++) {
					idVec3	v;
					if (matrix) {
						v = tri2->verts[j].xyz * brush->owner->rotation + brush->owner->origin;
					} else {
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
				mats->Append( surf->shader );
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
	if (brush->modelHandle > 0 ) {
		if (!models) {
			return 0;
		} else {
			return Brush_TransformModel(brush, tris, mats);
		}
	}

	int numSurfaces = 0;

	if ( brush->owner->eclass->fixedsize && !brush->entityModel) {
		return NULL;
	}

	if ( brush->pPatch ) {
		patchMesh_t *pm;
		int			width, height;

		pm = brush->pPatch;

		// build a patch mesh
		idSurface_Patch *cp = new idSurface_Patch( pm->width * 6, pm->height * 6 );
		cp->SetSize( pm->width, pm->height );
		for ( i = 0; i < pm->width; i++ ) {
			for ( j = 0; j < pm->height; j++ ) {
				(*cp)[j*cp->GetWidth()+i].xyz =  pm->ctrl(i, j).xyz;
				(*cp)[j*cp->GetWidth()+i].st = pm->ctrl(i, j).st;
			}
		}

		// subdivide it
		if ( pm->explicitSubdivisions ) {
			cp->SubdivideExplicit( pm->horzSubdivisions, pm->vertSubdivisions, true );
		} else {
			cp->Subdivide( DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true );
		}
		width = cp->GetWidth();
		height = cp->GetHeight();

		// convert to srfTriangles
		tri = R_AllocStaticTriSurf();
		tri->numVerts = width * height;
		tri->numIndexes = 6 * ( width - 1 ) * ( height - 1 );
		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
		for ( i = 0 ; i < tri->numVerts ; i++ ) {
			tri->verts[i] = (*cp)[i];
			if (bmodel) {
				tri->verts[i].xyz -= brush->owner->origin;
			}
		}

		tri->numIndexes = 0;
		for ( i = 1 ; i < width ; i++ ) {
			for ( j = 1 ; j < height ; j++ ) {
				tri->indexes[tri->numIndexes++] = ( j - 1 ) * width + i;
				tri->indexes[tri->numIndexes++] = ( j - 1 ) * width + i - 1;
				tri->indexes[tri->numIndexes++] = j * width + i - 1;

				tri->indexes[tri->numIndexes++] = j * width + i;
				tri->indexes[tri->numIndexes++] = ( j - 1 ) * width + i;
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
	for ( face_t *face = brush->brush_faces ; face; face = face->next ) {
		idWinding *w;

		w = face->face_winding;
		if (!w) {
			continue;	// freed or degenerate face
		}

		tri = R_AllocStaticTriSurf();
		tri->numVerts = w->GetNumPoints();
		tri->numIndexes = ( w->GetNumPoints() - 2 ) * 3;
		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );

		for ( i = 0 ; i < tri->numVerts ; i++ ) {

			tri->verts[i].Clear();

			tri->verts[i].xyz[0] = (*w)[i][0];
			tri->verts[i].xyz[1] = (*w)[i][1];
			tri->verts[i].xyz[2] = (*w)[i][2];

			if ( bmodel ) {
				tri->verts[i].xyz -= brush->owner->origin;
			}

			tri->verts[i].st[0] = (*w)[i][3];
			tri->verts[i].st[1] = (*w)[i][4];

			tri->verts[i].normal = face->plane.Normal();
		}

		tri->numIndexes = 0;
		for ( i = 2 ; i < w->GetNumPoints() ; i++ ) {
			tri->indexes[tri->numIndexes++] = 0;
			tri->indexes[tri->numIndexes++] = i-1;
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

			if ( b->hiddenBrush ) {
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

		idFile *objFile = fileSystem->OpenExplicitFileWrite( objFilePath );
		idFile *mtlFile = fileSystem->OpenExplicitFileWrite( mtlFilePath );

		Tris_ToOBJ(tris, mats, objFile, mtlFile);

		fileSystem->CloseFile( objFile );
		fileSystem->CloseFile( mtlFile );

		for( i = 0; i < tris.Num(); i++ ) {
			R_FreeStaticTriSurf( tris[i] );
		}
		tris.Clear();
	}
}

void Select_ToCM() {
	CFileDialog dlgFile( FALSE, "lwo, ase", NULL, 0, "(*.lwo)|*.lwo|(*.ase)|*.ase|(*.ma)|*.ma||", g_pParentWnd );

	if ( dlgFile.DoModal() == IDOK ) {
		idMapEntity *mapEnt;
		idMapPrimitive *p;
		idStr name;

		name = fileSystem->OSPathToRelativePath( dlgFile.GetPathName() );
		name.BackSlashesToSlashes();

		mapEnt = new idMapEntity();
		mapEnt->epairs.Set( "name", name.c_str() );

		for ( brush_t *b = selected_brushes.next; b != &selected_brushes; b = b->next ) {

			if ( b->hiddenBrush ) {
				continue;
			}

			if ( FilterBrush( b ) ) {
				continue;
			}

			p = BrushToMapPrimitive( b, b->owner->origin );
			if ( p ) {
				mapEnt->AddPrimitive( p );
			}
		}

		collisionModelManager->WriteCollisionModelForMapEntity( mapEnt, name.c_str() );

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
void CCamWnd::BuildRendererState() {
	renderEntity_t	worldEntity;
	entity_t	*ent;
	brush_t		*brush;

	FreeRendererState();

	// the renderWorld holds all the references and defs
	g_qeglobals.rw->InitFromMap( NULL );

	// create the raw model for all the brushes
	int numBrushes = 0;
	int numSurfaces = 0;

	// the renderModel for the world holds all the geometry that isn't in an entity
	worldModel = renderModelManager->AllocModel();
	worldModel->InitEmpty( "EditorWorldModel" );

	for ( brush_t *brushList = &active_brushes ; brushList ;
		brushList = (brushList == &active_brushes) ? &selected_brushes : NULL ) {

		for (brush = brushList->next; brush != brushList; brush = brush->next) {

			if ( brush->hiddenBrush ) {
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
				numSurfaces += Brush_ToTris( brush, &tris, &mats, false, false );
			}

			// add the surfaces to the renderModel
			modelSurface_t	surf;
			for ( int i = 0 ; i < tris.Num() ; i++ ) {
				surf.geometry = tris[i];
				surf.shader = mats[i];
				worldModel->AddSurface( surf );
			}

			numBrushes++;
		}
	}

	// bound and clean the triangles
	worldModel->FinishSurfaces();

	// the worldEntity just has the handle for the worldModel
	memset( &worldEntity, 0, sizeof( worldEntity ) );
	worldEntity.hModel = worldModel;
	worldEntity.axis = mat3_default;
	worldEntity.shaderParms[0] = 1;
	worldEntity.shaderParms[1] = 1;
	worldEntity.shaderParms[2] = 1;
	worldEntity.shaderParms[3] = 1;

	worldModelDef = g_qeglobals.rw->AddEntityDef( &worldEntity );

	// create the light and model entities exactly the way the game code would
	for ( ent = entities.next ; ent != &entities ; ent = ent->next ) {
		if ( ent->brushes.onext == &ent->brushes ) {
			continue;
		}

		if (CullBrush(ent->brushes.onext, true)) {
			continue;
		}

		if (Map_IsBrushFiltered(ent->brushes.onext)) {
			continue;
		}

		BuildEntityRenderState( ent, false );
	}

	//common->Printf("Render data used %d brushes\n", numBrushes);
	worldDirty = false;
	UpdateCaption();
}

/*
============================
CCamWnd::FreeRendererState

  Frees the render state data
============================
*/
void CCamWnd::FreeRendererState() {

	for ( entity_t *ent = entities.next ; ent != &entities ; ent = ent->next ) {
		if (ent->lightDef >= 0) {
			g_qeglobals.rw->FreeLightDef( ent->lightDef );
			ent->lightDef = -1;
		}

		if (ent->modelDef >= 0) {
			renderEntity_t *refent = const_cast<renderEntity_t *>(g_qeglobals.rw->GetRenderEntity( ent->modelDef ));
			if ( refent ) {
				if ( refent->callbackData ) {
					Mem_Free( refent->callbackData );
					refent->callbackData = NULL;
				}
				if ( refent->joints ) {
					Mem_Free16(refent->joints);
					refent->joints = NULL;
				}
			}
			g_qeglobals.rw->FreeEntityDef( ent->modelDef );
			ent->modelDef = -1;
		}
	}

	if ( worldModel ) {
		renderModelManager->FreeModel( worldModel );
		worldModel = NULL;
	}

}


/*
========================
CCamWnd::UpdateCaption

  updates the caption based on rendermode and whether the render mode needs updated
========================
*/
void CCamWnd::UpdateCaption() {

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
	SetWindowText(strCaption);
}

/*
===========================
CCamWnd::ToggleRenderMode

	Toggles the render mode
===========================
*/
void CCamWnd::ToggleRenderMode() {
	renderMode ^= 1;
	UpdateCaption();
}

/*
===========================
CCamWnd::ToggleRebuildMode

	Toggles the rebuild mode
===========================
*/
void CCamWnd::ToggleRebuildMode() {
	rebuildMode ^= 1;
	UpdateCaption();
}

/*
===========================
CCamWnd::ToggleEntityMode

	Toggles the entity mode
===========================
*/
void CCamWnd::ToggleEntityMode() {
	entityMode ^= 1;
	UpdateCaption();
}


/*
===========================
CCamWnd::ToggleRenderMode

	Toggles the render mode
===========================
*/
void CCamWnd::ToggleAnimationMode() {
	animationMode ^= 1;
	if (animationMode)  {
		SetTimer(0, 10, NULL);
	} else {
		KillTimer(0);
	}
	UpdateCaption();
}

/*
===========================
CCamWnd::ToggleSoundMode

	Toggles the sound mode
===========================
*/
void CCamWnd::ToggleSoundMode() {
	soundMode ^= 1;

	UpdateCaption();

	for ( entity_t *ent = entities.next ; ent != &entities ; ent = ent->next ) {
		ent->UpdateSoundEmitter();
	}
}

/*
===========================
CCamWnd::ToggleRenderMode

	Toggles the render mode
===========================
*/
void CCamWnd::ToggleSelectMode() {
	selectMode ^= 1;
	UpdateCaption();
}

/*
=========================
CCamWnd::MarkWorldDirty

  marks the render world as dirty
=========================
*/
void CCamWnd::MarkWorldDirty() {
	worldDirty = true;
	UpdateCaption();
}


/*
 =======================================================================================================================
    Cam_Render

	This used the renderSystem to draw a fully lit view of the world
 =======================================================================================================================
 */
void CCamWnd::Cam_Render() {
	if (!active_brushes.next) {
		return;					// not valid yet
	}

	// create the model, using explicit normals
	if ( rebuildMode && worldDirty ) {
		BuildRendererState();
	}

	const auto oldWindowWidth = glConfig.windowWidth;
	const auto oldWindowHeight = glConfig.windowHeight;
	const auto oldVidWidth = glConfig.vidWidth;
	const auto oldVidHeight = glConfig.vidHeight;

	glConfig.windowWidth = width;
	glConfig.windowHeight = height;

	// render it
	renderSystem->BeginFrame( width, height );

	renderView_t refdef;
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg = m_Camera->GetOrigin();

	// the editor uses opposite pitch convention
	const auto angles = m_Camera->GetAngles();
	refdef.viewaxis = idAngles( -angles.pitch, angles.yaw, angles.roll ).ToMat3();

	refdef.width = SCREEN_WIDTH;
	refdef.height = SCREEN_HEIGHT;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan((float)height / width) * idMath::M_RAD2DEG;

	// only set in animation mode to give a consistent look
	if (animationMode) {
		refdef.time = eventLoop->Milliseconds();
	}

	g_qeglobals.rw->RenderScene( &refdef );

	renderSystem->EndFrame();

	// get back to the editor state
	GL_ModelViewMatrix.LoadIdentity();
	glConfig.windowWidth = oldWindowWidth;
	glConfig.windowHeight = oldWindowHeight;
	glConfig.vidWidth = oldVidWidth;
	glConfig.vidHeight = oldVidHeight;
}


void CCamWnd::OnTimer(UINT nIDEvent)
{
	if ((renderMode && animationMode) || nIDEvent == 1) {
		Sys_UpdateWindows(W_CAMERA);
	}
	if (nIDEvent == 1) {
		KillTimer(1);
	}

	if (!animationMode ) {
		KillTimer(0);
	}
}


void CCamWnd::UpdateCameraView() {
	if (QE_SingleBrush(true, true)) {
		brush_t *b = selected_brushes.next;
		if (b->owner->eclass->nShowFlags & ECLASS_CAMERAVIEW) {
			// find the entity that targets this
			const char *name = b->owner->ValueForKey("name");
			entity_t *ent = FindEntity("target", name);
			if (ent) {
				if (!saveValid) {
					saveOrg = m_Camera->GetOrigin();
					saveAng = m_Camera->GetAngles();
					saveValid = true;
				}
				idVec3 v = b->owner->origin - ent->origin;
				v.Normalize();
				idAngles ang = v.ToMat3().ToAngles();
				ang.pitch = -ang.pitch;
				ang.roll = 0.0f;

				m_Camera->SetAngles(ang);
				m_Camera->SetOrigin(ent->origin);
				Sys_UpdateWindows( W_CAMERA );
				return;
			}
		}
	}
	if (saveValid) {
		m_Camera->SetAngles(saveAng);
		m_Camera->SetOrigin(saveOrg);
		Sys_UpdateWindows(W_CAMERA);
		saveValid = false;
	}
}

