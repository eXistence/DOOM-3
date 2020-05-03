#pragma once

#include <QWindow>

class fhRenderWindow : public QWindow {

public:
	fhRenderWindow(QWindow *parent = nullptr);

	virtual void exposeEvent(QExposeEvent *) override;
	virtual bool event(QEvent *ev) override;

	QWidget *createContainingWidget(QWidget *parent);

private:
	virtual void render(){};
	void callRender();
	void init();

	QOpenGLContext *m_context;
	bool m_initialized;
};