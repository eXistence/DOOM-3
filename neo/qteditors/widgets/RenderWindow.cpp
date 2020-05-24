#include "RenderWindow.h"
#include "../../sys/win32/win_local.h"
#include <QOpenGLContext>
#include <QtPlatformHeaders/QWGLNativeContext>

fhRenderWindow::fhRenderWindow(QWindow *parent) : QWindow(parent), m_context(nullptr), m_initialized(false) {

	setSurfaceType(QWindow::OpenGLSurface);
}

void fhRenderWindow::exposeEvent(QExposeEvent *) {
	if (isExposed()) {
		callRender();
	}
}

bool fhRenderWindow::event(QEvent *ev) {
	if (ev->type() == QEvent::UpdateRequest) {
		callRender();
	}
	return QWindow::event(ev);
}

void fhRenderWindow::callRender() {
	if (!m_initialized) {
		init();
		m_initialized = true;
	}

	if (!m_context)
		return;

	if (!m_context->makeCurrent(this))
		return;

	render();

	m_context->swapBuffers(this);
	wglMakeCurrent(win32.hDC, win32.hGLRC);
}

void fhRenderWindow::init() {
	auto ctx = win32.hGLRC;
	auto hwnd = (HWND)this->winId();
	auto hDC = GetDC(hwnd);

	int pixelFormat = ChoosePixelFormat(hDC, &win32.pfd);
	if (pixelFormat > 0) {
		if (SetPixelFormat(hDC, pixelFormat, &win32.pfd) == NULL) {
			int foo = 2;
		}
	}

	ReleaseDC(hwnd, hDC);

	m_context = new QOpenGLContext(this);
	//		m_context->setFormat( requestedFormat() );
	QWGLNativeContext nativeContext(ctx, hwnd);
	m_context->setNativeHandle(QVariant::fromValue(nativeContext));

	if (!m_context->create()) {
		delete m_context;
		m_context = 0;
	}
}

QWidget *fhRenderWindow::createContainingWidget(QWidget* parent) {
	QWidget *widget = QWidget::createWindowContainer(this, parent);		
	return widget;
}