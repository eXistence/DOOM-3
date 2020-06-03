#pragma once
#include <QWidget>
#include "MaterialTreeModel.h"
#include <QLineEdit>

class fhMaterialBrowser : public QWidget {
Q_OBJECT 
public:
explicit fhMaterialBrowser(QWidget *parent);
	~fhMaterialBrowser();

signals:
	void loadMaterialsRequested(QVector<QString> materials);

private:
	QMenu* contextMenu;	
	QAction *loadMaterialsAction;
	QLineEdit *filter;
	fhMaterialTreeModel *model;
};