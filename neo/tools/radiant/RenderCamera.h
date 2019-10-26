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

class fhRenderMatrix;

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
		angles[PITCH] += degree;
		updateVectors();
	}

	void TurnCameraLeftRight(float degree) {
		angles[YAW] += degree;
		updateVectors();
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
		updateVectors();
	}

	idVec3 GetViewRight() const {
		return vright;
	}

	idVec3 GetViewUp() const {
		return vup;
	}

	// (r,u) is a point on the cameras projection plane.
	// r is horizontal coordinate (-1 left, +1 right)
	// u is vertical coordinate (-1 bottom, +1 up)
	idVec3 GetRayFromPoint(float r, float u) const;	

	const idMat3& GetRotationMatrix() const {
		return rotationMatrix;
	}

	fhRenderMatrix CreateViewMatrix() const;

private:
	void updateVectors();

	idVec3		origin = idVec3(0, 0, 0);
	idAngles	angles = idAngles(0, 0, 0);

	idMat3      rotationMatrix;

	idVec3		vup = idVec3(0, 0, 0);;
	idVec3		vpn = idVec3(0, 0, 0);;
	idVec3		vright = idVec3(0, 0, 0);;	// view matrix
	idVec3		forward = idVec3(0, 0, 0);;
	idVec3		right = idVec3(0, 0, 0);;	// move matrix
	CameraDrawMode	drawMode = CameraDrawMode::Textures;
};