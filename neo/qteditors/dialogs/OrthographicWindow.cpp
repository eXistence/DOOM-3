#include "OrthographicWindow.h"
#include "OrthographicRenderPass.h"
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>

void MoveSelection(const idVec3 &orgMove);

namespace fh {

brush_t *CreateEntityBrush(QPoint point, const fhOrthoCamera &camera) {	
	idVec3 origin = camera.WindowCoordsToPoint(point, true);
	const int nDim = camera.GetDirectionAxisIndex();
	origin[nDim] = camera.SnapToGrid((g_qeglobals.d_new_brush_bottom[nDim] + g_qeglobals.d_new_brush_top[nDim]) / 2);

	idVec3 mins = origin - idVec3(16, 16, 16);
	idVec3 maxs = origin + idVec3(16, 16, 16);

	brush_t *n = Brush_Create(mins, maxs, &g_qeglobals.d_texturewin.texdef);
	if (n) {
		Brush_AddToList(n, &selected_brushes);
		Entity_LinkBrush(world_entity, n);
		Brush_Build(n);
	}

	return n;
}

void CreateEntityFromName(const char *pName, bool forceFixed, idVec3 min, idVec3 max, idVec3 org) {
	if (stricmp(pName, "worldspawn") == 0) {
		common->Printf("Can't create an entity with worldspawn.");
		return;
	}

	eclass_t *entityClass = Eclass_ForName(pName, false);
	/*
	if ((GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
		Select_Ungroup();
	}
	*/

	// create it
	entity_t *petNew = Entity_Create(entityClass, forceFixed);

	if (petNew && idStr::Icmp(pName, "light") == 0) {
		idVec3 rad = max - min;
		rad *= 0.5;
		if (rad.x != 0 && rad.y != 0 && rad.z != 0) {
			petNew->SetKeyValue("light_radius",
								va("%g %g %g", idMath::Fabs(rad.x), idMath::Fabs(rad.y), idMath::Fabs(rad.z)));
			petNew->DeleteKey("light");
		}
	}

	if (petNew == NULL) {
		if (!((selected_brushes.next == &selected_brushes) || (selected_brushes.next->next != &selected_brushes))) {
			brush_t *b = selected_brushes.next;
			if (b->owner != world_entity && ((b->owner->eclass->fixedsize && entityClass->fixedsize) || forceFixed)) {
				idVec3 mins, maxs;
				idVec3 origin;
				for (int i = 0; i < 3; i++) {
					origin[i] = b->mins[i] - entityClass->mins[i];
				}

				VectorAdd(entityClass->mins, origin, mins);
				VectorAdd(entityClass->maxs, origin, maxs);

				brush_t *nb = Brush_Create(mins, maxs, &entityClass->texdef);
				Entity_LinkBrush(b->owner, nb);
				nb->owner->eclass = entityClass;
				nb->owner->SetKeyValue("classname", pName);
				Brush_Free(b);
				Brush_Build(nb);
				Brush_AddToList(nb, &active_brushes);
				Select_Brush(nb);
				return;
			}
		}

		g_pParentWnd->MessageBox("Failed to create entity.", "info", 0);
		return;
	}

	Select_Deselect();

	//
	// entity_t* pEntity = world_entity; if (selected_brushes.next !=
	// &selected_brushes) pEntity = selected_brushes.next->owner;
	//
	Select_Brush(petNew->brushes.onext);
	Brush_Build(petNew->brushes.onext);
}

} // namespace fh

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
				idVec3 previous = camera.WindowCoordsToPoint(leftButtonDown, true);
				idVec3 current = camera.WindowCoordsToPoint(event->pos(), true);
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

bool fhOrthoRenderWindow::eventFilter(QObject *obj, QEvent *event) {
	switch (event->type()) {
	case QEvent::DragEnter:
		dragEnterEvent(static_cast<QDragEnterEvent *>(event));
		break;
	case QEvent::DragMove:
		dragMoveEvent(static_cast<QDragMoveEvent *>(event));
		break;
	case QEvent::DragLeave:
		dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
		break;
	case QEvent::Drop:
		dropEvent(static_cast<QDropEvent *>(event));
		break;
	}
	return fhRenderWindow::eventFilter(obj, event);
}

void fhOrthoRenderWindow::dragEnterEvent(QDragEnterEvent *event) {}
void fhOrthoRenderWindow::dragMoveEvent(QDragMoveEvent *event) {}
void fhOrthoRenderWindow::dragLeaveEvent(QDragLeaveEvent *event) {}
void fhOrthoRenderWindow::dropEvent(QDropEvent *event) {
	Select_Deselect();

	QString text = event->mimeData()->text();
	QByteArray ba = text.toLocal8Bit();
	idVec3 pos = camera.WindowCoordsToPoint(event->pos(), true);
	common->Printf("create entitiy: %s at (%f %f %f)\n", ba.data(), pos.x, pos.y, pos.z);

	idVec3 min, max, org;
	Select_GetBounds(min, max);
	Select_GetMid(org);

	if (selected_brushes.next == &selected_brushes) {
		fh::CreateEntityBrush(event->pos(), camera);
		min.Zero();
		max.Zero();
		fh::CreateEntityFromName(ba.data(), true, min, max, org);
	} else {
		fh::CreateEntityFromName(ba.data(), false, min, max, org);
	}
}

void fhOrthoRenderWindow::cycleViewType() { camera.cycleViewType(); }

void fhOrthoRenderWindow::render() {
	fhOrthographicRenderPass pass(camera, width(), height());
	pass.render();
}
