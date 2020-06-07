#include "Radiant.h"
#include <QAction>
#include <QDomDocument>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <QToolBar>

#include "DockAreaWidget.h"

#include "dialogs/EntityBrowser.h"
#include "dialogs/MaterialBrowser.h"
#include "dialogs/MaterialsView.h"
#include "dialogs/OrthographicView.h"
#include "dialogs/RenderPreview.h"

class fhFontAwesome {
public:
	explicit fhFontAwesome(QString fileUrl) {
		QFile file(fileUrl);
		if (!file.open(QFile::ReadOnly)) {
			return;
		}
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		auto object = doc.object();

		for (auto i = object.begin(); i != object.end(); ++i) {
			if (!i->isObject()) {
				continue;
			}
			const auto name = i.key();
			const auto object = i->toObject();

			const auto svgIt = object.find("svg");
			if (svgIt == object.end() || !svgIt->isObject()) {
				continue;
			}

			const auto svgObject = svgIt->toObject();
			for (auto j = svgObject.begin(); j != svgObject.end(); ++j) {
				if (!j->isObject()) {
					continue;
				}

				const auto styleName = j.key();
				const auto styleObject = j->toObject();

				const auto rawIt = styleObject.find("raw");
				if (rawIt == styleObject.end() || !rawIt->isString()) {
					continue;
				}

				const auto widthIt = styleObject.find("width");
				if (widthIt == styleObject.end() || !widthIt->isDouble()) {
					continue;
				}

				const auto heightIt = styleObject.find("height");
				if (heightIt == styleObject.end() || !heightIt->isDouble()) {
					continue;
				}

				const float width = (float)widthIt->toDouble();
				const float height = (float)heightIt->toDouble();

				fhSvgIcon icon;
				icon.raw = rawIt->toString();
				if (width > height) {
					icon.size.x = 1;
					icon.size.y = height / width;
				} else {
					icon.size.x = width / height;
					icon.size.y = 1;
				}

				icons[name][styleName] = icon;
			}
		}
	}

	QIcon createIcon(const QString &name, const QString &style, idVec3 color) {
		QColor qcolor;
		qcolor.setRgbF(color.x, color.y, color.z);
		return createIcon(name, style, qcolor);
	}

	QIcon createIcon(const QString &name, const QString &style, QColor color) {
		const auto &entry = icons.value(name).value(style);
		if (entry.raw.isEmpty()) {
			return QIcon();
		}

		return renderSvg(entry.raw.toLocal8Bit(), color);
	}

	QIcon createIcon(const QString &filename, QColor color) {
		QFile file(filename);
		if (!file.open(QFile::ReadOnly)) {
			return QIcon();
		}

		return renderSvg(file.readAll(), color);
	}

private:
	static QIcon renderSvg(const QByteArray &raw, QColor color) {

		QDomDocument doc;
		doc.setContent(raw);
		setAttrRecur(doc.documentElement(), "path", "fill", color.name());
		QSvgRenderer svgRenderer(doc.toByteArray());

		QPixmap pix(svgRenderer.defaultSize());
		pix.fill(Qt::transparent);
		QPainter pixPainter(&pix);
		pixPainter.setBrushOrigin(8, 8);
		svgRenderer.render(&pixPainter);

		return QIcon(pix);
	}


	static void setAttrRecur(QDomElement &elem, QString strtagname, QString strattr, QString strattrval) {
		// if it has the tagname then overwritte desired attribute
		if (elem.tagName().compare(strtagname) == 0) {
			elem.setAttribute(strattr, strattrval);
		}
		// loop all children
		for (int i = 0; i < elem.childNodes().count(); i++) {
			if (!elem.childNodes().at(i).isElement()) {
				continue;
			}
			setAttrRecur(elem.childNodes().at(i).toElement(), strtagname, strattr, strattrval);
		}
	}

	struct fhSvgIcon {
		QString raw;
		idVec2 size;
	};

	QMap<QString, QMap<QString, fhSvgIcon>> icons;
};

static ads::CDockWidget *createDockWidget(const QString &name, QWidget *widget) {
	ads::CDockWidget *dockWidget = new ads::CDockWidget(name);
	dockWidget->setWidget(widget);
	return dockWidget;
}

fhRadiant::fhRadiant(RenderCamera *renderCamera, QWidget *parent) : QMainWindow(parent) {
	dockManager = new ads::CDockManager(this);
	dockManager->setStyleSheet("");
	cameraView = new fhPreviewCamera(renderCamera, this);
	orthographicView = new fhOrthographicView(this);
	entityBrowser = new fhEntityBrowser(this);
	materialBrowser = new fhMaterialBrowser(this);
	materialsView = new fhMaterialsView(&session, this);

	fhFontAwesome fontAwesome(":/fontawesome.json");
	const QColor buttonColor("#ffaa00");

	QToolBar *toolbar = addToolBar(tr("File"));
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	QMenu *selectionMenu = menuBar()->addMenu(tr("&Selection"));

	QAction *open = new QAction(tr("&Open Map"), this);
	open->setIcon(fontAwesome.createIcon("folder-open", "regular", buttonColor));
	fileMenu->addAction(open);
	toolbar->addAction(open);

	QAction *save = new QAction(tr("&Save Map"), this);
	save->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
	save->setIcon(fontAwesome.createIcon("save", "regular", buttonColor));
	fileMenu->addAction(save);
	toolbar->addAction(save);

	toolbar->addSeparator();

	QAction *rotateX = new QAction(tr("Rotate around x axis"), this);
	rotateX->setIcon(fontAwesome.createIcon(":/rotate-x.svg", buttonColor));
	toolbar->addAction(rotateX);

	QAction *mirrorX = new QAction(tr("Mirror on x axis"), this);
	mirrorX->setIcon(fontAwesome.createIcon(":/mirror-x.svg", buttonColor));
	toolbar->addAction(mirrorX);

	QAction *rotateY = new QAction(tr("Rotate around y axis"), this);
	rotateY->setIcon(fontAwesome.createIcon(":/rotate-y.svg", buttonColor));
	toolbar->addAction(rotateY);

	QAction *mirrorY = new QAction(tr("Mirror on y axis"), this);
	mirrorY->setIcon(fontAwesome.createIcon(":/mirror-y.svg", buttonColor));
	toolbar->addAction(mirrorY);

	QAction *rotateZ = new QAction(tr("Rotate around z axis"), this);
	rotateZ->setIcon(fontAwesome.createIcon(":/rotate-z.svg", buttonColor));
	toolbar->addAction(rotateZ);

	QAction *mirrorZ = new QAction(tr("Mirror on z axis"), this);
	mirrorZ->setIcon(fontAwesome.createIcon(":/mirror-z.svg", buttonColor));
	toolbar->addAction(mirrorZ);


	QAction *saveAs = new QAction(tr("&Save Map as"), this);
	fileMenu->addAction(saveAs);

	QAction *deselect = selectionMenu->addAction("deselect");
	deselect->setShortcut(QKeySequence(Qt::Key_Escape));

	QAction *del = selectionMenu->addAction("delete");
	del->setIcon(fontAwesome.createIcon("trash-alt", "regular", buttonColor));
	QList<QKeySequence> shortcuts;
	shortcuts << QKeySequence(Qt::Key_Backspace) << QKeySequence(Qt::Key_Delete);
	del->setShortcuts(shortcuts);

	QObject::connect(deselect, &QAction::triggered, [this]() { session.deselect(); });
	QObject::connect(del, &QAction::triggered, [this]() { session.deleteSelected(); });
	QObject::connect(open, &QAction::triggered, [this]() { loadMapFile(); });
	QObject::connect(save, &QAction::triggered, [this]() { saveMapFile(); });
	QObject::connect(saveAs, &QAction::triggered, [this]() { saveAsMapFile(); });

	QObject::connect(materialBrowser, &fhMaterialBrowser::loadMaterialsRequested,
					 [this](const QVector<QString> &materials) { materialsView->setLoadedMaterials(materials); });

	auto left = addDockableView(cameraView->windowTitle(), cameraView, ads::LeftDockWidgetArea);
	auto middle = addDockableView(orthographicView->windowTitle(), orthographicView, ads::RightDockWidgetArea);
	auto right = addDockableView(entityBrowser->windowTitle(), entityBrowser, ads::RightDockWidgetArea);

	dockManager->addDockWidget(ads::CenterDockWidgetArea,
							   createDockWidget(materialBrowser->windowTitle(), materialBrowser), right);

	dockManager->addDockWidget(ads::BottomDockWidgetArea, createDockWidget(materialsView->windowTitle(), materialsView),
							   left);
}

fhRadiant::~fhRadiant() { delete dockManager; }

void fhRadiant::update() {
	cameraView->draw();
	cameraView->repaint();

	orthographicView->draw();
	orthographicView->repaint();
}

ads::CDockAreaWidget *fhRadiant::addDockableView(const QString &name, QWidget *widget, ads::DockWidgetArea area,
												 ads::CDockAreaWidget *dockAreaWidget) {
	return dockManager->addDockWidget(area, createDockWidget(name, widget), dockAreaWidget);
}

void fhRadiant::loadMapFile() {
	QFileDialog dialog(this);
	dialog.setNameFilter(tr("Maps (*.map)"));
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setFileMode(QFileDialog::ExistingFile);

	if (!dialog.exec())
		return;
	QStringList fileNames = dialog.selectedFiles();
	if (fileNames.isEmpty() || fileNames.first().isEmpty()) {
		return;
	}

	session.loadMap(fileNames.first());
}

void fhRadiant::saveAsMapFile() {
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Maps (*.map)"));
	dialog.setViewMode(QFileDialog::Detail);

	if (!dialog.exec())
		return;
	QStringList fileNames = dialog.selectedFiles();
	if (fileNames.isEmpty() || fileNames.first().isEmpty()) {
		return;
	}

	session.saveMap(fileNames.first());
}

void fhRadiant::saveMapFile() {
	const auto file = session.getMapFile();
	if (file.isEmpty()) {
		saveAsMapFile();
	} else {
		session.saveMap(file);
	}
}