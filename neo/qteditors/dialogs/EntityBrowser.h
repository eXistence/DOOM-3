#pragma once
#include <QWidget>
#include "EntityTreeModel.h"
#include <QLineEdit>

class fhEntityBrowser : public QWidget {
Q_OBJECT 
public:
	explicit fhEntityBrowser(QWidget *parent);
	~fhEntityBrowser();

private:
	QLineEdit *filter;
	fhEntityTreeModel *model;
};