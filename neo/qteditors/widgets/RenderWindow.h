#pragma once

#include <QWindow>

class fhRenderWindow : public QWindow {

public:
	fhRenderWindow(QWindow *parent = nullptr);

	virtual void exposeEvent(QExposeEvent *) override;
	virtual bool event(QEvent *ev) override;

protected:
	virtual void render(){};

private:
	void callRender();
	void init();

	QOpenGLContext *m_context;
	bool m_initialized;
};