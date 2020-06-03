#include "EditorSession.h"
#include <QSet>

#include "../tools/radiant/QE3.H"

// legacy stuff
#if 0
void Select_Deselect(bool);
void Select_Delete();
void Map_LoadFile(const char *filename);
bool Map_SaveFile(const char *filename, bool use_region, bool autosave);
void Select_SetDefaultTexture(const idMaterial *mat, bool fitScale, bool setTexture);
#endif
fhEditorSession::fhEditorSession() {}

fhEditorSession::~fhEditorSession() {}

void fhEditorSession::loadMap(const QString &file) {
	QByteArray ba = file.toLocal8Bit();
	Map_LoadFile(ba.data());
	this->mapfile = file;
}

void fhEditorSession::saveMap(const QString &file) {
	QByteArray ba = file.toLocal8Bit();
	Map_SaveFile(ba.data(), false, false);
	this->mapfile = file;
}

void fhEditorSession::deselect() {
	Select_Deselect();
	mapUpdated();
}

void fhEditorSession::deleteSelected() {
	Select_Delete();
	mapUpdated();
}

void fhEditorSession::applyMaterial(const idMaterial *mat) {
	Select_SetDefaultTexture(mat, false, true);
	mapUpdated();
}

QSet<QString> fhEditorSession::getMaterialsInUse() const { 
	QSet<QString> materials;

	brush_t *b;
	for (b = active_brushes.next; b != NULL && b != &active_brushes; b = b->next) {
		if (b->pPatch) {
			materials.insert(b->pPatch->d_texture->GetName());
		} else {
			for (face_t *f = b->brush_faces; f; f = f->next) {
				materials.insert(f->texdef.name);
			}
		}
	}

	for (b = selected_brushes.next; b != NULL && b != &selected_brushes; b = b->next) {
		if (b->pPatch) {
			materials.insert(b->pPatch->d_texture->GetName());
		} else {
			for (face_t *f = b->brush_faces; f; f = f->next) {
				materials.insert(f->texdef.name);
			}
		}
	}

	return materials;
}