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
#pragma once

enum class CameraDrawMode {
	Wireframe,
	Solid,
	Textures
};

class RenderCamera {
public:
	//strafing forward/backward. positive distance is forward, negative distance is backward
	void MoveCameraForwardBackward(float distance) {
		origin = origin + forward * distance;
	}

	//strafing left/right. Negative distance is left, positive distance is right
	void MoveCameraLeftRight(float distance) {
		origin = origin + right * distance;		
	}

	void MoveCameraUpDown(float distance) {
		origin[2] += distance;
	}

	void TurnCameraUpDown(float degree) {
		angles[PITCH] = idMath::ClampFloat(-85, 85, angles[PITCH] + degree);
	}

	void TurnCameraLeftRight(float degree) {
		angles[YAW] += degree;
	}

	const idVec3& GetOrigin() const {
		return origin;
	}

	void SetOrigin(idVec3 origin) {
		this->origin = origin;
	}

	void SetOriginXY(idVec3 origin) {
		this->origin.x = origin.x;
		this->origin.y = origin.y;
	}

	void SetOrigin(float x, float y, float z) {
		origin = idVec3(x, y, z);
	}

	void SetOrigin(int axis, float value) {
		assert(axis >= 0 && axis <= 2);
		origin[axis] = value;
	}

	CameraDrawMode GetDrawMode() const {
		return drawMode;
	}

	void SetDrawMode(CameraDrawMode mode) {
		drawMode = mode;
	}

	idAngles GetAngles() const {
		return angles;
	}

	void SetAngles(idAngles angles) {
		this->angles = angles;
	}

	void SetAngles(float pitch, float yaw, float roll) {
		angles.pitch = pitch;
		angles.yaw = yaw;
		angles.roll = roll;
	}

	void SetAngle(CameraAngle angle, float value) {
		angles[(int)angle] = value;
	}

	void SetPitch(float pitch) {
		angles.pitch = pitch;		
	}

	void SetYaw(float yaw) {
		angles.yaw = yaw;
	}

	idVec3 GetViewRight() const {
		return vright;
	}

	idVec3 GetViewUp() const {
		return vup;
	}

	idVec3 GetViewPn() const {
		return vpn;
	}


	void BuildMatrix(bool renderMode, const float matrix[4][4]) {

		const float xa = (renderMode ? -angles[PITCH] : angles[PITCH]) * idMath::M_DEG2RAD;
		const float ya = angles[YAW] * idMath::M_DEG2RAD;

		// the movement matrix is kept 2d
		forward[0] = cos(ya);
		forward[1] = sin(ya);
		right[0] = forward[1];
		right[1] = -forward[0];

		for (int i = 0; i < 3; i++) {
			vright[i] = matrix[i][0];
			vup[i] = matrix[i][1];
			vpn[i] = matrix[i][2];
		}

		vright.Normalize();
		vup.Normalize();
		vpn.Normalize();
	}

	// (r,u) is a point on the cameras projection plane.
	// r is horizontal coordinate (-1 left, +1 right)
	// u is vertical coordinate (-1 bottom, +1 up)
	idVec3 GetRayFromPoint(float r, float u) const {
		// calc ray direction

		const float f = 1;

		idVec3	dir;
		for (int i = 0; i < 3; i++) {
			dir[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
		}
		
		return dir.Normalized();
	}

	int			width, height;
	
private:

	idVec3		origin;
	idAngles	angles;

	

	idVec3		vup, vpn, vright;	// view matrix
	idVec3		forward, right;	// move matrix
	CameraDrawMode	drawMode;
};


/////////////////////////////////////////////////////////////////////////////
// CCamWnd window

class CCamWnd : public CWnd
{
  DECLARE_DYNCREATE(CCamWnd);
// Construction
public:
	CCamWnd();
	~CCamWnd();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	RenderCamera& Camera(){ return m_Camera; };
	void Cam_MouseControl(float dtime);
	void Cam_ChangeFloor(bool up);
	void BuildRendererState();
	void ToggleRenderMode();
	void ToggleRebuildMode();
	void ToggleEntityMode();
	void ToggleSelectMode();
	void ToggleAnimationMode();
	void ToggleSoundMode();
	void UpdateCameraView();

	void BuildEntityRenderState( entity_t *ent, bool update );
	bool GetRenderMode() {
		return renderMode;
	}
	bool GetRebuildMode() {
		return rebuildMode;
	}
	bool GetEntityMode() {
		return entityMode;
	}
	bool GetAnimationMode() {
		return animationMode;
	}
	bool GetSelectMode() {
		return selectMode;
	}
	bool GetSoundMode() {
		return soundMode;
	}

	void MarkWorldDirty();

private:
	void SetProjectionMatrix();
	
	void Cam_BuildMatrix();
	void Cam_PositionDrag();
	void Cam_MouseLook();
	void Cam_MouseDown(int x, int y, int buttons);
	void Cam_MouseUp (int x, int y, int buttons);
	void Cam_MouseMoved (int x, int y, int buttons);
	void InitCull();
	bool CullBrush (brush_t *b, bool cubicOnly);
	void Cam_Draw();
	void Cam_Render();

	// game renderer interaction
	qhandle_t	worldModelDef;
	idRenderModel	*worldModel;		// createRawModel of the brush and patch geometry
	bool	worldDirty;
	bool	renderMode;
	bool	rebuildMode;
	bool	entityMode;
	bool	selectMode;
	bool	animationMode;
	bool	soundMode;
	void	FreeRendererState();
	void	UpdateCaption();

	RenderCamera m_Camera;
	int	m_nCambuttonstate;
	CPoint m_ptButton;
	CPoint m_ptCursor;
	CPoint m_ptLastCursor;
	idVec3 m_vCull1;
	idVec3 m_vCull2;
	int m_nCullv1[3];
	int m_nCullv2[3];
	idVec3 saveOrg;
	idAngles saveAng;
	bool saveValid;

	idPlane m_viewPlanes[5];

	// Generated message map functions
protected:
	void OriginalMouseDown(UINT nFlags, CPoint point);
	void OriginalMouseUp(UINT nFlags, CPoint point);
	//{{AFX_MSG(CCamWnd)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////