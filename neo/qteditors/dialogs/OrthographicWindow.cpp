#include "OrthographicWindow.h"
#include "OrthographicRenderPass.h"
#include <QMouseEvent>

void MoveSelection(const idVec3 &orgMove);

fhOrthoRenderWindow::fhOrthoRenderWindow() : camera(this) {}

void fhOrthoRenderWindow::DragNewBrush(QPoint dragStart, QPoint cursor) {
	// delete the current selection
	if (selected_brushes.next != &selected_brushes) {
		Brush_Free(selected_brushes.next);
	}

	idVec3 mins = camera.WindowCoordsToPoint(dragStart, true);

	int nDim = (camera.viewType == ViewType::XY) ? 2 : (camera.viewType == ViewType::YZ) ? 0 : 1;

	mins[nDim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_bottom[nDim] / g_qeglobals.d_gridsize));

	idVec3 maxs = camera.WindowCoordsToPoint(cursor, true);

	maxs[nDim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_top[nDim] / g_qeglobals.d_gridsize));
	if (maxs[nDim] <= mins[nDim]) {
		maxs[nDim] = mins[nDim] + g_qeglobals.d_gridsize;
	}

	for (int i = 0; i < 3; i++) {
		if (mins[i] == maxs[i]) {
			return; // don't create a degenerate brush
		}

		if (mins[i] > maxs[i]) {
			float temp = mins[i];
			mins[i] = maxs[i];
			maxs[i] = temp;
		}
	}

	brush_t *n = Brush_Create(mins, maxs, &g_qeglobals.d_texturewin.texdef);
	if (!n) {
		return;
	}

	idVec3 vSize;
	VectorSubtract(maxs, mins, vSize);
	// g_strStatus.Format("Size X:: %.1f  Y:: %.1f  Z:: %.1f", vSize[0], vSize[1], vSize[2]);
	// g_pParentWnd->SetStatusText(2, g_strStatus);

	Brush_AddToList(n, &selected_brushes);

	Entity_LinkBrush(world_entity, n);

	Brush_Build(n);

	// Sys_UpdateWindows (W_ALL);
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

void fhOrthoRenderWindow::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() & Qt::LeftButton && !leftDrag) {
		if ((event->pos() - leftButtonDown).manhattanLength() > 10) {
			leftDrag = true;
			leftButtonDown = event->pos();
			dragNewBrush = Select_IsEmpty();
			if (!dragNewBrush) {
				g_qeglobals.d_num_move_points = 0;
				Brush_SideSelect(selected_brushes.next, camera.WindowCoordsToPoint(event->pos(), HUGE_DISTANCE, false),
								 camera.GetDirection(), false);
			}
		}
	}

	if (event->buttons() & Qt::RightButton && !rightDrag) {
		if ((event->pos() - rightButtonDown).manhattanLength() > 10) {
			rightDrag = true;
			rightButtonDown = event->pos();
		}
	}

	if (event->buttons() & Qt::RightButton) {
		if (rightDrag) {
			QPoint cursor = event->pos();
			idVec3 delta = idVec3(0, 0, 0);
			switch (camera.viewType) {
			case ViewType::XY:
				delta.x = -(cursor.x() - rightButtonDown.x());
				delta.y = cursor.y() - rightButtonDown.y();
				break;
			case ViewType::XZ:
				delta.x = -(cursor.x() - rightButtonDown.x());
				delta.z = cursor.y() - rightButtonDown.y();
				break;
			case ViewType::YZ:
				delta.y = -(cursor.x() - rightButtonDown.x());
				delta.z = cursor.y() - rightButtonDown.y();
				break;
			}
			camera.move(delta);
			rightButtonDown = cursor;
		}
	} else if (event->buttons() & Qt::LeftButton) {
		if (leftDrag) {
			if (dragNewBrush) {
				DragNewBrush(leftButtonDown, event->pos());
			} else {
				idVec3 previous = camera.WindowCoordsToPoint(leftButtonDown, false);
				idVec3 current = camera.WindowCoordsToPoint(event->pos(), false);
				idVec3 delta = current - previous;
				leftButtonDown = event->pos();

				MoveSelection(delta);
			}
		}
	}

	requestUpdate();
}

void fhOrthoRenderWindow::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		leftButtonDown = event->pos();
		leftDrag = false;
	}

	if (event->button() == Qt::RightButton) {
		rightButtonDown = event->pos();
		rightDrag = false;
	}
}

void fhOrthoRenderWindow::mouseReleaseEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		if (!leftDrag && QApplication::keyboardModifiers() & Qt::ShiftModifier) {
			idVec3 origin = camera.WindowCoordsToPoint(event->pos(), false);
			idVec3 dir;
			switch (camera.viewType) {
			case ViewType::XY:
				origin.z = HUGE_DISTANCE;
				dir = idVec3(0, 0, -1);
				break;
			case ViewType::XZ:
				origin.y = HUGE_DISTANCE;
				dir = idVec3(0, -1, 0);
				break;
			case ViewType::YZ:
				origin.x = HUGE_DISTANCE;
				dir = idVec3(-1, 0, 0);
				break;
			}

			int flags = 0;
			if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
				flags |= SF_SINGLEFACE;
			}

			Select_Ray(origin, dir, flags);
		}
		leftDrag = false;
	}

	if (event->button() == Qt::RightButton) {
		if (!rightDrag) {
			contextMenuRequested(event->pos());
		}

		rightDrag = false;
	}
	requestUpdate();
}

void fhOrthoRenderWindow::wheelEvent(QWheelEvent *event) {
	QPoint numDegrees = event->angleDelta() / 8;

	const float minScale = 0.02;
	const float maxScale = 4.0;
	camera.scale += numDegrees.y() * 0.01;
	camera.scale = idMath::ClampFloat(minScale, maxScale, camera.scale);
	requestUpdate();

	event->accept();
}

void fhOrthoRenderWindow::cycleViewType() { camera.cycleViewType(); }

void fhOrthoRenderWindow::render() {
	fhOrthographicRenderPass pass(camera, width(), height());
	pass.render();
}
