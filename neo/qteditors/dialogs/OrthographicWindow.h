#pragma once

#include "OrthographicView.h"
#include "OrthographicCamera.h"
#include "../widgets/RenderWindow.h"


class fhOrthoRenderWindow : public fhRenderWindow {
	Q_OBJECT
public:
	fhOrthoRenderWindow();

	void DragNewBrush(QPoint dragStart, QPoint cursor);
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void cycleViewType();

signals:
	void contextMenuRequested(QPoint position);

private:
	void render() override;

	fhOrthoCamera camera;

	QPoint rightButtonDown;
	bool rightDrag = false;

	QPoint leftButtonDown;
	bool leftDrag = false;
	bool dragNewBrush = false;
};