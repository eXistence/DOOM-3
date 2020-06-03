#pragma once

#include <QObject>
#include <QSet>

class idMaterial;

class fhEditorSession : public QObject {
	Q_OBJECT
public:
	fhEditorSession();
	~fhEditorSession();

	QString getMapFile() const { return mapfile; }

	void loadMap(const QString &file);
	void saveMap(const QString &file);

	void deselect();
	void deleteSelected();

	QSet<QString> getMaterialsInUse() const;

	void applyMaterial(const idMaterial *mat);

signals:
	void mapUpdated();

private:
	QString mapfile;
};