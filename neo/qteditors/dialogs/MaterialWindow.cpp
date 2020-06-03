#include "MaterialWindow.h"
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>

void drawText(const char *text, float scale, const idVec3 &pos, const idVec4 &color);

fhMaterialRenderWindow::Entry::Entry(const idMaterial *material)
	: material(material), image(material->GetEditorImage()) {}

fhMaterialRenderWindow::fhMaterialRenderWindow() : camera(this) {}

void fhMaterialRenderWindow::mouseMoveEvent(QMouseEvent *event) {}

void fhMaterialRenderWindow::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::MouseButton::LeftButton) {
		auto local = event->localPos();

		int x = local.x();
		int y = local.y() + scrollPosition;
		
		const int row = (int)floorf((float)y / (float)cellHeight);
		const int column = (int)floorf((float)x / (float)cellWidth);
		const int index = maxVisibleColumns() * row + column;

		if (index >= 0 && index != activeMaterialIndex && index < entries.size()) {
			activeMaterialIndex = index;
			materialSelected(entries[activeMaterialIndex].material);
		} else {
			activeMaterialIndex = -1;
			materialSelected(nullptr);
		}

		requestUpdate();
	}
}

void fhMaterialRenderWindow::mouseReleaseEvent(QMouseEvent *event) {}

void fhMaterialRenderWindow::wheelEvent(QWheelEvent *event) {
	auto f = event->angleDelta().y();
	setScrollPosition(scrollPosition + f * -0.2);
	scrollPositionChanged(scrollPosition);
}

void fhMaterialRenderWindow::resizeEvent(QResizeEvent *event) { scrollHeightChanged(calcMaxScrollPos()); }

void fhMaterialRenderWindow::setMaterials(const QVector<Entry> &materials) {
	Entry previousActive;
	if (activeMaterialIndex >= 0 && activeMaterialIndex < this->entries.size()) {
		previousActive = this->entries[activeMaterialIndex];
	}
	this->entries = materials;
	this->scrollPosition = 0;
	scrollPositionChanged(0);
	scrollHeightChanged(calcMaxScrollPos());
	activeMaterialIndex = this->entries.indexOf(previousActive);
	this->requestUpdate();
}

int fhMaterialRenderWindow::calcMaxScrollPos() const {
	int maxColumns = maxVisibleColumns();
	
	int maxRows = entries.size() / maxColumns;
	if (entries.size() % maxColumns != 0) {
		maxRows += 1;
	}

	int maxVisible = maxVisibleRows();

	if (maxVisible >= maxRows) {
		return 0;
	}

	return static_cast<int>((maxRows - maxVisibleRows()) * cellHeight);
}

void fhMaterialRenderWindow::render() {
	GL_State(GLS_DEFAULT);
	glViewport(0, 0, width(), height());
	glScissor(0, 0, width(), height());
	glClearColor(g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][0],
				 g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][1],
				 g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][2], 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	GL_ProjectionMatrix.LoadIdentity();
	GL_ProjectionMatrix.Ortho(0, width(), 0, height(), -100, 100);
	globalImages->BindNull();

	// init stuff

	const int maxColumns = static_cast<int>(width() / cellWidth);

	for (int i = 0; i < entries.size(); ++i) {
		const int column = i % maxColumns;
		const int row = static_cast<int>(i / maxColumns);

		renderCell(column, row, entries[i], i == activeMaterialIndex);
	}
}

static void renderQuad(fhImmediateMode &im, idVec2 from, idVec2 to, float depth) {
	im.Begin(GL_QUADS);
	im.TexCoord2f(0, 0);
	im.Vertex(idVec3(from.x, from.y, depth));
	im.TexCoord2f(1, 0);
	im.Vertex(idVec3(to.x, from.y, depth));
	im.TexCoord2f(1, 1);
	im.Vertex(idVec3(to.x, to.y, depth));
	im.TexCoord2f(0, 1);
	im.Vertex(idVec3(from.x, to.y, depth));
	im.End();
}

void fhMaterialRenderWindow::renderCell(int column, int row, const Entry &entry, bool active) {
	const auto outerFrom = idVec2(column * cellWidth, (height() + scrollPosition) - (row * cellHeight)) + idVec2(2, -2);
	const auto outerTo = outerFrom + idVec2(cellWidth, -cellHeight) + idVec2(-2, 2);
	if (outerTo.y > height() || outerFrom.y < 0) {
		return;
	}

	fhImmediateMode im;

	const float spacing = 2;

	const auto fullInnerSize =
		idVec2(abs(outerFrom.x - outerTo.x), abs(outerFrom.x - outerTo.x)) - (idVec2(spacing, spacing) * 2);

	auto innerFrom = outerFrom + idVec2(spacing, -spacing);
	auto innerTo = innerFrom + idVec2(fullInnerSize.x, -fullInnerSize.y);

	auto editorImage = entry.image;
	if (editorImage) {
		if (editorImage->uploadWidth > editorImage->uploadHeight) {

			auto fullHeight = abs(innerFrom.y - innerTo.y);
			auto scaledHeight = fullHeight * ((float)editorImage->uploadHeight) / editorImage->uploadWidth;

			innerFrom.y -= (fullHeight - scaledHeight) * 0.5;
			innerTo = innerFrom + idVec2(fullInnerSize.x, -scaledHeight);
		} else if (editorImage->uploadHeight > editorImage->uploadWidth) {
			auto fullWidth = abs(innerFrom.x - innerTo.x);
			auto scaledWidth = fullWidth * ((float)editorImage->uploadWidth) / editorImage->uploadHeight;

			innerFrom.x += (fullWidth - scaledWidth) * 0.5;
			innerTo = innerFrom + idVec2(scaledWidth, -fullInnerSize.y);
		}
	}

	if (active) {
		im.Color(idVec4(0.6, 0.15, 0.15, 1));
	} else {
		im.Color(idVec4(0.1, 0.1, 0.1, 1));
	}

	renderQuad(im, outerFrom, outerTo, 0);

	if (editorImage) {
		im.Color(idVec4(1, 1, 1, 1));
		im.SetTexture(editorImage);
	} else {
		im.Color(idVec4(0.5, 0.5, 0.5, 1));
	}

	renderQuad(im, innerFrom, innerTo, 0.1);

	const char *name = entry.material->GetName();
	int start = strlen(name);
	while (start > 1 && name[start - 1] != '/') {
		start -= 1;
	}

	drawText(&name[start], 0.8, idVec3(outerFrom.x + spacing, outerTo.y + spacing, 0.1), idVec4(1, 1, 1, 1));
}

int fhMaterialRenderWindow::maxVisibleColumns() const { 
	int ret = static_cast<int>(width() / cellWidth); 
	if (ret > 0) {
		return ret;
	}
	return 1;
}
int fhMaterialRenderWindow::maxVisibleRows() const {
	int ret = static_cast<int>(height() / cellHeight);
	if (ret > 0) {
		return ret;
	}
	return 1;
}