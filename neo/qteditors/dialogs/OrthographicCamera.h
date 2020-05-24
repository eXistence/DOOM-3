#pragma once

#include "../tools/radiant/QE3.H"
#include "../widgets/RenderWindow.h"

class fhOrthoCamera {
public:
	explicit fhOrthoCamera(const QWindow *window);

	idVec3 origin = idVec3(0, 0, 0);
	float scale = 1.0f;
	ViewType viewType = ViewType::XY;

	void move(idVec3 delta);

	void cycleViewType();

	idVec3 WindowCoordsToPoint(int x, int y, float xyz, bool alignToGrid) const;
	idVec3 WindowCoordsToPoint(int x, int y, bool alignToGrid) const;
	idVec3 WindowCoordsToPoint(QPoint p, bool alignToGrid) const;
	idVec3 WindowCoordsToPoint(QPoint p, float xyz, bool alignToGrid) const;

	idVec3 SnapToGrid(idVec3 p) const;
	float SnapToGrid(float f) const;

	idVec3 GetDirection() const;

	int GetDirectionAxisIndex() const { return (viewType == ViewType::XY) ? 2 : (viewType == ViewType::YZ) ? 0 : 1; }

private:
	const QWindow *window;
};