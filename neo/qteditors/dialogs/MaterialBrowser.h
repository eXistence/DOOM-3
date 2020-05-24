#pragma once
#include <QWidget>
#include "MaterialTreeModel.h"
#include <QLineEdit>

class fhMaterialBrowser : public QWidget {
Q_OBJECT 
public:
explicit fhMaterialBrowser(QWidget *parent);
	~fhMaterialBrowser();

private:
	QLineEdit *filter;
	fhMaterialTreeModel *model;
};