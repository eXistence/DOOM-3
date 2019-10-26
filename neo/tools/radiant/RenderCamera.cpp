#include "RenderCamera.h"
#include "RenderMatrix.h"
#include "tr_local.h"

void RenderCamera::updateVectors() {
#if 0
	const auto doom2gl = idMat3(
		0, 0, -1,
		-1, 0, 0,
		0, 1, 0);

	const auto gl2doom = idMat3(
		0, -1, 0,
		0, 0, 1,
		-1, 0, 0);
#endif
	angles[PITCH] = idMath::ClampFloat(-85, 85, angles[PITCH]);

	const auto pitch = idRotation(idVec3(0, 0, 0), idVec3(0, 1, 0), angles[PITCH]).ToMat3();
	const auto yaw = idRotation(idVec3(0, 0, 0), idVec3(0, 0, 1), -angles[YAW]).ToMat3();	

	rotationMatrix = pitch * yaw;

	vpn = idVec3(1, 0, 0) * rotationMatrix;
	vup = idVec3(0, 0, 1) * rotationMatrix;
	vright = idVec3(0, -1, 0) * rotationMatrix;

	forward = idVec3(1, 0, 0) * yaw;
	right = idVec3(0, -1, 0) * yaw;
}

fhRenderMatrix RenderCamera::CreateViewMatrix() const {

#if 0
	const auto pitch = idRotation(idVec3(0, 0, 0), idVec3(0, 1, 0), -angles[PITCH]).ToMat3();
	const auto yaw = idRotation(idVec3(0, 0, 0), idVec3(0, 0, 1), angles[YAW]).ToMat3();

	const auto rotation = (yaw * pitch).ToMat4().Transpose();
	
	auto m2 = fhRenderMatrix(rotation.ToFloatPtr()) * fhRenderMatrix::FlipMatrix();
	/*
	m2[12] = -origin.x;
	m2[13] = -origin.y;
	m2[14] = -origin.z;
	*/
	return m2;

#else
	joGLMatrixStack stack;
	stack.LoadIdentity();
	stack.Rotate(-90.0f, 1.f, 0.f, 0.f); // put Z going up
	stack.Rotate(90.0f, 0.f, 0.f, 1.f); // put Z going up
	stack.Rotate(angles[0], 0, 1, 0);
	stack.Rotate(-angles[1], 0, 0, 1);
	stack.Translate(-origin[0], -origin[1], -origin[2]);

	float tmp[16];
	stack.Get(tmp);

	auto ret = fhRenderMatrix(tmp);
	return ret;
#endif
}

// (r,u) is a point on the cameras projection plane.
// r is horizontal coordinate (-1 left, +1 right)
// u is vertical coordinate (-1 bottom, +1 up)
idVec3 RenderCamera::GetRayFromPoint(float r, float u) const {
	// calc ray direction

	const float f = 1;

	idVec3	dir;
	for (int i = 0; i < 3; i++) {
		dir[i] = vpn[i] * f + vright[i] * r + vup[i] * u;
	}

	return dir.Normalized();
}