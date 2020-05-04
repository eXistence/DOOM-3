#include "OrthographicCamera.h"

fhOrthoCamera::fhOrthoCamera(const QWindow *window) : window(window) {}

void fhOrthoCamera::move(idVec3 delta) { origin += delta * (1.0 / scale); }

void fhOrthoCamera::cycleViewType() { viewType = static_cast<ViewType>(((int)viewType + 1) % 3); }

idVec3 fhOrthoCamera::WindowCoordsToPoint(int x, int y, float xyz, bool alignToGrid) const {
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

idVec3 fhOrthoCamera::WindowCoordsToPoint(int x, int y, bool alignToGrid) const {
	return WindowCoordsToPoint(x, y, 0, alignToGrid);
}

idVec3 fhOrthoCamera::WindowCoordsToPoint(QPoint p, bool alignToGrid) const {
	return WindowCoordsToPoint(p.x(), p.y(), alignToGrid);
}

idVec3 fhOrthoCamera::WindowCoordsToPoint(QPoint p, float xyz, bool alignToGrid) const {
	return WindowCoordsToPoint(p.x(), p.y(), xyz, alignToGrid);
}

idVec3 fhOrthoCamera::GetDirection() const {
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
