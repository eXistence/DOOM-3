#pragma once

#include "OrthographicCamera.h"
#include "OrthographicView.h"

class fhOrthoCamera;

class fhOrthographicRenderPass {
public:
	fhOrthographicRenderPass(const fhOrthoCamera &camera, int width, int height)
		: camera(camera), windowWidth(width), windowHeight(height) {}

	void render();

private:
	void XY_DrawGrid();
	void XY_DrawBlockGrid();
	void PaintSizeInfo(int nDim1, int nDim2, idVec3 vMinBounds, idVec3 vMaxBounds);

	bool RotateMode();
	bool ScaleMode();

	int width() const { return windowWidth; }

	int height() const { return windowHeight; }

	const fhOrthoCamera &camera;
	const int windowWidth;
	const int windowHeight;
};