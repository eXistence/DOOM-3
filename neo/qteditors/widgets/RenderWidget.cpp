/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of
these additional terms immediately following the terms and conditions of the GNU General Public License which
accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software
LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "RenderWidget.h"
#include "../../renderer/tr_local.h"
#include "../../sys/win32/win_local.h"
#include "../../tools/radiant/GLWidget.h"
#include "../tools/radiant/GLWidget.h"
#include "../tools/radiant/QE3.H"
#include "RenderWindow.h"
#include "qopenglcontext.h"
#include "qwindow.h"
#include <QCursor>
#include <QMouseEvent>
#include <QTimer>
#include <QToolBar>
#include <QtPlatformHeaders/QWGLNativeContext>
#include <QStatusBar>

void Drag_Begin(int x, int y, int buttons, const idVec3 &xaxis, const idVec3 &yaxis, const idVec3 &origin,
				const idVec3 &dir);

class KeyEventFilter : public QObject {
public:
	explicit KeyEventFilter(RenderCamera **camera, fhLegacyRenderWindow *parent) : m_camera(camera), parent(parent) {}

	void setEnabled(bool enabled);
	bool isEnabled() const { return enabled; }
	void update();

protected:
	bool eventFilter(QObject *obj, QEvent *event) {
		const auto camera = *m_camera;
		if (!camera || !enabled) {
			return false;
		}

		switch (event->type()) {
		case QEvent::KeyPress:
			return onKeyPress(*static_cast<QKeyEvent *>(event));
		case QEvent::KeyRelease:
			return onKeyRelease(*static_cast<QKeyEvent *>(event));
		case QEvent::MouseMove:
			return onMouseMove(*static_cast<QMouseEvent *>(event));
		case QEvent::MouseButtonPress:
			return onMouseButtonPress(*static_cast<QMouseEvent *>(event));
		case QEvent::MouseButtonRelease:
			return onMouseButtonRelease(*static_cast<QMouseEvent *>(event));
		case QEvent::MouseButtonDblClick:
			if (((QMouseEvent *)event)->button() == Qt::LeftButton) {
				setEnabled(false);
				return true;
			}
		}
		return QObject::eventFilter(obj, event);
	}

private:
	QPoint centerMouse();

	bool onKeyPress(const QKeyEvent &event);
	bool onKeyRelease(const QKeyEvent &event);
	bool onMouseMove(const QMouseEvent &event);
	bool onMouseButtonPress(const QMouseEvent &event);
	bool onMouseButtonRelease(const QMouseEvent &event);

	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;

	QCursor cursor;

	RenderCamera *camera() { return *m_camera; }
	fhLegacyRenderWindow *parent = nullptr;
	bool rightMouseButtonDown = false;
	RenderCamera **m_camera;
	bool enabled = false;
};

class fhLegacyRenderWindow : public fhRenderWindow {
	bool mouseDown = false;
	int prevX = 0;
	int prevY = 0;
	KeyEventFilter *eventFilter = nullptr;

public:
	fhLegacyRenderWindow(idGLDrawable **drawable, RenderCamera **camera, QWindow *parent = nullptr)
		: fhRenderWindow(parent), m_drawable(drawable), m_camera(camera) {

		this->eventFilter = new KeyEventFilter(camera, this);
		this->installEventFilter(this->eventFilter);

		m_timer.setInterval(16);
		m_timer.start();

		QObject::connect(&m_timer, &QTimer::timeout, [=]() {
			if (this->isVisible() && this->eventFilter->isEnabled()) {
				eventFilter->update();
			}
		});
	}

	virtual void mouseDoubleClickEvent(QMouseEvent *event) override {
		if (event->button() == Qt::LeftButton) {
			this->eventFilter->setEnabled(true);
		}
	}
	virtual void focusOutEvent(QFocusEvent *event) override { this->eventFilter->setEnabled(false); }
	void wheelEvent(QWheelEvent *event) override {
		QPoint numDegrees = event->angleDelta() / 8;
		(*m_drawable)->mouseScroll(numDegrees.y());
		event->accept();
	}

	virtual void mouseMoveEvent(QMouseEvent *ev) override {
		if (RenderCamera *camera = *m_camera) {
			return;
		}

		if (ev->type() == QEvent::MouseMove && mouseDown) {
			(*m_drawable)->mouseMove(ev->x(), ev->y());
		}
	}

	virtual bool event(QEvent *ev) override {
		switch (ev->type()) {
		case QEvent::MouseButtonPress:
			handleButton(dynamic_cast<QMouseEvent *>(ev));
			break;
		case QEvent::MouseButtonRelease:
			handleButton(dynamic_cast<QMouseEvent *>(ev));
			break;
		};
		return fhRenderWindow::event(ev);
	}

	void handleButton(QMouseEvent *ev) {

		if (!ev)
			return;

		if (auto camera = *m_camera) {
			if (ev->type() == QEvent::MouseButtonRelease) {
				auto windowSize = this->size();
				int height = windowSize.height();
				int width = windowSize.width();
				int y = height - ev->y();
				int x = ev->x();

				const float u = (float)(y - height / 2) / (width / 2);
				const float r = (float)(x - width / 2) / (width / 2);
				const idVec3 dir = camera->GetRayFromPoint(r, u);

				const auto LBUTTON = 0x0001;
				const auto RBUTTON = 0x0002;
				const auto MBUTTON = 0x0010;
				const auto SHIFT = 0x0004;
				const auto CONTROL = 0x0008;

				int buttons = 0;
				if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
					buttons |= SHIFT;
				}
				if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
					buttons |= CONTROL;
				}
				if (ev->button() == Qt::RightButton) {
					buttons |= RBUTTON;
				}
				if (ev->button() == Qt::LeftButton) {
					buttons |= LBUTTON;
				}
				if (ev->button() == Qt::MiddleButton) {
					buttons |= MBUTTON;
				}

				Drag_Begin(x, y, buttons, camera->GetViewRight(), camera->GetViewUp(), camera->GetOrigin(), dir);
				return;
			}
		}

		if (!m_drawable || !*m_drawable)
			return;

		if (RenderCamera *camera = *m_camera) {
			if (ev->button() == Qt::RightButton) {
				if (ev->type() == QEvent::MouseButtonPress && !mouseDown) {
					prevX = ev->x();
					prevY = ev->y();
					mouseDown = true;
				} else if (ev->type() == QEvent::MouseButtonRelease) {
					mouseDown = false;
				}
			}
			return;
		}

		if (ev->type() == QEvent::MouseButtonPress) {

			if (ev->button() == Qt::LeftButton)
				(*m_drawable)->buttonDown(MouseButton::Left, ev->x(), ev->y());

			if (ev->button() == Qt::RightButton) {
				if (!mouseDown) {
					mouseDown = true;
					(*m_drawable)->buttonDown(MouseButton::Right, ev->x(), ev->y());
				}
			}
		}

		if (ev->type() == QEvent::MouseButtonRelease) {
			if (ev->button() == Qt::LeftButton)
				(*m_drawable)->buttonUp(MouseButton::Left, ev->x(), ev->y());

			if (ev->button() == Qt::RightButton) {
				(*m_drawable)->buttonUp(MouseButton::Right, ev->x(), ev->y());
				mouseDown = false;
			}
		}
	}

	void render() override {

		const auto oldWindowWidth = glConfig.windowWidth;
		const auto oldWindowHeight = glConfig.windowHeight;
		const auto oldVidWidth = glConfig.vidWidth;
		const auto oldVidHeight = glConfig.vidHeight;

		QSize rect = size();

		glConfig.windowWidth = rect.width();
		glConfig.windowHeight = rect.height();

		glViewport(0, 0, rect.width(), rect.height());
		glScissor(0, 0, rect.width(), rect.height());

		GL_ProjectionMatrix.LoadIdentity();
		GL_ProjectionMatrix.Ortho(0, rect.width(), 0, rect.height(), -256, 256);

		glClearColor(0.4f, 0.4f, 0.4f, 0.7f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		if (m_drawable && *m_drawable) {
			(*m_drawable)->draw(1, 1, rect.width(), rect.height());
		}

		glConfig.windowWidth = oldWindowWidth;
		glConfig.windowHeight = oldWindowHeight;
		glConfig.vidWidth = oldVidWidth;
		glConfig.vidHeight = oldVidHeight;

		glViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
		glScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}

private:
	idGLDrawable **m_drawable;
	RenderCamera **m_camera;
	QPoint m_previousCursorPos;
	QTimer m_timer;
};

fhRenderWidget::fhRenderWidget(QWidget *parent) : QWidget(parent), m_drawable(nullptr) {
	auto layout = new QVBoxLayout;
	this->setLayout(layout);
	layout->setMargin(0);
	layout->setSpacing(0);

	QToolBar *toolbar = new QToolBar(this);
	QAction *foo = new QAction("Foo", this);
	toolbar->addAction(foo);
	layout->addWidget(toolbar);

	m_window = new fhLegacyRenderWindow(&m_drawable, &m_camera);
	layout->addWidget(m_window->createContainingWidget(this));

	this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

fhRenderWidget::~fhRenderWidget() {}

void fhRenderWidget::updateDrawable() { m_window->requestUpdate(); }

QSize fhRenderWidget::sizeHint() const { return QSize(200, 200); }

bool KeyEventFilter::onKeyPress(const QKeyEvent &event) {
	switch (event.key()) {
	case Qt::Key::Key_Escape:
		setEnabled(false);
		break;
	case Qt::Key::Key_W:
		forward = true;
		break;
	case Qt::Key::Key_S:
		backward = true;
		break;
	case Qt::Key::Key_A:
		left = true;
		break;
	case Qt::Key::Key_D:
		right = true;
		break;
	}
	return true;
}

bool KeyEventFilter::onKeyRelease(const QKeyEvent &event) {
	switch (event.key()) {
	case Qt::Key::Key_W:
		forward = false;
		break;
	case Qt::Key::Key_S:
		backward = false;
		break;
	case Qt::Key::Key_A:
		left = false;
		break;
	case Qt::Key::Key_D:
		right = false;
		break;
	}
	return true;
}

QPoint KeyEventFilter::centerMouse() {
	auto windowSize = parent->size();
	auto localCenter = QPoint(windowSize.width() * 0.5, windowSize.height() * 0.5);
	QCursor::setPos(parent->mapToGlobal(localCenter));
	return localCenter;
}

bool KeyEventFilter::onMouseMove(const QMouseEvent &event) {
	auto localCenter = centerMouse();

	int dX = event.x() - localCenter.x();
	int dY = event.y() - localCenter.y();

	camera()->TurnCameraLeftRight(-dX);
	camera()->TurnCameraUpDown(dY);
	parent->requestUpdate();

	return true;
}

bool KeyEventFilter::onMouseButtonPress(const QMouseEvent &event) { return true; }

bool KeyEventFilter::onMouseButtonRelease(const QMouseEvent &event) { return true; }

void KeyEventFilter::setEnabled(bool enabled) {
	if (enabled == this->enabled) {
		return;
	}

	this->enabled = enabled;

	if (enabled) {
		common->Printf("enter fly mode\n");
		parent->setCursor(Qt::BlankCursor);
		centerMouse();
	} else {
		common->Printf("leave fly mode\n");
		parent->unsetCursor();
		forward = false;
		backward = false;
		left = false;
		right = false;
	}
}

void KeyEventFilter::update() {
	if (forward)
		(*m_camera)->MoveCameraForwardBackward(10, true);

	if (backward)
		(*m_camera)->MoveCameraForwardBackward(-10, true);

	if (left)
		(*m_camera)->MoveCameraLeftRight(-10);

	if (right)
		(*m_camera)->MoveCameraLeftRight(10);

	parent->requestUpdate();
}