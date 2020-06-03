#pragma once

#include "../widgets/RenderWindow.h"
#include "OrthographicCamera.h"
#include "OrthographicView.h"

class fhMaterialRenderWindow : public fhRenderWindow {
	Q_OBJECT
public:
	struct Entry {
		explicit Entry(const idMaterial *material);
		Entry() = default;
		Entry(const Entry &) = default;
		Entry &operator=(const Entry &) = default;

		bool operator==(const Entry &other) const { return material == other.material && image == other.image; }
		bool operator!=(const Entry &other) const { return !(*this == other); }

		const idMaterial *material = nullptr;
		idImage *image = nullptr;
	};

	fhMaterialRenderWindow();

	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

	void setMaterials(const QVector<Entry> &materials);

	void setScrollPosition(int pos) {
		auto maxPos = calcMaxScrollPos();
		scrollPosition = idMath::ClampInt(0, maxPos, pos);
		requestUpdate();
	}

signals:
	void scrollHeightChanged(int height);
	void scrollPositionChanged(int position);
	void materialSelected(const idMaterial *material);

private:
	int maxVisibleColumns() const;
	int maxVisibleRows() const;

	int calcMaxScrollPos() const;

	void render() override;
	void renderCell(int column, int row, const Entry &entry, bool active);

	QVector<Entry> entries;
	int activeMaterialIndex = -1;

	int scrollPosition = 0;
	float cellWidth = 128.0;
	float cellHeight = 140.0;

	fhOrthoCamera camera;
};