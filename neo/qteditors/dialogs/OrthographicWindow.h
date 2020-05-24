#pragma once

#include "../widgets/RenderWindow.h"
#include "OrthographicCamera.h"
#include "OrthographicView.h"

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

	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dropEvent(QDropEvent *event);

	bool eventFilter(QObject *obj, QEvent *event) override;

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