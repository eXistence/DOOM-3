/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include <GL/glu.h>

#include "../../renderer/tr_local.h"
#include "../../renderer/model_local.h"	// for idRenderModelMD5
#include "../../renderer/RenderProgram.h"
#include "../../renderer/ImmediateMode.h"

void Brush_DrawCurve( const brush_t *b, bool bSelected, bool cam );
void drawText(const char* text, float scale, const idVec3& pos, const idVec3& color, ViewType viewType);

// globals
static int		g_nBrushId = 0;
bool	g_bShowLightVolumes = false;
bool	g_bShowLightTextures = false;

static const int POINTS_PER_KNOT = 50;

/*
================
DrawRenderModel
================
*/
void DrawRenderModel( idRenderModel *model, const idVec3 &origin, const idMat3 &axis, bool cameraView, const idVec3& color ) {
	const CameraDrawMode nDrawMode = g_pParentWnd->GetCamera().GetDrawMode();
	const idMat4 modelView = idMat4(axis, origin).Transpose();

	GL_ModelViewMatrix.Push();
	GL_ModelViewMatrix.Load(modelView.ToFloatPtr());

	for ( int i = 0; i < model->NumSurfaces(); i++ ) {
		const modelSurface_t *surf = model->Surface( i );
		const idMaterial *material = surf->shader;

		if ( R_CreateAmbientCache(surf->geometry, true) ) {
			assert(surf->geometry->ambientCache);

			int offset = vertexCache.Bind(surf->geometry->ambientCache);

			if (cameraView && (nDrawMode == CameraDrawMode::Textures) && material) {
				GL_UseProgram(defaultProgram);
				material->GetEditorImage()->Bind(1);

				fhRenderProgram::SetColorAdd(idVec4(color * 0.25, 1));
				fhRenderProgram::SetColorModulate(idVec4(1, 1, 1, 1));
			} else {
				GL_UseProgram(vertexColorProgram);
				fhRenderProgram::SetColorAdd( idVec4( color, 1 ) );
				fhRenderProgram::SetColorModulate( idVec4( 0, 0, 0, 0 ) );
			}

			fhRenderProgram::SetModelViewMatrix(GL_ModelViewMatrix.Top());
			fhRenderProgram::SetProjectionMatrix(GL_ProjectionMatrix.Top());
			fhRenderProgram::SetDiffuseColor(idVec4::one);
			fhRenderProgram::SetBumpMatrix(idVec4(1,0,0,0), idVec4(0,1,0,0));

			GL_SetupVertexAttributes(fhVertexLayout::DrawPosColorTexOnly, offset);
			glDrawElements(GL_TRIANGLES, surf->geometry->numIndexes,  GL_INDEX_TYPE, surf->geometry->indexes);

			GL_UseProgram(nullptr);

		} else {
			fhImmediateMode im;

			if (cameraView && nDrawMode == CameraDrawMode::Textures) {
				im.SetTexture(material->GetEditorImage());
			}

			im.Begin(GL_TRIANGLES);
			im.Color3fv(color.ToFloatPtr());

			const srfTriangles_t *tri = surf->geometry;

			for (int j = 0; j < tri->numIndexes; j += 3) {
				for (int k = 0; k < 3; k++) {
					const int		index = tri->indexes[j + k];
					im.TexCoord2f(tri->verts[index].st.x, tri->verts[index].st.y);
					im.Vertex3fv(tri->verts[index].xyz.ToFloatPtr());
				}
			}

			im.End();
		}
	}

  GL_ModelViewMatrix.Pop();
}

/*
================
SnapVectorToGrid
================
*/
static void SnapVectorToGrid(idVec3 &v) {
	v.x = floor(v.x / g_qeglobals.d_gridsize + 0.5f) * g_qeglobals.d_gridsize;
	v.y = floor(v.y / g_qeglobals.d_gridsize + 0.5f) * g_qeglobals.d_gridsize;
	v.z = floor(v.z / g_qeglobals.d_gridsize + 0.5f) * g_qeglobals.d_gridsize;
}

/*
================
Brush_Alloc
================
*/
brush_t *Brush_Alloc( void ) {
	brush_t *b = new brush_t;
	b->prev = b->next = NULL;
	b->oprev = b->onext = NULL;
	b->owner = NULL;
	b->mins.Zero();
	b->maxs.Zero();

	b->lightCenter.Zero();
	b->lightRight.Zero();
	b->lightTarget.Zero();
	b->lightUp.Zero();
	b->lightRadius.Zero();
	b->lightOffset.Zero();
	b->lightColor.Zero();
	b->lightStart.Zero();
	b->lightEnd.Zero();
	b->pointLight = false;
	b->startEnd = false;
	b->lightTexture = 0;
	b->trackLightOrigin = false;

	b->entityModel = false;
	b->brush_faces = NULL;
	b->hiddenBrush = false;
	b->pPatch = NULL;
	b->pUndoOwner = NULL;
	b->undoId = 0;
	b->redoId = 0;
	b->ownerId = 0;
	b->numberId = 0;
	b->modelHandle = NULL;
	b->animSnapshotModel = NULL;
	b->forceVisibile = false;
	b->forceWireFrame = false;

	return b;
}

/*
================
TextureAxisFromPlane
================
*/
static const idVec3	baseaxis[18] = {
	idVec3(0, 0, 1),
	idVec3(1, 0, 0),
	idVec3(0, -1, 0),

	// floor
	idVec3(0, 0, -1),
	idVec3(1, 0, 0),
	idVec3(0, -1, 0),

	// ceiling
	idVec3(1, 0, 0),
	idVec3(0, 1, 0),
	idVec3(0, 0, -1),

	// west wall
	idVec3(-1, 0, 0),
	idVec3(0, 1, 0),
	idVec3(0, 0, -1),

	// east wall
	idVec3(0, 1, 0),
	idVec3(1, 0, 0),
	idVec3(0, 0, -1),

	// south wall
	idVec3(0, -1, 0),
	idVec3(1, 0, 0),
	idVec3(0, 0, -1)	// north wall
};

static void TextureAxisFromPlane( const idPlane &pln, idVec3 &xv, idVec3 &yv) {
	int		bestaxis;
	float	dot, best;
	int		i;

	best = 0;
	bestaxis = 0;

	for (i = 0; i < 6; i++) {
		dot = DotProduct(pln, baseaxis[i * 3]);
		if (dot > best) {
			best = dot;
			bestaxis = i;
		}
	}

	VectorCopy(baseaxis[bestaxis * 3 + 1], xv);
	VectorCopy(baseaxis[bestaxis * 3 + 2], yv);
}

/*
================
Brush_RemoveEmptyFaces

Frees any overconstraining faces
================
*/
static void Brush_RemoveEmptyFaces(brush_t *b) {
	face_t	*f, *next;

	f = b->brush_faces;
	b->brush_faces = NULL;

	for (; f; f = next) {
		next = f->next;
		if (!f->face_winding) {
			Face_Free(f);
		}
		else {
			f->next = b->brush_faces;
			b->brush_faces = f;
		}
	}
}

/*
================
ShadeForNormal

  Light different planes differently to improve recognition
================
*/
float	lightaxis[3] = { 0.6f, 0.8f, 1.0f };

float ShadeForNormal(idVec3 normal) {
	int		i;
	float	f;

	// axial plane
	for (i = 0; i < 3; i++) {
		if ( idMath::Fabs(normal[i]) > 0.9f ) {
			f = lightaxis[i];
			return f;
		}
	}

	// between two axial planes
	for (i = 0; i < 3; i++) {
		if ( idMath::Fabs(normal[i]) < 0.1f ) {
			f = (lightaxis[(i + 1) % 3] + lightaxis[(i + 2) % 3]) / 2;
			return f;
		}
	}

	// other
	f = (lightaxis[0] + lightaxis[1] + lightaxis[2]) / 3;
	return f;
}

/*
================
Face_Alloc
================
*/
face_t *Face_Alloc(void) {
	brushprimit_texdef_t	bp;

	face_t *f = (face_t *) Mem_ClearedAlloc(sizeof(*f));

	bp.coords[0][0] = 0.0f;
	bp.coords[1][1] = 0.0f;
	f->brushprimit_texdef = bp;
	f->dirty = true;
	return f;
}

/*
================
Face_Free
================
*/
void Face_Free(face_t *f) {
	assert(f != 0);

	if (f->face_winding) {
		delete f->face_winding;
	}

	f->texdef.~texdef_t();

	Mem_Free(f);
}

/*
================
Face_Clone
================
*/
face_t *Face_Clone(face_t *f) {
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;
	n->brushprimit_texdef = f->brushprimit_texdef;

	memcpy(n->planepts, f->planepts, sizeof(n->planepts));
	n->plane = f->plane;
	n->originalPlane = f->originalPlane;
	n->dirty = f->dirty;

	// all other fields are derived, and will be set by Brush_Build
	return n;
}

/*
================
Face_FullClone

  Used by Undo.
  Makes an exact copy of the face.
================
*/
face_t *Face_FullClone(face_t *f) {
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;
	n->brushprimit_texdef = f->brushprimit_texdef;
	memcpy(n->planepts, f->planepts, sizeof(n->planepts));
	n->plane = f->plane;
	n->originalPlane = f->originalPlane;
	n->dirty = f->dirty;
	if (f->face_winding) {
		n->face_winding = f->face_winding->Copy();
	}
	else {
		n->face_winding = NULL;
	}

	n->d_texture = Texture_ForName(n->texdef.name);
	return n;
}

/*
================
Clamp
================
*/
void Clamp(float &f, int nClamp) {
	float	fFrac = f - static_cast<int>(f);
	f = static_cast<int>(f) % nClamp;
	f += fFrac;
}

/*
================
Face_MoveTexture
================
*/
void Face_MoveTexture(face_t *f, idVec3 delta) {
	Face_MoveTexture_BrushPrimit(f, delta);
}

/*
================
Face_SetColor
================
*/
void Face_SetColor(brush_t *b, face_t *f, float fCurveColor) {
	float		shade;
	const idMaterial	*q;

	q = f->d_texture;

	// set shading for face
	shade = ShadeForNormal( f->plane.Normal() );
	if (g_pParentWnd->GetCamera().GetDrawMode() == CameraDrawMode::Textures && (b->owner && !b->owner->eclass->fixedsize)) {
		// if (b->curveBrush) shade = fCurveColor;
		f->d_color[0] = f->d_color[1] = f->d_color[2] = shade;
	}
	else if ( f && b && b->owner ) {
		f->d_color[0] = shade * b->owner->eclass->color.x;
		f->d_color[1] = shade * b->owner->eclass->color.y;
		f->d_color[2] = shade * b->owner->eclass->color.z;
	}
}

/*
================
Face_TextureVectors

  NOTE: this is never to get called while in brush primitives mode
================
*/
void Face_TextureVectors(face_t *f, float STfromXYZ[2][4]) {
	idVec3		pvecs[2];
	int			sv, tv;
	float		ang, sinv, cosv;
	float		ns, nt;
	int			i, j;
	const idMaterial	*q;
	texdef_t	*td;

	td = &f->texdef;
	q = f->d_texture;

	memset(STfromXYZ, 0, 8 * sizeof (float));

	if (!td->scale[0]) {
		td->scale[0] = (g_PrefsDlg.m_bHiColorTextures) ? 2 : 1;
	}

	if (!td->scale[1]) {
		td->scale[1] = (g_PrefsDlg.m_bHiColorTextures) ? 2 : 1;
	}

	// get natural texture axis
	TextureAxisFromPlane( f->plane, pvecs[0], pvecs[1]);

	// rotate axis
	if (td->rotate == 0) {
		sinv = 0;
		cosv = 1;
	}
	else if (td->rotate == 90) {
		sinv = 1;
		cosv = 0;
	}
	else if (td->rotate == 180) {
		sinv = 0;
		cosv = -1;
	}
	else if (td->rotate == 270) {
		sinv = -1;
		cosv = 0;
	}
	else {
		ang = DEG2RAD( td->rotate );
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (pvecs[0][0]) {
		sv = 0;
	}
	else if (pvecs[0][1]) {
		sv = 1;
	}
	else {
		sv = 2;
	}

	if (pvecs[1][0]) {
		tv = 0;
	}
	else if (pvecs[1][1]) {
		tv = 1;
	}
	else {
		tv = 2;
	}

	for (i = 0; i < 2; i++) {
		ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
		nt = sinv * pvecs[i][sv] + cosv * pvecs[i][tv];
		STfromXYZ[i][sv] = ns;
		STfromXYZ[i][tv] = nt;
	}

	// scale
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			STfromXYZ[i][j] = STfromXYZ[i][j] / td->scale[i];
		}
	}

	// shift
	STfromXYZ[0][3] = td->shift[0];
	STfromXYZ[1][3] = td->shift[1];

	for (j = 0; j < 4; j++) {
		STfromXYZ[0][j] /= q->GetEditorImage()->uploadWidth;
		STfromXYZ[1][j] /= q->GetEditorImage()->uploadHeight;
	}
}

/*
================
Face_MakePlane
================
*/
void Face_MakePlane(face_t *f) {
	int		j;
	idVec3	t1, t2, t3;

	idPlane oldPlane = f->plane;

	// convert to a vector / dist plane
	for (j = 0; j < 3; j++) {
		t1[j] = f->planepts[0][j] - f->planepts[1][j];
		t2[j] = f->planepts[2][j] - f->planepts[1][j];
		t3[j] = f->planepts[1][j];
	}

	f->plane = t1.Cross( t2 );
	//if ( f->plane.Compare( vec3_origin ) ) {
	//	printf("WARNING: brush plane with no normal\n");
	//}

	f->plane.Normalize(false);
	f->plane[3] = - (t3 * f->plane.Normal());

	if ( !f->dirty && !f->plane.Compare( oldPlane, 0.01f ) ) {
		f->dirty = true;
	}
}

/*
================
EmitTextureCoordinates
================
*/
void EmitTextureCoordinates(idVec5 &xyzst, const idMaterial *q, face_t *f, bool force) {
	float	STfromXYZ[2][4];

	if (!force) {
		EmitBrushPrimitTextureCoordinates(f, f->face_winding);
	}
	else {
		Face_TextureVectors(f, STfromXYZ);
		xyzst[3] = DotProduct(xyzst, STfromXYZ[0]) + STfromXYZ[0][3];
		xyzst[4] = DotProduct(xyzst, STfromXYZ[1]) + STfromXYZ[1][3];
	}
}

/*
================
Brush_MakeFacePlanes
================
*/
void Brush_MakeFacePlanes(brush_t *b) {
	face_t	*f;

	for (f = b->brush_faces; f; f = f->next) {
		Face_MakePlane(f);
	}
}

/*
================
DrawBrushEntityName
================
*/
void DrawBrushEntityName(brush_t *b, const idVec3& color) {
	const char	*name;

	// float a, s, c; vec3_t mid; int i;
	if (!b->owner) {
		return; // during contruction
	}

	if (b->owner == world_entity) {
		return;
	}

	if (b != b->owner->brushes.onext) {
		return; // not key brush
	}

	if (!(g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES)) {
		// draw the angle pointer (e.g. little arrow o na speaker entity)
		float angle = b->owner->FloatForKey("angle");
		if (angle) {
			float s = sin( DEG2RAD( angle ) );
			float c = cos( DEG2RAD( angle ) );

			const idVec3 delta4 = idVec3(c * 4, s * 4, s * 4);
			const idVec3 delta8 = idVec3( c * 8, s * 8, s * 8 );

			idVec3 mid = (b->mins + b->maxs) / 2.0f;

			idVec3 prev = mid;
			mid[0] += c * 8;
			mid[1] += s * 8;
			mid[2] += s * 8;
			g_qeglobals.lineBuffer.Add( prev, mid, color );

			prev = mid;
			mid[0] -= c * 4;
			mid[1] -= s * 4;
			mid[2] -= s * 4;
			mid[0] -= s * 4;
			mid[1] += c * 4;
			mid[2] += c * 4;
			g_qeglobals.lineBuffer.Add( prev, mid, color );

			prev = mid;
			mid[0] += c * 4;
			mid[1] += s * 4;
			mid[2] += s * 4;
			mid[0] += s * 4;
			mid[1] -= c * 4;
			mid[2] -= c * 4;
			g_qeglobals.lineBuffer.Add( prev, mid, color );

			prev = mid;
			mid[0] -= c * 4;
			mid[1] -= s * 4;
			mid[2] -= s * 4;
			mid[0] += s * 4;
			mid[1] -= c * 4;
			mid[2] -= c * 4;
			g_qeglobals.lineBuffer.Add( prev, mid, color );
		}
	}

	ViewType viewType = g_pParentWnd->ActiveXY()->GetViewType();
	float scale = g_pParentWnd->ActiveXY()->Scale();

	if (g_qeglobals.d_savedinfo.show_names && scale >= 1.0f) {
		name = b->owner->ValueForKey("name");
		int nameLen = strlen(name);
		if ( nameLen == 0 ) {
			name = b->owner->ValueForKey("classname");
			nameLen = strlen(name);
		}
		if ( nameLen > 0 ) {
			idVec3 origin = b->owner->origin;

			float halfWidth = ( (nameLen / 2) *  (7.0f / scale) );
			float halfHeight = 4.0f / scale;

			switch (viewType) {
			case ViewType::XY:
				origin.x -= halfWidth;
				origin.y += halfHeight;
				break;
			case ViewType::XZ:
				origin.x -= halfWidth;
				origin.z += halfHeight;
				break;
			case ViewType::YZ:
				origin.y -= halfWidth;
				origin.z += halfHeight;
				break;
			}

			drawText(name, 1.0f, origin, color, viewType);
		}
	}
}

/*
================
Brush_MakeFaceWinding

  returns the visible winding
================
*/
idWinding *Brush_MakeFaceWinding(brush_t *b, face_t *face, bool keepOnPlaneWinding) {
	idWinding	*w;
	face_t		*clip;
	idPlane		plane;
	bool		past;

	// get a poly that covers an effectively infinite area
	w = new idWinding( face->plane );

	// chop the poly by all of the other faces
	past = false;
	for (clip = b->brush_faces; clip && w; clip = clip->next) {
		if (clip == face) {
			past = true;
			continue;
		}

		if ( DotProduct(face->plane, clip->plane) > 0.999f &&
				idMath::Fabs(face->plane[3] - clip->plane[3]) < 0.01f ) { // identical plane, use the later one
			if (past) {
				delete w;
				common->Printf("Unable to create face winding on brush\n");
				return NULL;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		VectorSubtract(vec3_origin, clip->plane, plane );
		plane[3] = -clip->plane[3];

		w = w->Clip( plane, ON_EPSILON, keepOnPlaneWinding );
		if ( !w ) {
			return w;
		}
	}

	if ( w->GetNumPoints() < 3) {
		delete w;
		w = NULL;
	}

	if (!w) {
		Sys_Status("Unable to create face winding on brush\n");
	}
	return w;
}

/*
================
Brush_Build

  Builds a brush rendering data and also sets the min/max bounds
  TTimo added a bConvert flag to convert between old and new brush texture formats
  TTimo brush grouping: update the group treeview if necessary
================
*/
void Brush_Build(brush_t *b, bool bSnap, bool bMarkMap, bool bConvert, bool updateLights) {
	bool bLocalConvert = false;

	//
	// if bConvert is set and g_qeglobals.bNeedConvert is not, that just means we need
	// convert for this brush only
	//
	if (bConvert && !g_qeglobals.bNeedConvert) {
		bLocalConvert = true;
		g_qeglobals.bNeedConvert = true;
	}

	/* build the windings and generate the bounding box */
	Brush_BuildWindings(b, bSnap, b->owner && b->owner->HasModel() || b->pPatch, updateLights);

	/* move the points and edges if in select mode */
	if (g_qeglobals.d_select_mode == sel_vertex || g_qeglobals.d_select_mode == sel_edge) {
		SetupVertexSelection();
	}

	if (bMarkMap) {
		Sys_MarkMapModified();
		g_pParentWnd->GetCameraWindow()->MarkWorldDirty();
	}

	if (bLocalConvert) {
		g_qeglobals.bNeedConvert = false;
	}
}

/*
================
Brush_SplitBrushByFace

  The incoming brush is NOT freed. The incoming face is NOT left referenced.
================
*/
void Brush_SplitBrushByFace(brush_t *in, face_t *f, brush_t **front, brush_t **back) {
	brush_t *b;
	face_t	*nf;
	idVec3	temp;

	b = Brush_Clone(in);
	nf = Face_Clone(f);

	nf->texdef = b->brush_faces->texdef;
	nf->brushprimit_texdef = b->brush_faces->brushprimit_texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	Brush_Build(b);
	Brush_RemoveEmptyFaces(b);
	if (!b->brush_faces) {	// completely clipped away
		Brush_Free(b);
		*back = NULL;
	}
	else {
		Entity_LinkBrush(in->owner, b);
		*back = b;
	}

	b = Brush_Clone(in);
	nf = Face_Clone(f);

	// swap the plane winding
	VectorCopy(nf->planepts[0], temp);
	VectorCopy(nf->planepts[1], nf->planepts[0]);
	VectorCopy(temp, nf->planepts[1]);

	nf->texdef = b->brush_faces->texdef;
	nf->brushprimit_texdef = b->brush_faces->brushprimit_texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	Brush_Build(b);
	Brush_RemoveEmptyFaces(b);
	if (!b->brush_faces) {	// completely clipped away
		Brush_Free(b);
		*front = NULL;
	}
	else {
		Entity_LinkBrush(in->owner, b);
		*front = b;
	}
}

/*
================
Brush_BestSplitFace

  returns the best face to split the brush with. return NULL if the brush is convex
================
*/
face_t *Brush_BestSplitFace(brush_t *b) {
	face_t		*face, *f, *bestface;
	idWinding	*front, *back;
	int			splits, tinywindings, value, bestvalue;

	bestvalue = 999999;
	bestface = NULL;
	for ( face = b->brush_faces; face; face = face->next ) {
		splits = 0;
		tinywindings = 0;
		for ( f = b->brush_faces; f; f = f->next ) {
			if ( f == face ) {
				continue;
			}

			f->face_winding->Split( face->plane, 0.1f, &front, &back );

			if ( !front ) {
				delete back;
			}
			else if ( !back ) {
				delete front;
			}
			else {
				splits++;
				if ( front->IsTiny() ) {
					tinywindings++;
				}

				if ( back->IsTiny() ) {
					tinywindings++;
				}
				delete front;
				delete back;
			}
		}

		if ( splits ) {
			value = splits + 50 * tinywindings;
			if ( value < bestvalue ) {
				bestvalue = value;
				bestface = face;
			}
		}
	}

	return bestface;
}

/*
================
Brush_MakeConvexBrushes

  MrE FIXME: this doesn't work because the old Brush_SplitBrushByFace is used
  Turns the brush into a minimal number of convex brushes.
  If the input brush is convex then it will be returned. Otherwise the input
  brush will be freed.
  NOTE: the input brush should have windings for the faces.
================
*/
brush_t *Brush_MakeConvexBrushes(brush_t *b) {
	brush_t *front, *back, *end;
	face_t	*face;

	b->next = NULL;
	face = Brush_BestSplitFace(b);
	if (!face) {
		return b;
	}

	Brush_SplitBrushByFace(b, face, &front, &back);

	// this should never happen
	if (!front && !back) {
		return b;
	}

	Brush_Free(b);
	if (!front) {
		return Brush_MakeConvexBrushes(back);
	}

	b = Brush_MakeConvexBrushes(front);
	if (back) {
		for (end = b; end->next; end = end->next);
		end->next = Brush_MakeConvexBrushes(back);
	}

	return b;
}

/*
================
Brush_Convex

  returns true if the brush is convex
================
*/
int Brush_Convex(brush_t *b) {
	face_t	*face1, *face2;

	for (face1 = b->brush_faces; face1; face1 = face1->next) {
		if (!face1->face_winding) {
			continue;
		}

		for (face2 = b->brush_faces; face2; face2 = face2->next) {
			if (face1 == face2) {
				continue;
			}

			if (!face2->face_winding) {
				continue;
			}

			if ( face1->face_winding->PlanesConcave( *face2->face_winding,
					face1->plane.Normal(), face2->plane.Normal(), -face1->plane[3], -face2->plane[3] ) ) {
				return false;
			}
		}
	}

	return true;
}

/*
================
Brush_MoveVertexes

  The input brush must be convex.
  The input brush must have face windings.
  The output brush will be convex.
  Returns true if the WHOLE vertex movement is performed.
================
*/
#define MAX_MOVE_FACES	64
#define TINY_EPSILON	0.0325f

int Brush_MoveVertex(brush_t *b, const idVec3 &vertex, const idVec3 &delta, idVec3 &end, bool bSnap) {
	face_t		*f, *face, *newface, *lastface, *nextface;
	face_t		*movefaces[MAX_MOVE_FACES];
	int			movefacepoints[MAX_MOVE_FACES];
	idWinding	*w, tmpw(3);
	idVec3		start, mid;
	idPlane		plane;
	int			i, j, k, nummovefaces, result, done;
	float		dot, front, back, frac, smallestfrac;

	result = true;
	tmpw.SetNumPoints( 3 );
	VectorCopy(vertex, start);
	VectorAdd(vertex, delta, end);

	// snap or not?
	//
	if (bSnap) {
		for (i = 0; i < 3; i++) {
			end[i] = floor( end[i] / 0.125f + 0.5f ) * 0.125f;
		}
	}

	VectorCopy(end, mid);

	// if the start and end are the same
	if ( start.Compare( end, TINY_EPSILON ) ) {
		return false;
	}

	// the end point may not be the same as another vertex
	for ( face = b->brush_faces; face; face = face->next ) {
		w = face->face_winding;
		if (!w) {
			continue;
		}

		for (i = 0; i < w->GetNumPoints(); i++) {
			if ( end.Compare( (*w)[i].ToVec3(), TINY_EPSILON ) ) {
				VectorCopy(vertex, end);
				return false;
			}
		}
	}

	done = false;
	while (!done) {
		//
		// chop off triangles from all brush faces that use the to be moved vertex store
		// pointers to these chopped off triangles in movefaces[]
		//
		nummovefaces = 0;
		for (face = b->brush_faces; face; face = face->next) {
			w = face->face_winding;
			if (!w) {
				continue;
			}

			for (i = 0; i < w->GetNumPoints(); i++) {
				if ( start.Compare( (*w)[i].ToVec3(), TINY_EPSILON ) ) {
					if (face->face_winding->GetNumPoints() <= 3) {
						movefacepoints[nummovefaces] = i;
						movefaces[nummovefaces++] = face;
						break;
					}

					dot = DotProduct(end, face->plane) + face->plane[3];

					// if the end point is in front of the face plane
					//if ( dot > 0.1f ) {
					if ( dot > TINY_EPSILON ) {
						// fanout triangle subdivision
						for (k = i; k < i + w->GetNumPoints() - 3; k++) {
							VectorCopy((*w)[i], tmpw[0]);
							VectorCopy((*w)[(k + 1) % w->GetNumPoints()], tmpw[1]);
							VectorCopy((*w)[(k + 2) % w->GetNumPoints()], tmpw[2]);
							newface = Face_Clone(face);

							// get the original
							for (f = face; f->original; f = f->original) {};

							newface->original = f;

							// store the new winding
							if (newface->face_winding) {
								delete newface->face_winding;
							}

							newface->face_winding = tmpw.Copy();

							// get the texture
							newface->d_texture = Texture_ForName(newface->texdef.name);

							// add the face to the brush
							newface->next = b->brush_faces;
							b->brush_faces = newface;

							// add this new triangle to the move faces
							movefacepoints[nummovefaces] = 0;
							movefaces[nummovefaces++] = newface;
						}

						// give the original face a new winding
						VectorCopy((*w)[(i - 2 + w->GetNumPoints()) % w->GetNumPoints()], tmpw[0]);
						VectorCopy((*w)[(i - 1 + w->GetNumPoints()) % w->GetNumPoints()], tmpw[1]);
						VectorCopy((*w)[i], tmpw[2]);
						delete face->face_winding;
						face->face_winding = tmpw.Copy();

						// add the original face to the move faces
						movefacepoints[nummovefaces] = 2;
						movefaces[nummovefaces++] = face;
					}
					else {
						// chop a triangle off the face
						VectorCopy((*w)[(i - 1 + w->GetNumPoints()) % w->GetNumPoints()], tmpw[0]);
						VectorCopy((*w)[i], tmpw[1]);
						VectorCopy((*w)[(i + 1) % w->GetNumPoints()], tmpw[2]);

						// remove the point from the face winding
						w->RemovePoint( i );

						// get texture crap right
						Face_SetColor(b, face, 1.0);
						for (j = 0; j < w->GetNumPoints(); j++) {
							EmitTextureCoordinates( (*w)[j], face->d_texture, face );
						}

						// make a triangle face
						newface = Face_Clone(face);

						// get the original
						for (f = face; f->original; f = f->original) {};

						newface->original = f;

						// store the new winding
						if (newface->face_winding) {
							delete newface->face_winding;
						}

						newface->face_winding = tmpw.Copy();

						// get the texture
						newface->d_texture = Texture_ForName(newface->texdef.name);

						// add the face to the brush
						newface->next = b->brush_faces;
						b->brush_faces = newface;
						movefacepoints[nummovefaces] = 1;
						movefaces[nummovefaces++] = newface;
					}
					break;
				}
			}
		}

		//
		// now movefaces contains pointers to triangle faces that contain the to be moved
		// vertex
		//
		done = true;
		VectorCopy(end, mid);
		smallestfrac = 1;
		for (face = b->brush_faces; face; face = face->next) {
			// check if there is a move face that has this face as the original
			for (i = 0; i < nummovefaces; i++) {
				if (movefaces[i]->original == face) {
					break;
				}
			}

			if (i >= nummovefaces) {
				continue;
			}

			// check if the original is not a move face itself
			for (j = 0; j < nummovefaces; j++) {
				if (face == movefaces[j]) {
					break;
				}
			}

			// if the original is not a move face itself
			if (j >= nummovefaces) {
				memcpy(&plane, &movefaces[i]->original->plane, sizeof(plane));
			}
			else {
				k = movefacepoints[j];
				w = movefaces[j]->face_winding;
				VectorCopy((*w)[(k + 1) % w->GetNumPoints()], tmpw[0]);
				VectorCopy((*w)[(k + 2) % w->GetNumPoints()], tmpw[1]);

				k = movefacepoints[i];
				w = movefaces[i]->face_winding;
				VectorCopy((*w)[(k + 1) % w->GetNumPoints()], tmpw[2]);

				if ( !plane.FromPoints( tmpw[0].ToVec3(), tmpw[1].ToVec3(), tmpw[2].ToVec3(), false ) ) {
					VectorCopy((*w)[(k + 2) % w->GetNumPoints()], tmpw[2]);
					if ( !plane.FromPoints( tmpw[0].ToVec3(), tmpw[1].ToVec3(), tmpw[2].ToVec3() ), false ) {
						// this should never happen otherwise the face merge did
						// a crappy job a previous pass
						continue;
					}
				}
				plane[0] = -plane[0];
				plane[1] = -plane[1];
				plane[2] = -plane[2];
				plane[3] = -plane[3];
			}

			// now we've got the plane to check against
			front = DotProduct(start, plane) + plane[3];
			back = DotProduct(end, plane) + plane[3];

			// if the whole move is at one side of the plane
			if (front < TINY_EPSILON && back < TINY_EPSILON) {
				continue;
			}

			if (front > -TINY_EPSILON && back > -TINY_EPSILON) {
				continue;
			}

			// if there's no movement orthogonal to this plane at all
			if ( idMath::Fabs(front - back) < 0.001f ) {
				continue;
			}

			// ok first only move till the plane is hit
			frac = front / (front - back);
			if (frac < smallestfrac) {
				mid[0] = start[0] + (end[0] - start[0]) * frac;
				mid[1] = start[1] + (end[1] - start[1]) * frac;
				mid[2] = start[2] + (end[2] - start[2]) * frac;
				smallestfrac = frac;
			}

			done = false;
		}

		// move the vertex
		for (i = 0; i < nummovefaces; i++) {
			// move vertex to end position
			VectorCopy( mid, (*movefaces[i]->face_winding)[movefacepoints[i]] );

			// create new face plane
			for (j = 0; j < 3; j++) {
				VectorCopy( (*movefaces[i]->face_winding)[j], movefaces[i]->planepts[j] );
			}

			Face_MakePlane( movefaces[i] );
			if ( movefaces[i]->plane.Normal().Length() < TINY_EPSILON ) {
				result = false;
			}
		}

		// if the brush is no longer convex
		if (!result || !Brush_Convex(b)) {
			for (i = 0; i < nummovefaces; i++) {
				// move the vertex back to the initial position
				VectorCopy( start, (*movefaces[i]->face_winding)[movefacepoints[i]] );

				// create new face plane
				for (j = 0; j < 3; j++) {
					VectorCopy( (*movefaces[i]->face_winding)[j], movefaces[i]->planepts[j] );
				}

				Face_MakePlane(movefaces[i]);
			}

			result = false;
			VectorCopy(start, end);
			done = true;
		}
		else {
			VectorCopy(mid, start);
		}

		// get texture crap right
		for (i = 0; i < nummovefaces; i++) {
			Face_SetColor( b, movefaces[i], 1.0f );
			for (j = 0; j < movefaces[i]->face_winding->GetNumPoints(); j++) {
				EmitTextureCoordinates( (*movefaces[i]->face_winding)[j], movefaces[i]->d_texture, movefaces[i] );
			}
		}

		// now try to merge faces with their original faces
		lastface = NULL;
		for (face = b->brush_faces; face; face = nextface) {
			nextface = face->next;
			if (!face->original) {
				lastface = face;
				continue;
			}

			if ( !face->plane.Compare( face->original->plane, 0.0001f ) ) {
				lastface = face;
				continue;
			}

			w = face->face_winding->TryMerge( *face->original->face_winding, face->plane.Normal(), true );
			if (!w) {
				lastface = face;
				continue;
			}

			delete face->original->face_winding;
			face->original->face_winding = w;

			// get texture crap right
			Face_SetColor( b, face->original, 1.0f );
			for (j = 0; j < face->original->face_winding->GetNumPoints(); j++) {
				EmitTextureCoordinates( (*face->original->face_winding)[j], face->original->d_texture, face->original);
			}

			// remove the face that was merged with the original
			if (lastface) {
				lastface->next = face->next;
			}
			else {
				b->brush_faces = face->next;
			}

			Face_Free(face);
		}
	}

	return result;
}

/*
================
Brush_InsertVertexBetween

  Adds a vertex to the brush windings between the given two points.
================
*/
int Brush_InsertVertexBetween(brush_t *b, idVec3 p1, idVec3 p2) {
	face_t		*face;
	idWinding	*w, *neww;
	idVec3		point;
	int			i, insert;

	if ( p1.Compare( p2, TINY_EPSILON ) ) {
		return false;
	}

	VectorAdd( p1, p2, point );
	VectorScale( point, 0.5f, point );
	insert = false;

	// the end point may not be the same as another vertex
	for (face = b->brush_faces; face; face = face->next) {
		w = face->face_winding;
		if (!w) {
			continue;
		}

		neww = NULL;
		for (i = 0; i < w->GetNumPoints(); i++) {
			if (! p1.Compare((*w)[i].ToVec3(), TINY_EPSILON)) {
				continue;
			}

			if ( p2.Compare( (*w)[(i + 1) % w->GetNumPoints()].ToVec3(), TINY_EPSILON ) ) {
				neww = new idWinding( *w );
				neww->InsertPoint( point, (i + 1) % w->GetNumPoints() );
				break;
			}
			else if ( p2.Compare( (*w)[(i - 1 + w->GetNumPoints()) % w->GetNumPoints()].ToVec3(), TINY_EPSILON ) ) {
				neww = new idWinding( *w );
				neww->InsertPoint( point, i );
				break;
			}
		}

		if (neww) {
			delete face->face_winding;
			face->face_winding = neww;
			insert = true;
		}
	}

	return insert;
}

/*
================
Brush_ResetFaceOriginals

  reset points to original faces to NULL
================
*/
void Brush_ResetFaceOriginals(brush_t *b) {
	face_t	*face;

	for (face = b->brush_faces; face; face = face->next) {
		face->original = NULL;
	}
}

/*
================
Brush_Parse

  The brush is NOT linked to any list
  FIXME: when using old brush primitives, the test loop for "Brush" and "patchDef2" "patchDef3"
  run before each face parsing. It works, but it's a performance hit
================
*/
brush_t *Brush_Parse(idVec3 origin) {
	brush_t *b;
	face_t	*f;
	int		i, j;
	idVec3	useOrigin = origin;

	g_qeglobals.d_parsed_brushes++;
	b = Brush_Alloc();
	do {
		if (!GetToken(true)) {
			break;
		}

		if (!strcmp(token, "}")) {
			break;
		}

		// handle "Brush" primitive
		if ( idStr::Icmp(token, "brushDef") == 0 || idStr::Icmp(token, "brushDef2") == 0 || idStr::Icmp(token, "brushDef3") == 0 ) {
			// Timo parsing new brush format
			g_qeglobals.bPrimitBrushes = true;

			// check the map is not mixing the two kinds of brushes
			if (g_qeglobals.bOldBrushes) {
				common->Printf("Warning : old brushes and brush primitive in the same file are not allowed ( Brush_Parse )\n");
			}

			bool	newFormat = false;
			if ( idStr::Icmp(token, "brushDef2") == 0 ) {
				newFormat = true;

				// useOrigin.Zero();
			}
			else if ( idStr::Icmp(token, "brushDef3") == 0 ) {
				newFormat = true;
			}


			BrushPrimit_Parse(b, newFormat, useOrigin);

			if (newFormat) {
				//Brush_BuildWindings(b, true, true, false, false);
			}

			if (b == NULL) {
				Warning("parsing brush primitive");
				return NULL;
			}
			else {
				continue;
			}
		}

		if ( idStr::Icmp(token, "patchDef2") == 0 || idStr::Icmp(token, "patchDef3") == 0 ) {
			Brush_Free(b);

			// double string compare but will go away soon
			b = Patch_Parse( idStr::Icmp(token, "patchDef2") == 0 );
			if (b == NULL) {
				Warning("parsing patch/brush");
				return NULL;
			}
			else {
				continue;
			}

			// handle inline patch
		}
		else {
			// Timo parsing old brush format
			g_qeglobals.bOldBrushes = true;

			// check the map is not mixing the two kinds of brushes
			if (g_qeglobals.bPrimitBrushes) {
				common->Printf("Warning : old brushes and brush primitive in the same file are not allowed ( Brush_Parse )\n");
			}

			// set the "need" conversion flag
			g_qeglobals.bNeedConvert = true;


			f = Face_Alloc();

			//
			// add the brush to the end of the chain, so loading and saving a map doesn't
			// reverse the order
			//
			f->next = NULL;
			if (!b->brush_faces) {
				b->brush_faces = f;
			}
			else {
				face_t	*scan;
				for (scan = b->brush_faces; scan->next; scan = scan->next)
					;
				scan->next = f;
			}

			// read the three point plane definition
			for (i = 0; i < 3; i++) {
				if (i != 0) {
					GetToken(true);
				}

				if (strcmp(token, "(")) {
					Warning("parsing brush");
					return NULL;
				}

				for (j = 0; j < 3; j++) {
					GetToken(false);
					f->planepts[i][j] = atof(token);
				}

				GetToken(false);
				if (strcmp(token, ")")) {
					Warning("parsing brush");
					return NULL;
				}
			}
		}

		// read the texturedef
		GetToken(false);
		f->texdef.SetName(token);
		if (token[0] == '(') {
			int i = 32;
		}

		GetToken(false);
		f->texdef.shift[0] = atoi(token);
		GetToken(false);
		f->texdef.shift[1] = atoi(token);
		GetToken(false);
		f->texdef.rotate = atoi(token);
		GetToken(false);
		f->texdef.scale[0] = atof(token);
		GetToken(false);
		f->texdef.scale[1] = atof(token);

		// the flags and value field aren't necessarily present
		f->d_texture = Texture_ForName(f->texdef.name);

		//
		// FIXME: idMaterial f->texdef.flags = f->d_texture->flags; f->texdef.value =
		// f->d_texture->value; f->texdef.contents = f->d_texture->contents;
		//
		if (TokenAvailable()) {
			GetToken(false);
			GetToken(false);
			GetToken(false);
			f->texdef.value = atoi(token);
		}
	} while (1);

	return b;
}

/*
================
QERApp_MapPrintf_FILE

  callback for surface properties plugin must fit a PFN_QERAPP_MAPPRINTF ( see isurfaceplugin.h )
  carefully initialize !
================
*/
FILE	*g_File;

void WINAPI QERApp_MapPrintf_FILE(char *text, ...) {
	va_list argptr;
	char	buf[32768];

	va_start(argptr, text);
	vsprintf(buf, text, argptr);
	va_end(argptr);

	fprintf(g_File, buf);
}

/*
================
Brush_SetEpair

  sets an epair for the given brush
================
*/
void Brush_SetEpair(brush_t *b, const char *pKey, const char *pValue) {
	if (b->pPatch) {
		Patch_SetEpair(b->pPatch, pKey, pValue);
	}
	else {
		b->epairs.Set(pKey, pValue);
	}
}

/*
================
Brush_GetKeyValue
================
*/
const char *Brush_GetKeyValue(brush_t *b, const char *pKey) {
	if (b->pPatch) {
		return Patch_GetKeyValue(b->pPatch, pKey);
	}
	else {
		return b->epairs.GetString(pKey);
	}
}

/*
================
Brush_Write

	save all brushes as Brush primitive format
================
*/
void Brush_Write(brush_t *b, FILE *f, const idVec3 &origin, bool newFormat) {
	if (b->pPatch) {
		Patch_Write(b->pPatch, f);
		return;
	}


	// save brush primitive format
	if (newFormat) {
		WriteFileString(f, "{\nbrushDef3\n{\n");
	}
	else {
		WriteFileString(f, "{\nbrushDef\n{\n");
	}

	// brush epairs
	int count = b->epairs.GetNumKeyVals();
	for (int j = 0; j < count; j++) {
		WriteFileString(f, "\"%s\" \"%s\"\n", b->epairs.GetKeyVal(j)->GetKey().c_str(), b->epairs.GetKeyVal(j)->GetValue().c_str());
	}

	for (auto fa = b->brush_faces; fa; fa = fa->next) {
		// save planepts
		if (newFormat) {
			idPlane plane;

			if (fa->dirty) {
				fa->planepts[0] -= origin;
				fa->planepts[1] -= origin;
				fa->planepts[2] -= origin;
				plane.FromPoints( fa->planepts[0], fa->planepts[1], fa->planepts[2], false );
				fa->planepts[0] += origin;
				fa->planepts[1] += origin;
				fa->planepts[2] += origin;
			} else {
				plane = fa->originalPlane;
			}

			WriteFileString(f, " ( ");
			for (int i = 0; i < 4; i++) {
				if (plane[i] == (int)plane[i]) {
					WriteFileString(f, "%i ", (int)plane[i]);
				}
				else {
					WriteFileString(f, "%f ", plane[i]);
				}
			}

			WriteFileString(f, ") ");
		}
		else {
			for (int i = 0; i < 3; i++) {
				WriteFileString(f, "( ");
				for (int j = 0; j < 3; j++) {
					if (fa->planepts[i][j] == static_cast<int>(fa->planepts[i][j])) {
						WriteFileString(f, "%i ", static_cast<int>(fa->planepts[i][j]));
					}
					else {
						WriteFileString(f, "%f ", fa->planepts[i][j]);
					}
				}

				WriteFileString(f, ") ");
			}
		}

		// save texture coordinates
		WriteFileString(f, "( ( ");
		for (int i = 0; i < 3; i++) {
			if (fa->brushprimit_texdef.coords[0][i] == static_cast<int>(fa->brushprimit_texdef.coords[0][i])) {
				WriteFileString(f, "%i ", static_cast<int>(fa->brushprimit_texdef.coords[0][i]));
			}
			else {
				WriteFileString(f, "%f ", fa->brushprimit_texdef.coords[0][i]);
			}
		}

		WriteFileString(f, ") ( ");
		for (int i = 0; i < 3; i++) {
			if (fa->brushprimit_texdef.coords[1][i] == static_cast<int>(fa->brushprimit_texdef.coords[1][i])) {
				WriteFileString(f, "%i ", static_cast<int>(fa->brushprimit_texdef.coords[1][i]));
			}
			else {
				WriteFileString(f, "%f ", fa->brushprimit_texdef.coords[1][i]);
			}
		}

		WriteFileString(f, ") ) ");

		char	*pName = strlen(fa->texdef.name) > 0 ? fa->texdef.name : "notexture";
		WriteFileString(f, "\"%s\" ", pName);
		WriteFileString(f, "%i %i %i\n", 0, 0, 0);
	}

	WriteFileString(f, "}\n}\n");
}

/*
================
Brush_Write

  save all brushes as Brush primitive format to a CMemFile*
================
*/
void Brush_Write(brush_t *b, CMemFile *pMemFile, const idVec3 &origin, bool newFormat) {
	if (b->pPatch) {
		Patch_Write(b->pPatch, pMemFile);
		return;
	}


	// brush primitive format
	if (newFormat) {
		MemFile_fprintf(pMemFile, "{\nBrushDef2\n{\n");
	}
	else {
		MemFile_fprintf(pMemFile, "{\nBrushDef\n{\n");
	}

	// brush epairs
	// brush epairs
	int count = b->epairs.GetNumKeyVals();
	for (int j = 0; j < count; j++) {
		MemFile_fprintf(pMemFile, "\"%s\" \"%s\"\n", b->epairs.GetKeyVal(j)->GetKey().c_str(), b->epairs.GetKeyVal(j)->GetValue().c_str());
	}

	for (auto fa = b->brush_faces; fa; fa = fa->next) {
		if (newFormat) {
			// save planepts
			idPlane plane;

			if (fa->dirty) {
				fa->planepts[0] -= origin;
				fa->planepts[1] -= origin;
				fa->planepts[2] -= origin;
				plane.FromPoints( fa->planepts[0], fa->planepts[1], fa->planepts[2], false );
				fa->planepts[0] += origin;
				fa->planepts[1] += origin;
				fa->planepts[2] += origin;
			} else {
				plane = fa->originalPlane;
			}

			MemFile_fprintf(pMemFile, " ( ");
			for (int i = 0; i < 4; i++) {
				if (plane[i] == (int)plane[i]) {
					MemFile_fprintf(pMemFile, "%i ", (int)plane[i]);
				}
				else {
					MemFile_fprintf(pMemFile, "%f ", plane[i]);
				}
			}

			MemFile_fprintf(pMemFile, ") ");
		}
		else {
			for (int i = 0; i < 3; i++) {
				MemFile_fprintf(pMemFile, "( ");
				for (int j = 0; j < 3; j++) {
					if (fa->planepts[i][j] == static_cast<int>(fa->planepts[i][j])) {
						MemFile_fprintf(pMemFile, "%i ", static_cast<int>(fa->planepts[i][j]));
					}
					else {
						MemFile_fprintf(pMemFile, "%f ", fa->planepts[i][j]);
					}
				}

				MemFile_fprintf(pMemFile, ") ");
			}
		}

		// save texture coordinates
		MemFile_fprintf(pMemFile, "( ( ");
		for (int i = 0; i < 3; i++) {
			if (fa->brushprimit_texdef.coords[0][i] == static_cast<int>(fa->brushprimit_texdef.coords[0][i])) {
				MemFile_fprintf(pMemFile, "%i ", static_cast<int>(fa->brushprimit_texdef.coords[0][i]));
			}
			else {
				MemFile_fprintf(pMemFile, "%f ", fa->brushprimit_texdef.coords[0][i]);
			}
		}

		MemFile_fprintf(pMemFile, ") ( ");
		for (int i = 0; i < 3; i++) {
			if (fa->brushprimit_texdef.coords[1][i] == static_cast<int>(fa->brushprimit_texdef.coords[1][i])) {
				MemFile_fprintf(pMemFile, "%i ", static_cast<int>(fa->brushprimit_texdef.coords[1][i]));
			}
			else {
				MemFile_fprintf(pMemFile, "%f ", fa->brushprimit_texdef.coords[1][i]);
			}
		}

		MemFile_fprintf(pMemFile, ") ) ");

		// save texture attribs
		char	*pName = strlen(fa->texdef.name) > 0 ? fa->texdef.name : "unnamed";
		MemFile_fprintf(pMemFile, "\"%s\" ", pName);
		MemFile_fprintf(pMemFile, "%i %i %i\n", 0, 0, 0);
	}

	MemFile_fprintf(pMemFile, "}\n}\n");
}

/*
================
Brush_Create

  Create non-textured blocks for entities The brush is NOT linked to any list
================
*/
brush_t *Brush_Create(idVec3 mins, idVec3 maxs, texdef_t *texdef) {
	int		i, j;
	idVec3	pts[4][2];
	face_t	*f;
	brush_t *b;

	//
	// brush primitive mode : convert texdef to brushprimit_texdef ? most of the time
	// texdef is empty
	//
	for (i = 0; i < 3; i++) {
		if (maxs[i] < mins[i]) {
			Error("Brush_InitSolid: backwards");
		}
	}

	b = Brush_Alloc();

	pts[0][0][0] = mins[0];
	pts[0][0][1] = mins[1];

	pts[1][0][0] = mins[0];
	pts[1][0][1] = maxs[1];

	pts[2][0][0] = maxs[0];
	pts[2][0][1] = maxs[1];

	pts[3][0][0] = maxs[0];
	pts[3][0][1] = mins[1];

	for (i = 0; i < 4; i++) {
		pts[i][0][2] = mins[2];
		pts[i][1][0] = pts[i][0][0];
		pts[i][1][1] = pts[i][0][1];
		pts[i][1][2] = maxs[2];
	}

	for (i = 0; i < 4; i++) {
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;
		j = (i + 1) % 4;

		VectorCopy(pts[j][1], f->planepts[0]);
		VectorCopy(pts[i][1], f->planepts[1]);
		VectorCopy(pts[i][0], f->planepts[2]);
	}

	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy(pts[0][1], f->planepts[0]);
	VectorCopy(pts[1][1], f->planepts[1]);
	VectorCopy(pts[2][1], f->planepts[2]);

	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy(pts[2][0], f->planepts[0]);
	VectorCopy(pts[1][0], f->planepts[1]);
	VectorCopy(pts[0][0], f->planepts[2]);

	return b;
}

/*
=============
Brush_Scale
=============
*/
void Brush_Scale(brush_t* b) {
	for ( face_t *f = b->brush_faces; f; f = f->next ) {
		for ( int i = 0; i < 3; i++ ) {
			VectorScale( f->planepts[i], g_qeglobals.d_gridsize, f->planepts[i] );
		}
	}
}

/*
================
Brush_CreatePyramid

  Create non-textured pyramid for light entities The brush is NOT linked to any list
================
*/
brush_t *Brush_CreatePyramid(idVec3 mins, idVec3 maxs, texdef_t *texdef) {
	// ++timo handle new brush primitive ? return here ??
	return Brush_Create(mins, maxs, texdef);

	int i;
	for (i = 0; i < 3; i++) {
		if (maxs[i] < mins[i]) {
			Error("Brush_InitSolid: backwards");
		}
	}

	brush_t *b = Brush_Alloc();

	idVec3	corners[4];

	float	fMid = idMath::Rint(mins[2] + (idMath::Rint((maxs[2] - mins[2]) / 2)));

	corners[0][0] = mins[0];
	corners[0][1] = mins[1];
	corners[0][2] = fMid;

	corners[1][0] = mins[0];
	corners[1][1] = maxs[1];
	corners[1][2] = fMid;

	corners[2][0] = maxs[0];
	corners[2][1] = maxs[1];
	corners[2][2] = fMid;

	corners[3][0] = maxs[0];
	corners[3][1] = mins[1];
	corners[3][2] = fMid;

	idVec3	top, bottom;

	top[0] = idMath::Rint(mins[0] + ((maxs[0] - mins[0]) / 2));
	top[1] = idMath::Rint(mins[1] + ((maxs[1] - mins[1]) / 2));
	top[2] = idMath::Rint(maxs[2]);

	VectorCopy(top, bottom);
	bottom[2] = mins[2];

	// sides
	for (i = 0; i < 4; i++) {
		face_t	*f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		int j = (i + 1) % 4;

		VectorCopy(top, f->planepts[0]);
		VectorCopy(corners[i], f->planepts[1]);
		VectorCopy(corners[j], f->planepts[2]);

		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		VectorCopy(bottom, f->planepts[2]);
		VectorCopy(corners[i], f->planepts[1]);
		VectorCopy(corners[j], f->planepts[0]);
	}

	return b;
}

/*
================
Brush_MakeSided

  Makes the current brush have the given number of 2d sides
================
*/
void Brush_MakeSided(int sides) {
	int			i, axis;
	idVec3		mins, maxs;
	brush_t		*b;
	texdef_t	*texdef;
	face_t		*f;
	idVec3		mid;
	float		width;
	float		sv, cv;

	if (sides < 3) {
		Sys_Status("Bad sides number", 0);
		return;
	}

	if (sides >= MAX_POINTS_ON_WINDING - 4) {
		Sys_Status("too many sides.\n");
		return;
	}

	if (!QE_SingleBrush()) {
		Sys_Status("Must have a single brush selected", 0);
		return;
	}

	b = selected_brushes.next;
	VectorCopy(b->mins, mins);
	VectorCopy(b->maxs, maxs);
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free(b);

	if (g_pParentWnd->ActiveXY()) {
		switch (g_pParentWnd->ActiveXY()->GetViewType())
		{
			case ViewType::XY:
				axis = 2;
				break;
			case ViewType::XZ:
				axis = 1;
				break;
			case ViewType::YZ:
				axis = 0;
				break;
		}
	}
	else {
		axis = 2;
	}

	// find center of brush
	width = 8;
	for (i = 0; i < 3; i++) {
		mid[i] = (maxs[i] + mins[i]) * 0.5f;
		if (i == axis) {
			continue;
		}

		if ((maxs[i] - mins[i]) * 0.5f > width) {
			width = (maxs[i] - mins[i]) * 0.5f;
		}
	}

	b = Brush_Alloc();

	// create top face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[2][(axis + 1) % 3] = mins[(axis + 1) % 3];
	f->planepts[2][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[2][axis] = maxs[axis];
	f->planepts[1][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[1][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[1][axis] = maxs[axis];
	f->planepts[0][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[0][(axis + 2) % 3] = maxs[(axis + 2) % 3];
	f->planepts[0][axis] = maxs[axis];

	// create bottom face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[0][(axis + 1) % 3] = mins[(axis + 1) % 3];
	f->planepts[0][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[0][axis] = mins[axis];
	f->planepts[1][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[1][(axis + 2) % 3] = mins[(axis + 2) % 3];
	f->planepts[1][axis] = mins[axis];
	f->planepts[2][(axis + 1) % 3] = maxs[(axis + 1) % 3];
	f->planepts[2][(axis + 2) % 3] = maxs[(axis + 2) % 3];
	f->planepts[2][axis] = mins[axis];

	for (i = 0; i < sides; i++) {
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		sv = sin(i * 3.14159265 * 2 / sides);
		cv = cos(i * 3.14159265 * 2 / sides);

		f->planepts[0][(axis + 1) % 3] = floor(mid[(axis + 1) % 3] + width * cv + 0.5f);
		f->planepts[0][(axis + 2) % 3] = floor(mid[(axis + 2) % 3] + width * sv + 0.5f);
		f->planepts[0][axis] = mins[axis];

		f->planepts[1][(axis + 1) % 3] = f->planepts[0][(axis + 1) % 3];
		f->planepts[1][(axis + 2) % 3] = f->planepts[0][(axis + 2) % 3];
		f->planepts[1][axis] = maxs[axis];

		f->planepts[2][(axis + 1) % 3] = floor(f->planepts[0][(axis + 1) % 3] - width * sv + 0.5f);
		f->planepts[2][(axis + 2) % 3] = floor(f->planepts[0][(axis + 2) % 3] + width * cv + 0.5f);
		f->planepts[2][axis] = maxs[axis];
	}

	Brush_AddToList(b, &selected_brushes);

	Entity_LinkBrush(world_entity, b);

	Brush_Build(b);

	Sys_UpdateWindows(W_ALL);
}

/*
================
Brush_Free

  Frees the brush with all of its faces and display list.
  Unlinks the brush from whichever chain it is in.
  Decrements the owner entity's brushcount.
  Removes owner entity if this was the last brush unless owner is the world.
  Removes from groups

  set bRemoveNode to false to avoid trying to delete the item in group view tree control
================
*/
void Brush_Free(brush_t *b, bool bRemoveNode) {
	face_t	*f, *next;

  if ( b->animSnapshotModel ) {
    delete b->animSnapshotModel;
    b->animSnapshotModel = nullptr;
  }

	// free the patch if it's there
	if ( b->pPatch ) {
		Patch_Delete(b->pPatch);
	}

	// free faces
	for ( f = b->brush_faces; f; f = next ) {
		next = f->next;
		Face_Free(f);
	}

	b->epairs.Clear();

	// unlink from active/selected list
	if ( b->next ) {
		Brush_RemoveFromList(b);
	}

	// unlink from entity list
	if ( b->onext ) {
		Entity_UnlinkBrush(b);
	}

	delete b;
}

/*
================
Face_MemorySize

  returns the size in memory of the face
================
*/
int Face_MemorySize(const face_t *face) {
	int size = 0;

	if ( face->face_winding ) {
		size += sizeof( idWinding ) + face->face_winding->GetNumPoints() * sizeof( (face->face_winding)[0] );
	}
	size += sizeof( face_t );
	return size;
}

/*
================
Brush_MemorySize

  returns the size in memory of the brush
================
*/
int Brush_MemorySize( const brush_t *brush ) {
	int size = 0;
	if ( brush->pPatch ) {
		size += Patch_MemorySize( brush->pPatch );
	}

	for ( const face_t* face = brush->brush_faces; face; face = face->next ) {
		size += Face_MemorySize(face);
	}

	size += sizeof( brush_t ) + brush->epairs.Size();
	return size;
}

/*
================
Brush_Clone

  does not add the brush to any lists
================
*/
brush_t *Brush_Clone(brush_t *b) {
	brush_t *n = NULL;
	face_t	*f, *nf;

	if (b->pPatch) {
		patchMesh_t *p = Patch_Duplicate(b->pPatch);
		Brush_RemoveFromList(p->pSymbiot);
		Entity_UnlinkBrush(p->pSymbiot);
		n = p->pSymbiot;
	}
	else {
		n = Brush_Alloc();
		n->numberId = g_nBrushId++;
		n->owner = b->owner;
		n->lightColor = b->lightColor;
		n->lightEnd = b->lightEnd;
		n->lightOffset = b->lightOffset;
		n->lightRadius = b->lightRadius;
		n->lightRight = b->lightRight;
		n->lightStart = b->lightStart;
		n->lightTarget = b->lightTarget;
		n->lightCenter = b->lightCenter;
		n->lightTexture = b->lightTexture;
		n->lightUp = b->lightUp;
		n->modelHandle = b->modelHandle;
		n->pointLight = b->pointLight;
		for (f = b->brush_faces; f; f = f->next) {
			nf = Face_Clone(f);
			nf->next = n->brush_faces;
			n->brush_faces = nf;
		}
	}

	return n;
}

/*
================
Brush_FullClone

  Used by Undo.
  Makes an exact copy of the brush.
  Does NOT add the new brush to any lists.
================
*/
brush_t *Brush_FullClone(brush_t *b) {
	brush_t *n = NULL;
	face_t	*f, *nf, *f2, *nf2;

	if (b->pPatch) {
		patchMesh_t *p = Patch_Duplicate(b->pPatch);
		Brush_RemoveFromList(p->pSymbiot);
		Entity_UnlinkBrush(p->pSymbiot);
		n = p->pSymbiot;
		n->owner = b->owner;
		Brush_Build(n);
	}
	else {
		n = Brush_Alloc();
		n->numberId = g_nBrushId++;
		n->owner = b->owner;
		n->lightColor = b->lightColor;
		n->lightEnd = b->lightEnd;
		n->lightOffset = b->lightOffset;
		n->lightRadius = b->lightRadius;
		n->lightRight = b->lightRight;
		n->lightStart = b->lightStart;
		n->lightTarget = b->lightTarget;
		n->lightCenter = b->lightCenter;
		n->lightTexture = b->lightTexture;
		n->lightUp = b->lightUp;
		n->modelHandle = b->modelHandle;
		n->pointLight = b->pointLight;
		VectorCopy(b->mins, n->mins);
		VectorCopy(b->maxs, n->maxs);
		for (f = b->brush_faces; f; f = f->next) {
			if (f->original) {
				continue;
			}

			nf = Face_FullClone(f);
			nf->next = n->brush_faces;
			n->brush_faces = nf;

			// copy all faces that have the original set to this face
			for (f2 = b->brush_faces; f2; f2 = f2->next) {
				if (f2->original == f) {
					nf2 = Face_FullClone(f2);
					nf2->next = n->brush_faces;
					n->brush_faces = nf2;

					// set original
					nf2->original = nf;
				}
			}
		}

		for (nf = n->brush_faces; nf; nf = nf->next) {
			Face_SetColor( n, nf, 1.0f );
			if (nf->face_winding) {
				EmitBrushPrimitTextureCoordinates(nf, nf->face_winding);
			}
		}
	}

	return n;
}

extern bool Patch_Intersect(patchMesh_t *pm, idVec3 origin, idVec3 direction , float &scale);
extern bool RayIntersectsTri
			(
				const idVec3	&origin,
				const idVec3	&direction,
				const idVec3	&vert0,
				const idVec3	&vert1,
				const idVec3	&vert2,
                float           &scale
			);


/*
================
RotateVector
================
*/
void RotateVector(idVec3 &v, idVec3 origin, float a, float c, float s) {
	float	x = v[0];
	float	y = v[1];
	if (a) {
		float x2 = (((x - origin[0]) * c) - ((y - origin[1]) * s)) + origin[0];
		float y2 = (((x - origin[0]) * s) + ((y - origin[1]) * c)) + origin[1];
		x = x2;
		y = y2;
	}
	v[0] = x;
	v[1] = y;
}
/*
================
Brush_ModelIntersect
================
*/

bool Brush_ModelIntersect(brush_t *b, idVec3 origin, idVec3 dir,float &scale) {

#if 0
	idRenderModel *model = b->modelHandle;
	idRenderModel *md5;

    if ( !model )
        model = b->owner->eclass->entityModel;

    scale = 0;
	if (model) {
		if ( model->IsDynamicModel() != DM_STATIC ) {
			if ( dynamic_cast<idRenderModelMD5 *>( model ) ) {
				// take care of animated models
				md5 = b->owner->eclass->entityModel;

				const char *classname = ValueForKey( b->owner, "classname" );
				if (stricmp(classname, "func_static") == 0) {
					classname = ValueForKey(b->owner, "animclass");
				}
				const char *anim = ValueForKey( b->owner, "anim" );
				int frame = IntForKey( b->owner, "frame" ) + 1;
				if ( frame < 1 ) {
					frame = 1;
				}
				if ( !anim || !anim[ 0 ] ) {
					anim = "idle";
				}
				model = gameEdit->ANIM_CreateMeshForAnim( md5, classname, anim, frame, false );
				if ( !model ) {
					model = renderModelManager->DefaultModel();
				}
			}
		}

		bool matrix = false;
		idMat3 mat;
		float a, s, c;
		if (GetMatrixForKey(b->owner, "rotation", mat)) {
			matrix = true;
		} else {
			a = FloatForKey(b->owner, "angle");
			if (a) {
				s = sin( DEG2RAD( a ) );
				c = cos( DEG2RAD( a ) );
			}
			else {
				s = c = 0;
			}
		}

		for (int i = 0; i < model->NumSurfaces() ; i++) {
			const modelSurface_t	*surf = model->Surface( i );
			srfTriangles_t	*tri = surf->geometry;
			for (int j = 0; j < tri->numIndexes; j += 3) {
				idVec3	v1, v2, v3;
				v1 = tri->verts[tri->indexes[j]].xyz;
				v2 = tri->verts[tri->indexes[j + 1]].xyz;
				v3 = tri->verts[tri->indexes[j + 2]].xyz;

				if (matrix) {
					v1 *= b->owner->rotation;
					v1 += b->owner->origin;
					v2 *= b->owner->rotation;
					v2 += b->owner->origin;
					v3 *= b->owner->rotation;
					v3 += b->owner->origin;
				} else {
					v1 += b->owner->origin;
					v2 += b->owner->origin;
					v3 += b->owner->origin;
					RotateVector(v1, b->owner->origin, a, c, s);
					RotateVector(v2, b->owner->origin, a, c, s);
					RotateVector(v3, b->owner->origin, a, c, s);
				}

				if (RayIntersectsTri(origin, dir, v1, v2, v3,scale)) {
					return true;
				}
			}
		}
	}

	return false;
#else
  editorModel_t editorModel = Brush_GetEditorModel(b);
  idRenderModel* model = editorModel.model;

  bool matrix = false;
  idMat3 mat;
  float a, s, c;
  if (b->owner->GetMatrixForKey("rotation", mat)) {
    matrix = true;
  }
  else {
    a = b->owner->FloatForKey("angle");
    if (a) {
      s = sin(DEG2RAD(a));
      c = cos(DEG2RAD(a));
    }
    else {
      s = c = 0;
    }
  }

  for (int i = 0; i < model->NumSurfaces(); i++) {
    const modelSurface_t	*surf = model->Surface(i);
    srfTriangles_t	*tri = surf->geometry;
    for (int j = 0; j < tri->numIndexes; j += 3) {
      idVec3	v1, v2, v3;
      v1 = tri->verts[tri->indexes[j]].xyz;
      v2 = tri->verts[tri->indexes[j + 1]].xyz;
      v3 = tri->verts[tri->indexes[j + 2]].xyz;

      if (matrix) {
        v1 *= b->owner->rotation;
        v1 += b->owner->origin;
        v2 *= b->owner->rotation;
        v2 += b->owner->origin;
        v3 *= b->owner->rotation;
        v3 += b->owner->origin;
      }
      else {
        v1 += b->owner->origin;
        v2 += b->owner->origin;
        v3 += b->owner->origin;
        RotateVector(v1, b->owner->origin, a, c, s);
        RotateVector(v2, b->owner->origin, a, c, s);
        RotateVector(v3, b->owner->origin, a, c, s);
      }

      if (RayIntersectsTri(origin, dir, v1, v2, v3, scale)) {
        return true;
      }
    }
  }

  return false;
#endif
}

face_t *Brush_Ray(idVec3 origin, idVec3 dir, brush_t *b, float *dist, bool testPrimitive) {
	face_t *firstface = NULL;
	idVec3	p1, p2;
	float	frac, d1, d2;
	int		i;
    float scale = HUGE_DISTANCE * 2;
	VectorCopy(origin, p1);
	for (i = 0; i < 3; i++) {
		p2[i] = p1[i] + dir[i] * HUGE_DISTANCE * 2;
	}

	for (face_t* f = b->brush_faces; f; f = f->next) {
		d1 = DotProduct(p1, f->plane) + f->plane[3];
		d2 = DotProduct(p2, f->plane) + f->plane[3];
		if (d1 >= 0 && d2 >= 0) {
			*dist = 0;
			return NULL;	// ray is on front side of face
		}

		if (d1 <= 0 && d2 <= 0) {
			continue;
		}

		// clip the ray to the plane
		frac = d1 / (d1 - d2);
		if (d1 > 0) {
			firstface = f;
			for (i = 0; i < 3; i++) {
				p1[i] = p1[i] + frac * (p2[i] - p1[i]);
			}
		}
		else {
			for (i = 0; i < 3; i++) {
				p2[i] = p1[i] + frac * (p2[i] - p1[i]);
			}
		}
	}

	// find distance p1 is along dir
	VectorSubtract(p1, origin, p1);
	d1 = DotProduct(p1, dir);

	if (testPrimitive && !g_PrefsDlg.m_selectByBoundingBrush) {
		if (b->pPatch) {
			if (!Patch_Intersect(b->pPatch, origin, dir, scale)) {
				*dist = 0;
				return NULL;
			}
		}
		else if ( b->modelHandle != NULL && dynamic_cast<idRenderModelPrt*>( b->modelHandle ) == NULL && dynamic_cast< idRenderModelLiquid*> ( b->modelHandle ) == NULL ) {
			if (!Brush_ModelIntersect(b, origin, dir, scale)) {
				*dist = 0;
				return NULL;
			}
		}
	}

	*dist = d1;
	return firstface;
}

/*
================
Brush_Point
================
*/
face_t *Brush_Point(idVec3 origin, brush_t *b) {
	face_t	*f;
	float	d1;

	for (f = b->brush_faces; f; f = f->next) {
		d1 = DotProduct(origin, f->plane) + f->plane[3];
		if (d1 > 0) {
			return NULL;	// point is on front side of face
		}
	}

	return b->brush_faces;
}

/*
================
Brush_AddToList
================
*/
void Brush_AddToList(brush_t *b, brush_t *list) {
	if (b->next || b->prev) {
		Error("Brush_AddToList: allready linked");
	}

	if (list == &selected_brushes || list == &active_brushes) {
		if (b->pPatch && list == &selected_brushes) {
			Patch_Select(b->pPatch);
		}
	}

	b->list = list;
	b->next = list->next;
	list->next->prev = b;
	list->next = b;
	b->prev = list;

}

/*
================
Brush_RemoveFromList
================
*/
void Brush_RemoveFromList(brush_t *b) {
	if (!b->next || !b->prev) {
		Error("Brush_RemoveFromList: not linked");
	}

	if (b->pPatch) {
		Patch_Deselect(b->pPatch);

		// Patch_Deselect(b->nPatchID);
	}

	b->list = NULL;
	b->next->prev = b->prev;
	b->prev->next = b->next;
	b->next = b->prev = NULL;
}

/*
================
SetFaceTexdef

  Doesn't set the curve flags.
  NOTE: never trust f->d_texture here, f->texdef and f->d_texture are out of sync when
  called by Brush_SetTexture use Texture_ForName() to find the right shader
  FIXME: send the right shader ( qtexture_t * ) in the parameters ?
  TTimo: surface plugin, added an IPluginTexdef* parameter if not NULL,
  get ->Copy() of it into the face ( and remember to hook ) if NULL, ask for a default
================
*/
void SetFaceTexdef( brush_t *b, face_t *f, texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale ) {
	f->texdef = *texdef;
	ConvertTexMatWithQTexture(brushprimit_texdef, NULL, &f->brushprimit_texdef, Texture_ForName(f->texdef.name));
}

/*
================
Brush_SetTexture
================
*/
void Brush_SetTexture(brush_t *b, texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale) {
	if (b->pPatch) {
		Patch_SetTexture(b->pPatch, texdef);
	}
	else {
		for (face_t * f = b->brush_faces; f; f = f->next) {
			SetFaceTexdef(b, f, texdef, brushprimit_texdef, bFitScale);
		}

		Brush_Build(b);
	}
}

/*
====================
Brush_SetTextureName
====================
*/
void Brush_SetTextureName(brush_t *b, const char *name) {
	if (b->pPatch) {
		Patch_SetTextureName(b->pPatch, name);
	}
	else {
		for (face_t * f = b->brush_faces; f; f = f->next) {
			f->texdef.SetName(name);
		}
		Brush_Build(b);
	}
}

/*
================
ClipLineToFace
================
*/
bool ClipLineToFace(idVec3 &p1, idVec3 &p2, face_t *f) {
	float	d1, d2, fr;
	int		i;
	float	*v;

	d1 = DotProduct(p1, f->plane) + f->plane[3];
	d2 = DotProduct(p2, f->plane) + f->plane[3];

	if (d1 >= 0 && d2 >= 0) {
		return false;	// totally outside
	}

	if (d1 <= 0 && d2 <= 0) {
		return true;	// totally inside
	}

	fr = d1 / (d1 - d2);

	if (d1 > 0) {
		v = p1.ToFloatPtr();
	}
	else {
		v = p2.ToFloatPtr();
	}

	for (i = 0; i < 3; i++) {
		v[i] = p1[i] + fr * (p2[i] - p1[i]);
	}

	return true;
}

/*
================
AddPlanept
================
*/
int AddPlanept(idVec3 *f) {
	int i;

	for (i = 0; i < g_qeglobals.d_num_move_points; i++) {
		if (g_qeglobals.d_move_points[i] == f) {
			return 0;
		}
	}

	if (g_qeglobals.d_num_move_points < MAX_MOVE_POINTS) {
		g_qeglobals.d_move_points[g_qeglobals.d_num_move_points++] = f;
	} else {
		Sys_Status("Trying to move too many points\n");
		return 0;
	}

	return 1;
}

/*
================
AddMovePlane
================
*/
static void AddMovePlane( idPlane *p ) {

	for (int i = 0; i < g_qeglobals.d_num_move_planes; i++) {
		if (g_qeglobals.d_move_planes[i] == p) {
			return;
		}
	}

	if (g_qeglobals.d_num_move_planes < MAX_MOVE_PLANES) {
		g_qeglobals.d_move_planes[g_qeglobals.d_num_move_planes++] = p;
	} else {
		Sys_Status("Trying to move too many planes\n");
	}

}

/*
================
Brush_SelectFaceForDragging

  Adds the faces planepts to move_points, and rotates and adds the planepts of adjacent face if shear is set
================
*/
void Brush_SelectFaceForDragging(brush_t *b, face_t *f, bool shear) {
	int			i;
	face_t		*f2;
	idWinding	*w;
	float		d;
	brush_t		*b2;
	int			c;

	if (b->owner->eclass->fixedsize || b->owner->HasModel()) {
		return;
	}

	c = 0;
	for (i = 0; i < 3; i++) {
		c += AddPlanept(&f->planepts[i]);
	}

	//AddMovePlane(&f->plane);

	if (c == 0) {
		return;				// allready completely added
	}

	// select all points on this plane in all brushes the selection
	for (b2 = selected_brushes.next; b2 != &selected_brushes; b2 = b2->next) {
		if (b2 == b) {
			continue;
		}

		for (f2 = b2->brush_faces; f2; f2 = f2->next) {
			for (i = 0; i < 3; i++) {
				if (idMath::Fabs(DotProduct(f2->planepts[i], f->plane) + f->plane[3]) > ON_EPSILON) {
					break;
				}
			}

			if (i == 3) {	// move this face as well
				Brush_SelectFaceForDragging(b2, f2, shear);
				break;
			}
		}
	}

	//
	// if shearing, take all the planes adjacent to selected faces and rotate their
	// points so the edge clipped by a selcted face has two of the points
	//
	if (!shear) {
		return;
	}

	for (f2 = b->brush_faces; f2; f2 = f2->next) {
		if (f2 == f) {
			continue;
		}

		w = Brush_MakeFaceWinding(b, f2, false);
		if (!w) {
			continue;
		}

		// any points on f will become new control points
		for (i = 0; i < w->GetNumPoints(); i++) {
			d = DotProduct( (*w)[i], f->plane ) + f->plane[3];
			if (d > -ON_EPSILON && d < ON_EPSILON) {
				break;
			}
		}

		// if none of the points were on the plane, leave it alone
		if (i != w->GetNumPoints()) {
			if (i == 0) {	// see if the first clockwise point was the
							///
							///  last point on the winding
				d = DotProduct( (*w)[w->GetNumPoints() - 1], f->plane ) + f->plane[3];
				if (d > -ON_EPSILON && d < ON_EPSILON) {
					i = w->GetNumPoints() - 1;
				}
			}

			AddPlanept(&f2->planepts[0]);
			//AddMovePlane(&f2->plane);

			VectorCopy((*w)[i], f2->planepts[0]);
			if (++i == w->GetNumPoints()) {
				i = 0;
			}

			// see if the next point is also on the plane
			d = DotProduct( (*w)[i], f->plane ) + f->plane[3];
			if (d > -ON_EPSILON && d < ON_EPSILON) {
				AddPlanept(&f2->planepts[1]);
			}

			VectorCopy( (*w)[i], f2->planepts[1] );
			if (++i == w->GetNumPoints()) {
				i = 0;
			}

			// the third point is never on the plane
			VectorCopy( (*w)[i], f2->planepts[2] );
		}

		delete w;
	}
}

/*
================
Brush_SideSelect

  The mouse click did not hit the brush, so grab one or more side planes for dragging.
================
*/
void Brush_SideSelect(brush_t *b, idVec3 origin, idVec3 dir, bool shear) {
	face_t	*f, *f2;
	idVec3	p1, p2;

	if (g_moveOnly) {
		return;
	}

	// if (b->pPatch) return; Patch_SideSelect(b->nPatchID, origin, dir);
	for (f = b->brush_faces; f; f = f->next) {
		VectorCopy(origin, p1);
		VectorMA(origin, MAX_WORLD_SIZE, dir, p2);

		for (f2 = b->brush_faces; f2; f2 = f2->next) {
			if (f2 == f) {
				continue;
			}

			ClipLineToFace(p1, p2, f2);
		}

		if (f2) {
			continue;
		}

		if ( p1.Compare( origin ) ) {
			continue;
		}

		if (ClipLineToFace(p1, p2, f)) {
			continue;
		}

		Brush_SelectFaceForDragging(b, f, shear);
	}
}

extern void UpdateSelectablePoint(brush_t *b, idVec3 v, int type);

/*
================
Brush_TransformedPoint
================
*/

idMat3 Brush_RotationMatrix(brush_t *b) {
	idMat3 mat;
	mat.Identity();
	if (!b->owner->GetMatrixForKey("light_rotation", mat)) {
		b->owner->GetMatrixForKey("rotation", mat);
	}
	return mat;
}

idVec3 Brush_TransformedPoint(brush_t *b, const idVec3 &in) {
	idVec3 out = in;
	out -= b->owner->origin;
	out *= Brush_RotationMatrix(b);
	out += b->owner->origin;
	return out;
}
/*
================
Brush_UpdateLightPoints
================
*/
static void Brush_UpdateLightPoints(brush_t *b, const idVec3 &offset) {

	if (!(b->owner->eclass->nShowFlags & ECLASS_LIGHT)) {
		if (b->modelHandle) {
			g_bScreenUpdates = false;
			g_pParentWnd->GetCameraWindow()->BuildEntityRenderState(b->owner, true);
			g_bScreenUpdates = true;
		}
		return;
	}

	if (b->entityModel) {
		return;
	}

	idVec3	vCenter;
	idVec3 *origin = (b->trackLightOrigin) ? &b->owner->lightOrigin : &b->owner->origin;

	if (!b->owner->GetVectorForKey("_color", b->lightColor)) {
		b->lightColor[0] = b->lightColor[1] = b->lightColor[2] = 1;
	}

	const char	*str = b->owner->ValueForKey("texture");
	b->lightTexture = -1;
	if (str && strlen(str) > 0) {
		const idMaterial	*q = Texture_LoadLight(str);
		if (q) {
			b->lightTexture = q->GetEditorImage()->texnum;
		}
	}

	str = b->owner->ValueForKey("light_right");
	if (str && *str) {
		idVec3	vRight, vUp, vTarget, vTemp;

		if (b->owner->GetVectorForKey("light_start", b->lightStart)) {
			b->startEnd = true;
			if (!b->owner->GetVectorForKey("light_end", b->lightEnd)) {
				b->owner->GetVectorForKey("light_target", b->lightEnd);
			}


			VectorAdd(b->lightEnd, *origin, b->lightEnd);
			VectorAdd(b->lightStart, *origin, b->lightStart);
			VectorAdd(b->lightStart, offset, b->lightStart);
		}
		else {
			b->startEnd = false;
		}

		b->owner->GetVectorForKey("light_right", vRight);
		b->owner->GetVectorForKey("light_up", vUp);
		b->owner->GetVectorForKey("light_target", vTarget);
		if (offset.x || offset.y || offset.z) {
			CString str;
			VectorAdd(vTarget, offset, vTarget);
			b->owner->SetKeyVec3("light_target", vTarget);
		}

		VectorAdd(vTarget, *origin, b->lightTarget);
		VectorAdd(b->lightTarget, vRight, b->lightRight);
		VectorAdd(b->lightTarget, vUp, b->lightUp);

		UpdateSelectablePoint(b, Brush_TransformedPoint(b, b->lightUp), LIGHT_UP);
		UpdateSelectablePoint(b, Brush_TransformedPoint(b, b->lightRight), LIGHT_RIGHT);
		UpdateSelectablePoint(b, Brush_TransformedPoint(b, b->lightTarget), LIGHT_TARGET);
		UpdateSelectablePoint(b, Brush_TransformedPoint(b, b->lightStart), LIGHT_START);
		UpdateSelectablePoint(b, Brush_TransformedPoint(b, b->lightEnd), LIGHT_END);
		b->pointLight = false;
	}
	else {
		b->pointLight = true;

		if (b->owner->GetVectorForKey("light_center", vCenter)) {

			if (offset.x || offset.y || offset.z) {
				CString str;
				VectorAdd(vCenter, offset, vCenter);
				b->owner->SetKeyVec3("light_center", vCenter);
			}

			VectorAdd(vCenter, *origin, b->lightCenter);
			UpdateSelectablePoint(b, b->lightCenter, LIGHT_CENTER);
		}

		if (!b->owner->GetVectorForKey("light_radius", b->lightRadius)) {
			float	f = b->owner->FloatForKey("light");
			if (f == 0) {
				f = 300;
			}

			b->lightRadius[0] = b->lightRadius[1] = b->lightRadius[2] = f;
		}
		else {
		}
	}

	g_bScreenUpdates = false;
	g_pParentWnd->GetCameraWindow()->BuildEntityRenderState(b->owner, true);
	g_bScreenUpdates = true;

}

/*
================
Brush_BuildWindings
================
*/
void Brush_BuildWindings(brush_t *b, bool bSnap, bool keepOnPlaneWinding, bool updateLights, bool makeFacePlanes) {
	idWinding	*w;
	face_t		*face;
	float		v;

	// clear the mins/maxs bounds
	b->mins[0] = b->mins[1] = b->mins[2] = 999999;
	b->maxs[0] = b->maxs[1] = b->maxs[2] = -999999;

	if (makeFacePlanes) {
		Brush_MakeFacePlanes(b);
	}

	face = b->brush_faces;

	float	fCurveColor = 1.0f;

	for (; face; face = face->next) {
		int i, j;
		delete face->face_winding;
		w = face->face_winding = Brush_MakeFaceWinding(b, face, keepOnPlaneWinding);
		face->d_texture = Texture_ForName(face->texdef.name);

		if (!w) {
			continue;
		}

		for (i = 0; i < w->GetNumPoints(); i++) {
			// add to bounding box
			for (j = 0; j < 3; j++) {
				v = (*w)[i][j];
				if (v > b->maxs[j]) {
					b->maxs[j] = v;
				}

				if (v < b->mins[j]) {
					b->mins[j] = v;
				}
			}
		}

		// setup s and t vectors, and set color if (!g_PrefsDlg.m_bGLLighting) {
		if (makeFacePlanes) {
			Face_SetColor(b, face, fCurveColor);

		// }
			fCurveColor -= 0.1f;
			if ( fCurveColor <= 0.0f ) {
				fCurveColor = 1.0f;
			}

			// computing ST coordinates for the windings

			if (g_qeglobals.bNeedConvert) {
				//
				// we have parsed old brushes format and need conversion convert old brush texture
				// representation to new format
				//
				FaceToBrushPrimitFace(face);
#ifdef _DEBUG
				// use old texture coordinates code to check against
				for (i = 0; i < w->GetNumPoints(); i++) {
					EmitTextureCoordinates((*w)[i], face->d_texture, face);
				}
#endif
			}

			//
			// use new texture representation to compute texture coordinates in debug mode we
			// will check against old code and warn if there are differences
			//
			EmitBrushPrimitTextureCoordinates(face, w);
		}

	}

	if (updateLights) {
		idVec3 offset;
		offset.Zero();
		Brush_UpdateLightPoints(b, offset);
	}
}

/*
================
Brush_SnapToGrid
================
*/
void Brush_SnapToGrid(brush_t *pb) {
	int i;
	for (face_t * f = pb->brush_faces; f; f = f->next) {
		idWinding *w = f->face_winding;

		if (!w) {
			continue;	// freed face
		}

		for (i = 0; i < w->GetNumPoints(); i++) {
			SnapVectorToGrid( (*w)[i].ToVec3() );
		}

		for (i = 0; i < 3; i++) {
			f->planepts[i].x = (*w)[i].x;
			f->planepts[i].y = (*w)[i].y;
			f->planepts[i].z = (*w)[i].z;
		}
	}
	idVec3 v;
	idStr str;
	entity_t* owner = pb->owner;

	if (owner->GetVectorForKey("origin", v)) {
		SnapVectorToGrid(pb->owner->origin);
		sprintf(str, "%i %i %i", (int)pb->owner->origin.x, (int)pb->owner->origin.y, (int)pb->owner->origin.z);
		owner->SetKeyValue("origin", str);
	}

	if (pb->owner->eclass->nShowFlags & ECLASS_LIGHT) {
		if (owner->GetVectorForKey("light_right", v)) {
			// projected
			SnapVectorToGrid(v);
			pb->lightRight = v;
			owner->SetKeyVec3("light_right", v);
			owner->GetVectorForKey("light_up", v);
			SnapVectorToGrid(v);
			pb->lightUp = v;
			owner->SetKeyVec3("light_up", v);
			owner->GetVectorForKey("light_target", v);
			SnapVectorToGrid(v);
			pb->lightTarget = v;
			owner->SetKeyVec3("light_target", v);
			if (owner->GetVectorForKey("light_start", v)) {
				SnapVectorToGrid(v);
				pb->lightStart = v;
				owner->SetKeyVec3("light_start", v);
				owner->GetVectorForKey("light_end", v);
				SnapVectorToGrid(v);
				pb->lightEnd = v;
				owner->SetKeyVec3("light_end", v);
			}
		} else {
			// point
			if (owner->GetVectorForKey("light_center", v)) {
				SnapVectorToGrid(v);
				owner->SetKeyVec3("light_center", v);
			}
		}
	}

	if ( pb->owner->curve ) {
		int c = pb->owner->curve->GetNumValues();
		for ( i = 0; i < c; i++ ) {
			v = pb->owner->curve->GetValue( i );
			SnapVectorToGrid( v );
			pb->owner->curve->SetValue( i, v );
		}
	}

	Brush_Build(pb);
}

/*
================
Brush_Rotate
================
*/
void Brush_Rotate(brush_t *b, idMat3 matrix, idVec3 origin, bool bBuild) {
	for (face_t * f = b->brush_faces; f; f = f->next) {
		for (int i = 0; i < 3; i++) {
			f->planepts[i] -= origin;
			f->planepts[i] *= matrix;
			f->planepts[i] += origin;
		}
	}

	if (bBuild) {
		Brush_Build(b, false, false);
	}
}

extern void VectorRotate3Origin( const idVec3 &vIn, const idVec3 &vRotation, const idVec3 &vOrigin, idVec3 &out );

/*
================
Brush_Rotate
================
*/
void Brush_Rotate(brush_t *b, idVec3 vAngle, idVec3 vOrigin, bool bBuild) {
	for (face_t * f = b->brush_faces; f; f = f->next) {
		for (int i = 0; i < 3; i++) {
			VectorRotate3Origin(f->planepts[i], vAngle, vOrigin, f->planepts[i]);
		}
	}

	if (bBuild) {
		Brush_Build(b, false, false);
	}
}

/*
================
Brush_Resize

  the brush must be a true axial box
================
*/
void Brush_Resize( brush_t *b, idVec3 vMin, idVec3 vMax ) {
	int i, j;
	face_t *f;

	assert( vMin[0] < vMax[0] && vMin[1] < vMax[1] && vMin[2] < vMax[2] );

	Brush_MakeFacePlanes( b );

	for( f = b->brush_faces; f; f = f->next ) {
		for ( i = 0; i < 3; i++ ) {
			if ( f->plane.Normal()[i] >= 0.999f ) {
				for ( j = 0; j < 3; j++ ) {
					f->planepts[j][i] = vMax[i];
				}
				break;
			}
			if ( f->plane.Normal()[i] <= -0.999f ) {
				for ( j = 0; j < 3; j++ ) {
					f->planepts[j][i] = vMin[i];
				}
				break;
			}
		}
		//assert( i < 3 );
	}

	Brush_Build( b, true );
}

/*
================
Entity_GetRotationMatrixAngles
================
*/
static bool Entity_GetRotationMatrixAngles( entity_t *e, idMat3 &mat, idAngles &angles ) {
	assert(e);

	int angle;

	/* the angle keyword is a yaw value, except for two special markers */
	if ( e->GetMatrixForKey( "rotation", mat ) ) {
		angles = mat.ToAngles();
		return true;
	}
	else if ( e->epairs.GetInt( "angle", "0", angle ) ) {
		if ( angle == -1 ) {		// up
			angles.Set( 270, 0, 0 );
		}
		else if ( angle == -2 ) {	// down
			angles.Set( 90, 0, 0 );
		}
		else {
			angles.Set( 0, angle, 0 );
		}
		mat = angles.ToMat3();
		return true;
	}
	else {
		mat.Identity();
		angles.Zero();
		return false;
	}
}

/*
================
FacingVectors
================
*/
static void FacingVectors(entity_t *e, idVec3 &forward, idVec3 &right, idVec3 &up) {
	idAngles	angles;
	idMat3		mat;

	Entity_GetRotationMatrixAngles(e, mat, angles);
	angles.ToVectors( &forward, &right, &up);
}

/*
================
Brush_DrawFacingAngle
================
*/
static void Brush_DrawFacingAngle( const brush_t *b, entity_t *e, bool particle, const idVec4& color ) {
	idVec3	forward, right, up;
	idVec3	endpoint, tip1, tip2;
	idVec3	start = (e->brushes.onext->mins + e->brushes.onext->maxs) * 0.5f;
	float dist = (b->maxs[0] - start[0]) * 2.5f;

	FacingVectors(e, forward, right, up);
	VectorMA(start, dist, ( particle ) ? up : forward, endpoint);

	dist = (b->maxs[0] - start[0]) * 0.5f;
	VectorMA(endpoint, -dist, ( particle ) ? up : forward, tip1);
	VectorMA(tip1, -dist, ( particle ) ? forward : up, tip1);
	VectorMA(tip1, 2 * dist, ( particle ) ? forward : up, tip2);
	globalImages->BindNull();

	//TODO(johl): linewidth should be 2, but linewidth>1 is deprecated. WTF?
    glLineWidth(1);
    fhImmediateMode im;
	im.Color4fv(color.ToFloatPtr());
	im.Begin(GL_LINES);
	im.Vertex3fv(start.ToFloatPtr());
	im.Vertex3fv(endpoint.ToFloatPtr());
	im.Vertex3fv(endpoint.ToFloatPtr());
	im.Vertex3fv(tip1.ToFloatPtr());
	im.Vertex3fv(endpoint.ToFloatPtr());
	im.Vertex3fv(tip2.ToFloatPtr());
	im.End();
}

/*
================
DrawProjectedLight
================
*/
static void DrawProjectedLight(const brush_t *b, bool bSelected, bool texture) {
	idVec3	v1, v2, cross, vieworg, edge[8][2], v[4];
	idVec3	target, start;

	if (!bSelected && !g_bShowLightVolumes) {
		return;
	}

	// use the renderer to get the volume outline
	idPlane		lightProject[4];
	idPlane		planes[6];
	srfTriangles_t	*tri;

	// use the game's epair parsing code so
	// we can use the same renderLight generation
	entity_t *ent = b->owner;
	idDict	spawnArgs;
	renderLight_t	parms;

	spawnArgs = ent->epairs;
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &parms );
	R_RenderLightFrustum( parms, planes );

	tri = R_PolytopeSurface(6, planes, NULL);

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDisable( GL_CULL_FACE );
	fhImmediateMode im;
	im.Color3f( 1, 0, 1 );
	im.Begin( GL_TRIANGLES );
	for (int i = 0; i < tri->numIndexes; i += 3) {
		im.Vertex3fv( tri->verts[tri->indexes[i]].xyz.ToFloatPtr() );
		im.Vertex3fv( tri->verts[tri->indexes[i + 1]].xyz.ToFloatPtr() );
		im.Vertex3fv( tri->verts[tri->indexes[i + 2]].xyz.ToFloatPtr() );
	}
	im.End();
	glEnable( GL_CULL_FACE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	R_FreeStaticTriSurf(tri);

	// draw different selection points for point lights or projected
	// lights (FIXME: rotate these based on parms!)
	if ( !bSelected ) {
		return;
	}

	idMat3 mat;
	bool transform = b->owner->GetMatrixForKey("light_rotation", mat);
	if (!transform) {
		transform = b->owner->GetMatrixForKey("rotation", mat);
	}
	idVec3 tv;
	idVec3 *origin = (b->trackLightOrigin) ? &b->owner->lightOrigin : &b->owner->origin;


	glPointSize(8);
	im.Begin(GL_POINTS);
	im.Color3f(1.0f, 0.4f, 0.8f);

	if (b->pointLight) {
		// point light
		if (b->lightCenter[0] || b->lightCenter[1] || b->lightCenter[2]) {
			tv = b->lightCenter;
			if (transform) {
			tv -= *origin;
			tv *= mat;
			tv += *origin;
			}
			im.Vertex3fv(tv.ToFloatPtr());
		}
	} else {
		// projected light
		tv = b->lightRight;
		if (transform) {
			tv -= *origin;
			tv *= mat;
			tv += *origin;
		}
		im.Vertex3fv(tv.ToFloatPtr());
		tv = b->lightTarget;
		if (transform) {
			tv -= *origin;
			tv *= mat;
			tv += *origin;
		}
		im.Vertex3fv(tv.ToFloatPtr());
		tv = b->lightUp;
		if (transform) {
			tv -= *origin;
			tv *= mat;
			tv += *origin;
		}
		im.Vertex3fv(tv.ToFloatPtr());

		if (b->startEnd) {
			im.Color3f(0.4f, 1.0f, 0.8f);
			im.Vertex3fv(b->lightStart.ToFloatPtr());
			im.Vertex3fv(b->lightEnd.ToFloatPtr());
		}
	}

	im.End();
	glPointSize(1);
}

/*
================
GLCircle
================
*/
static void GLCircle(float x, float y, float z, float r, const idVec4& color)
{
	float ix = 0;
	float iy = r;
	float ig = 3 - 2 * r;
	float idgr = -6;
	float idgd = 4 * r - 10;
	glPointSize(0.5f);
	fhImmediateMode im;
	im.Color4fv(color.ToFloatPtr());
	im.Begin(GL_POINTS);
	while (ix <= iy) {
		if (ig < 0) {
			ig += idgd;
			idgd -= 8;
			iy--;
		} else {
			ig += idgr;
			idgd -= 4;
		}
		idgr -= 4;
		ix++;
		im.Vertex3f(x + ix, y + iy, z);
		im.Vertex3f(x - ix, y + iy, z);
		im.Vertex3f(x + ix, y - iy, z);
		im.Vertex3f(x - ix, y - iy, z);
		im.Vertex3f(x + iy, y + ix, z);
		im.Vertex3f(x - iy, y + ix, z);
		im.Vertex3f(x + iy, y - ix, z);
		im.Vertex3f(x - iy, y - ix, z);
	}
	im.End();
}

/*
================
DrawSpeaker
================
*/
static void DrawSpeaker(const brush_t *b, bool bSelected, bool twoD) {

	if (!(g_qeglobals.d_savedinfo.showSoundAlways || (g_qeglobals.d_savedinfo.showSoundWhenSelected && bSelected))) {
		return;
	}

	// convert to units ( inches )
	float min = b->owner->FloatForKey("s_mindistance");
	float max = b->owner->FloatForKey("s_maxdistance");

	const char *s = b->owner->epairs.GetString("s_shader");
	if (s && *s) {
		const idSoundShader *shader = declManager->FindSound( s, false );
		if ( shader ) {
			if ( !min ) {
				min = shader->GetMinDistance();
			}
			if ( !max ) {
				max = shader->GetMaxDistance();
			}
		}
	}

	if (min == 0 && max == 0) {
		return;
	}


	// convert from meters to doom units
	min *= METERS_TO_DOOM;
	max *= METERS_TO_DOOM;

	idVec4 minColor;
	idVec4 maxColor;

	if (bSelected) {
		minColor.Set(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES].x, g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES].y, g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES].z, .5);
	}
	else {
		minColor.Set(b->owner->eclass->color.x, b->owner->eclass->color.y, b->owner->eclass->color.z, .5);
	}

	maxColor.Set(minColor.x, minColor.y, minColor.z, 1.0f);

	if (twoD) {
		GLCircle(b->owner->origin.x, b->owner->origin.y, b->owner->origin.z, min, minColor);
		GLCircle(b->owner->origin.x, b->owner->origin.y, b->owner->origin.z, max, maxColor);
	} else {
		GL_ModelViewMatrix.Push();
		GL_ModelViewMatrix.Translate( b->owner->origin.x, b->owner->origin.y, b->owner->origin.z );

		minColor.w = 0.35f;
		maxColor.w = 0.1f;

		const int tesselation = 16;

		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		fhImmediateMode im;
		im.Color3f( 0.4f, 0.4f, 0.4f );
		im.Sphere( min, tesselation, tesselation, true );
		im.Color3f( 0.8f, 0.8f, 0.8f );
		im.Sphere( max, tesselation, tesselation, true );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		globalImages->BindNull();

		im.Color4fv( minColor.ToFloatPtr() );
		im.Sphere( min, tesselation, tesselation, true );
		im.Color4fv( maxColor.ToFloatPtr() );
		im.Sphere( max, tesselation, tesselation, true );

		glDisable( GL_BLEND );

		GL_ModelViewMatrix.Pop();
	}
}

/*
================
DrawLight
================
*/
static void DrawLight(const brush_t *b, bool bSelected) {
	idVec3	vTriColor;
	if (!b->owner->GetVectorForKey("_color", vTriColor)) {
		vTriColor = idVec3(1, 1, 1);
	}

	idVec3	vCorners[4];
	float	fMid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

	vCorners[0][0] = b->mins[0];
	vCorners[0][1] = b->mins[1];
	vCorners[0][2] = fMid;

	vCorners[1][0] = b->mins[0];
	vCorners[1][1] = b->maxs[1];
	vCorners[1][2] = fMid;

	vCorners[2][0] = b->maxs[0];
	vCorners[2][1] = b->maxs[1];
	vCorners[2][2] = fMid;

	vCorners[3][0] = b->maxs[0];
	vCorners[3][1] = b->mins[1];
	vCorners[3][2] = fMid;

	idVec3	vTop;
	vTop[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
	vTop[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
	vTop[2] = b->maxs[2];

	idVec3 vBottom = vTop;
	vBottom[2] = b->mins[2];

	fhTrisBuffer* trisBuffer = g_qeglobals.surfaceBuffer.GetColorBuffer();

	idVec4 shadedColor = idVec4(vTriColor,1);
	for (int i = 0; i <= 3; i++) {
		trisBuffer->Add(vTop, vCorners[i], vCorners[(i<3) ? i + 1 : 0], shadedColor);
		shadedColor *= 0.95f;
	}

	shadedColor = idVec4(vTriColor,1);
	for (int i = 3; i >= 0; i--) {
		trisBuffer->Add(vBottom, vCorners[i], vCorners[(i>0) ? i - 1 : 3], shadedColor);
		shadedColor *= 0.95f;
	}

	DrawProjectedLight(b, bSelected, true);
}

editorModel_t Brush_GetEditorModel(const brush_t* b) {

  editorModel_t ret;
  ret.drawBounds = false;
  ret.model = b->modelHandle;

  if (ret.model == NULL && b->owner && b->owner->eclass) {
    ret.model = b->owner->eclass->entityModel;
  }
  if (ret.model == NULL) {
    ret.model = renderModelManager->DefaultModel();
  }
  assert(ret.model);
  ret.bounds = ret.model->Bounds();

  if(ret.model->IsDynamicModel() != DM_STATIC) {
    ret.drawBounds = true;

    if (dynamic_cast<idRenderModelMD5 *>(ret.model)) {
      if (!b->animSnapshotModel) {
        const char *classname = b->owner->ValueForKey("classname");
        if (stricmp(classname, "func_static") == 0) {
          classname = b->owner->ValueForKey("animclass");
        }
        const char *anim = b->owner->ValueForKey("anim");
        int frame = b->owner->IntForKey("frame") + 1;
        if (frame < 1) {
          frame = 1;
        }
        if (!anim || !anim[0]) {
          anim = "idle";
        }
        //common->Printf("instantiate dynamic model '%s'\n", classname);
        b->animSnapshotModel = gameEdit->ANIM_CreateMeshForAnim(ret.model, classname, anim, frame, false);
        if(!b->animSnapshotModel) {
          common->Warning("Unknown animation '%s' for classname '%s' (entity %s)", anim, classname, b->owner->ValueForKey("name"));

          idStr stubName = classname;
          stubName.Append("_");
          stubName.Append(anim);

          idRenderModelStatic *stub = new idRenderModelStatic;
          stub->InitEmpty(stubName);
          stub->MakeDefaultModel();
          stub->SetLevelLoadReferenced(true);
          b->animSnapshotModel = stub;
        }
      }

      assert(ret.model);
      ret.model = b->animSnapshotModel;
      ret.drawBounds = false;
    }
    else if (dynamic_cast<idRenderModelPrt*>(ret.model) || dynamic_cast<idRenderModelLiquid*>(ret.model)) {
      ret.bounds.Zero();
      ret.bounds.ExpandSelf(12.0f);
    }
  }

  assert(ret.model);
  return ret;
}

/*
================
Brush_DrawModel
================
*/
static void Brush_DrawModel( const brush_t *b, bool camera, bool bSelected ) {
	const auto nDrawMode = g_pParentWnd->GetCamera().GetDrawMode();

	if ( camera && g_PrefsDlg.m_nEntityShowState != ENTITY_WIREFRAME && nDrawMode != CameraDrawMode::Wireframe ) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
	else {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}

	editorModel_t editorModel = Brush_GetEditorModel(b);
	assert(editorModel.model);

	const idVec3 color = bSelected ? g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES] : b->owner->eclass->color;

	if(editorModel.drawBounds) {
	idVec3 center = editorModel.bounds.GetCenter();
		glBox(idVec4(color.x, color.y, color.z, 1.0f), b->owner->origin + center, editorModel.bounds.GetRadius(center));
	}

	idMat3 axis;
	idAngles angles;
	Entity_GetRotationMatrixAngles( b->owner, axis, angles );
	DrawRenderModel(editorModel.model, b->owner->origin, axis, camera, color);

	if (bSelected && camera) {
		//draw white triangle outlines
		globalImages->BindNull();

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glPolygonOffset(1.0f, 3.0f);
		DrawRenderModel(editorModel.model, b->owner->origin, axis, false, idVec3(1, 1, 1));
		glEnable(GL_DEPTH_TEST);
	}

	if ( bSelected && camera ) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
	else if ( camera ) {
		globalImages->BindNull();
	}

	if ( g_bPatchShowBounds ) {
		fhImmediateMode im;
		im.Begin(GL_LINES);

		for ( face_t *face = b->brush_faces; face; face = face->next ) {
			// only draw polygons facing in a direction we care about
			idWinding *w = face->face_winding;
			if (!w) {
				continue;
			}

			//
			// if (b->alphaBrush && !(face->texdef.flags & SURF_ALPHA)) continue;
			// draw the polygon
			//
			for (int i = 0; i < (w->GetNumPoints() - 1); i++) {
				im.Vertex3fv( (*w)[i].ToFloatPtr() );
				im.Vertex3fv( (*w)[i+1].ToFloatPtr() );
			}
			im.Vertex3fv((*w)[w->GetNumPoints() - 1].ToFloatPtr());
			im.Vertex3fv((*w)[0].ToFloatPtr());
		}

	    im.End();
	}
}

/*
================
GLTransformedVertex
================
*/
static void GLTransformedVertex(float x, float y, float z, idMat3 mat, idVec3 origin, idVec3 color, float maxDist) {
	idVec3 v(x,y,z);
	v -= origin;
	v *= mat;
	v += origin;

	idVec3 n = v - g_pParentWnd->GetCamera().GetOrigin();
	float max = n.Length() / maxDist;
	if (color.x) {
		color.x = max;
	} else if (color.y) {
		color.y = max;
	} else {
		color.z = max;
	}
	glColor3f(color.x, color.y, color.z);
	glVertex3f(v.x, v.y, v.z);

}

/*
================
GLTransformedCircle
================
*/
static void GLTransformedCircle(int type, idVec3 origin, float r, idMat3 mat, float pointSize, idVec3 color, float maxDist) {
	glPointSize(pointSize);
	glBegin(GL_POINTS);
	for (int i = 0; i < 360; i++) {
		float cx = origin.x;
		float cy = origin.y;
		float cz = origin.z;
		switch (type) {
			case 0:
				cx += r * cos((float)i);
				cy += r * sin((float)i);
				break;
			case 1:
				cx += r * cos((float)i);
				cz += r * sin((float)i);
				break;
			case 2:
				cy += r * sin((float)i);
				cz += r * cos((float)i);
				break;
			default:
				break;
		}
		GLTransformedVertex(cx, cy, cz, mat, origin, color, maxDist);
	}
	glEnd();
}

/*
================
Brush_DrawAxis
================
*/
static void Brush_DrawAxis(const brush_t *b) {
	if ( g_pParentWnd->ActiveXY()->RotateMode() && b->modelHandle ) {
		bool matrix = false;
		idMat3 mat;
		float a, s, c;
		if (b->owner->GetMatrixForKey("rotation", mat)) {
			matrix = true;
		} else {
			a = b->owner->FloatForKey("angle");
			if (a) {
				s = sin( DEG2RAD( a ) );
				c = cos( DEG2RAD( a ) );
			}
			else {
				s = c = 0;
			}
		}

		idBounds bo;
		bo.FromTransformedBounds(b->modelHandle->Bounds(), b->owner->origin, b->owner->rotation);

		const auto cameraOrigin = g_pParentWnd->GetCamera().GetOrigin();

		float dist = (cameraOrigin - bo[0]).Length();
		float dist2 = (cameraOrigin - bo[1]).Length();
		if (dist2 > dist) {
			dist = dist2;
		}

		float xr, yr, zr;
		xr = (b->modelHandle->Bounds()[1].x > b->modelHandle->Bounds()[0].x) ? b->modelHandle->Bounds()[1].x - b->modelHandle->Bounds()[0].x : b->modelHandle->Bounds()[0].x - b->modelHandle->Bounds()[1].x;
		yr = (b->modelHandle->Bounds()[1].y > b->modelHandle->Bounds()[0].y) ? b->modelHandle->Bounds()[1].y - b->modelHandle->Bounds()[0].y : b->modelHandle->Bounds()[0].y - b->modelHandle->Bounds()[1].y;
		zr = (b->modelHandle->Bounds()[1].z > b->modelHandle->Bounds()[0].z) ? b->modelHandle->Bounds()[1].z - b->modelHandle->Bounds()[0].z : b->modelHandle->Bounds()[0].z - b->modelHandle->Bounds()[1].z;

		globalImages->BindNull();

		GLTransformedCircle(0, b->owner->origin, xr, mat, 1.25, idVec3(0, 0, 1), dist);
		GLTransformedCircle(1, b->owner->origin, yr, mat, 1.25, idVec3(0, 1, 0), dist);
		GLTransformedCircle(2, b->owner->origin, zr, mat, 1.25, idVec3(1, 0, 0), dist);

		float wr = xr;
		int type = 0;
		idVec3 org = b->owner->origin;
		if (g_qeglobals.rotateAxis == 0) {
			wr = zr;
			type = 2;
		} else if (g_qeglobals.rotateAxis == 1) {
			wr = yr;
			type = 1;
		}

		if (g_qeglobals.flatRotation) {
			if (yr > wr) {
				wr = yr;
			}
			if (zr > wr) {
				wr = zr;
			}
			idVec3 vec = vec3_origin;
			vec[g_qeglobals.rotateAxis] = 1.0f;
			if (g_qeglobals.flatRotation == 1) {
				org = g_pParentWnd->ActiveXY()->RotateOrigin();
				float t = (org - bo.GetCenter()).Length();
				if (t > wr) {
					wr = t;
				}
			} else {
				org = bo.GetCenter();
			}
			idRotation rot(org, vec, 0);
			mat = rot.ToMat3();
		}
		GLTransformedCircle(type, org, wr * 1.03f, mat, 1.45f, idVec3(1, 1, 1), dist);
	}
}

/*
================
Brush_DrawModelInfo
================
*/
static void Brush_DrawModelInfo(const brush_t *b, bool selected) {
	if (b->modelHandle > 0) {
		Brush_DrawModel(b, true, selected);

		if ( selected ) {
			Brush_DrawAxis(b);
		}
		return;
	}
}

/*
================
Brush_DrawEmitter
================
*/
static void Brush_DrawEmitter(const brush_t *b, bool bSelected, bool cam) {
	if ( !( b->owner->eclass->nShowFlags & ECLASS_PARTICLE ) ) {
		return;
	}

	idVec4 color = idVec4(b->owner->eclass->color, 0.5f);
	if(bSelected) {
		color = idVec4(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES], 0.5);
	}

	if ( cam ) {
		Brush_DrawFacingAngle( b, b->owner, true, color );
	}
}

/*
================
Brush_DrawEnv
================
*/
static void Brush_DrawEnv( const brush_t *b, bool cameraView, bool bSelected ) {
	idVec3 origin, newOrigin;
	idMat3 axis, newAxis;
	idAngles newAngles;
	bool poseIsSet;

	idRenderModel *model = gameEdit->AF_CreateMesh( b->owner->epairs, origin, axis, poseIsSet );

	if ( !poseIsSet ) {
		if ( Entity_GetRotationMatrixAngles( b->owner, newAxis, newAngles ) ) {
			axis = newAxis;
		}
		if ( b->owner->epairs.GetVector( "origin", "0 0 0", newOrigin ) ) {
			origin = newOrigin;
		}
	}

	if ( model ) {
		if ( cameraView && g_PrefsDlg.m_nEntityShowState != ENTITY_WIREFRAME ) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		else {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}

		DrawRenderModel(model, origin, axis, true, bSelected ? g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES] : idVec3(1,1,1));

		globalImages->BindNull();
		delete model;
		model = NULL;
	}
}

/*
================
Brush_DrawCombatNode
================
*/
static void Brush_DrawCombatNode( const brush_t *b, bool cameraView, bool bSelected ) {
	float min_dist = b->owner->epairs.GetFloat( "min" );
	float max_dist = b->owner->epairs.GetFloat( "max" );
	float fov = b->owner->epairs.GetFloat( "fov", "60" );
	float yaw = b->owner->epairs.GetFloat("angle");
	idVec3 offset = b->owner->epairs.GetVector("offset");

	idAngles leftang( 0.0f, yaw + fov * 0.5f - 90.0f, 0.0f );
	idVec3 cone_left = leftang.ToForward();
	idAngles rightang( 0.0f, yaw - fov * 0.5f + 90.0f, 0.0f );
	idVec3 cone_right = rightang.ToForward();
	bool disabled = b->owner->epairs.GetBool( "start_off" );

	idVec4 color;
	if ( bSelected ) {
		color = colorRed;
	} else  {
		color = colorBlue;
	}

	idVec3 leftDir( -cone_left.y, cone_left.x, 0.0f );
	idVec3 rightDir( cone_right.y, -cone_right.x, 0.0f );
	leftDir.NormalizeFast();
	rightDir.NormalizeFast();

	idMat3 axis = idAngles(0, yaw, 0).ToMat3();
	idVec3 org = b->owner->origin + offset;
	idVec3 entorg = b->owner->origin;
	float cone_dot = cone_right * axis[ 1 ];
	if ( idMath::Fabs( cone_dot ) > 0.1 ) {
		idVec3 pt, pt1, pt2, pt3, pt4;
		float cone_dist = max_dist / cone_dot;
		pt1 = org + leftDir * min_dist;
		pt2 = org + leftDir * cone_dist;
		pt3 = org + rightDir * cone_dist;
		pt4 = org + rightDir * min_dist;
		glColor4fv(color.ToFloatPtr());
		glBegin(GL_LINE_STRIP);
		glVertex3fv( pt1.ToFloatPtr());
		glVertex3fv( pt2.ToFloatPtr());
		glVertex3fv( pt3.ToFloatPtr());
		glVertex3fv( pt4.ToFloatPtr());
		glVertex3fv( pt1.ToFloatPtr());
		glEnd();

		glColor4fv(colorGreen.ToFloatPtr());
		glBegin(GL_LINE_STRIP);
		glVertex3fv( entorg.ToFloatPtr());
		pt = (pt1 + pt4) * 0.5f;
		glVertex3fv( pt.ToFloatPtr());
		pt = (pt2 + pt3) * 0.5f;
		glVertex3fv( pt.ToFloatPtr());
		idVec3 tip = pt;
		idVec3 dir = ((pt1 + pt2) * 0.5f) - tip;
		dir.Normalize();
		pt = tip + dir * 15.0f;
		glVertex3fv( pt.ToFloatPtr());
		glVertex3fv( tip.ToFloatPtr());
		dir = ((pt4 + pt3) * 0.5f) - tip;
		dir.Normalize();
		pt = tip + dir * 15.0f;
		glVertex3fv( pt.ToFloatPtr());
		glEnd();
	}

}

/*
================
Brush_Draw
================
*/
void Brush_Draw(const brush_t *b, bool bSelected) {
	face_t		*face;
	int			i, order;
	idWinding	*w;
	bool model = false;

	//
	// (TTimo) NOTE: added by build 173, I check after pPlugEnt so it doesn't
	// interfere ?
	//
	if ( b->hiddenBrush ) {
		return;
	}

	Brush_DrawCurve( b, bSelected, true );

	if (b->pPatch) {
		Patch_DrawCam(b->pPatch, bSelected);
		return;
	}

	

	if (!(g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES) && (b->owner->eclass->nShowFlags & ECLASS_ANGLE)) {
		Brush_DrawFacingAngle(b, b->owner, false, idVec4(1,1,1,1));
	}

	if ( b->owner->eclass->fixedsize ) {

		DrawSpeaker( b, bSelected, false );

		if ( g_PrefsDlg.m_bNewLightDraw && (b->owner->eclass->nShowFlags & ECLASS_LIGHT) && !(b->modelHandle || b->entityModel) ) {
 			DrawLight( b, bSelected );
			return;
		}

		if ( b->owner->eclass->nShowFlags & ECLASS_ENV ) {
			Brush_DrawEnv( b, true, bSelected );
		}

		if ( b->owner->eclass->nShowFlags & ECLASS_COMBATNODE ) {
			Brush_DrawCombatNode( b, true, bSelected );
		}
	}


	if (!(b->owner && (b->owner->eclass->nShowFlags & ECLASS_WORLDSPAWN))) {
		g_qeglobals.pointBuffer.Add(b->owner->origin, idVec4(1,0,0,1), 4);
	}

	if ( b->owner->eclass->entityModel ) {
		//glColor3fv( b->owner->eclass->color.ToFloatPtr() );
		Brush_DrawModel( b, true, bSelected );
		return;
	}

	Brush_DrawEmitter( b, bSelected, true );

	if ( b->modelHandle > 0 && !model ) {
		Brush_DrawModelInfo( b, bSelected );
		return;
	}

	// guarantee the texture will be set first
	for (face = b->brush_faces, order = 0; face; face = face->next, order++) {
		w = face->face_winding;
		if (!w) {
			continue;	// freed face
		}

		if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_CAULK) {
			if (strstr(face->texdef.name, "caulk")) {
				continue;
			}
		}

		if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_VISPORTALS) {
			if (strstr(face->texdef.name, "visportal")) {
				continue;
			}
		}

		if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_NODRAW) {
			if (strstr(face->texdef.name, "nodraw")) {
				continue;
			}
		}
#if 0
    fhImmediateMode im;

		if ( (nDrawMode == cd_texture || nDrawMode == cd_light) && !b->forceWireFrame && face->d_texture ) {
      im.SetTexture(face->d_texture->GetEditorImage());
		}

		if (model) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			im.Color4f( face->d_color.x, face->d_color.y, face->d_color.z, 0.1f );
		} else {
			im.Color4f( face->d_color.x, face->d_color.y, face->d_color.z, face->d_texture->GetEditorAlpha() );
		}

		im.Begin(GL_POLYGON);

		for (i = 0; i < w->GetNumPoints(); i++) {
			if ( !b->forceWireFrame && ( nDrawMode == cd_texture || nDrawMode == cd_light ) ) {
				im.TexCoord2fv( &(*w)[i][3] );
			}

			im.Vertex3fv( (*w)[i].ToFloatPtr() );
		}

		im.End();

		if (model) {
			glDisable(GL_BLEND);
		}
#else
	const auto nDrawMode = g_pParentWnd->GetCamera().GetDrawMode();
    const idMaterial* material = nullptr;
    if (nDrawMode == CameraDrawMode::Textures && !b->forceWireFrame && face->d_texture) {
      material = face->d_texture;
    }

    fhTrisBuffer* trisBuffer = g_qeglobals.surfaceBuffer.GetMaterialBuffer(material);

    fhSimpleVert firstVert;
    fhSimpleVert prevVert;
    fhSimpleVert currentVert;

    currentVert.SetColor(idVec4(face->d_color, face->d_texture->GetEditorAlpha()));

    for (i = 0; i < w->GetNumPoints(); i++) {
      const idVec5& v = (*w)[i];

      currentVert.xyz.x = v[0];
      currentVert.xyz.y = v[1];
      currentVert.xyz.z = v[2];
      currentVert.st.x = v[3];
      currentVert.st.y = v[4];

      if(i == 0) {
        firstVert = currentVert;
      }

      if(i > 1) {
        trisBuffer->Add(firstVert, prevVert, currentVert);
      }

      prevVert = currentVert;
    }
#endif


	}

	globalImages->BindNull();
}

/*
================
Face_Draw
================
*/
void Face_Draw( const face_t *f, const idVec4& color) {
  if (f->face_winding == NULL ||  f->face_winding->GetNumPoints() < 3) {
		return;
	}

  fhTrisBuffer* buffer = g_qeglobals.surfaceBuffer.GetColorBuffer();

  for (int i = 2; i<f->face_winding->GetNumPoints(); ++i) {
    buffer->Add((*f->face_winding)[0].ToVec3(), (*f->face_winding)[i-1].ToVec3(), (*f->face_winding)[i].ToVec3(), color);
  }
}

/*
================
Face_DrawOutline
================
*/
void Face_DrawOutline( const face_t *f, const idVec3& color) {
  if (f->face_winding == NULL || f->face_winding->GetNumPoints() < 2) {
    return;
  }

  int numPoints = f->face_winding->GetNumPoints();
  const idVec5* points = &(*f->face_winding)[0];

  for (int i=0; (i+1) < numPoints; i++) {
    g_qeglobals.lineBuffer.Add(points[i].ToVec3(), points[i+1].ToVec3(), color);
  }

  g_qeglobals.lineBuffer.Add(points[0].ToVec3(), points[numPoints-1].ToVec3(), color);
}

/*
================
Brush_DrawCurve
================
*/
void Brush_DrawCurve( const brush_t *b, bool bSelected, bool cam ) {
	if ( b == NULL || b->owner->curve == NULL ) {
		return;
	}

	int maxage = b->owner->curve->GetNumValues();
	int i, time = 0;
	for ( i = 0; i < maxage; i++) {

		if ( bSelected && g_qeglobals.d_select_mode == sel_editpoint ) {
			idVec3 v = b->owner->curve->GetValue( i );
			if ( cam ) {
				glBox( colorBlue, v, 6.0f );
				if ( PointInMoveList( b->owner->curve->GetValueAddress( i ) ) >= 0 ) {
					glBox(colorBlue, v, 8.0f );
				}
			} else {
        glPointSize( 4.0f );
        fhImmediateMode im;
        im.Color4fv( colorBlue.ToFloatPtr() );
				im.Begin( GL_POINTS );
				im.Vertex3f( v.x, v.y, v.z );
				im.End();

				if ( PointInMoveList( b->owner->curve->GetValueAddress( i ) ) >= 0 ) {
					glBox(colorBlue, v, 4.0f );
				}
			}
		}

    // DHM - _D3XP : Makes it easier to see curve
		if ( i + 1  < maxage ) {
      fhImmediateMode im;
      im.Begin(GL_LINE_STRIP);

			int start = b->owner->curve->GetTime( i );
			int end = b->owner->curve->GetTime( i + 1 );
			int inc = (end - start) / POINTS_PER_KNOT;
			for ( int j = 0; j <= POINTS_PER_KNOT; j++ ) {
				idVec3 v = b->owner->curve->GetCurrentValue( start );
				im.Vertex3f( v.x, v.y, v.z );
				start += inc;
			}

      im.End();
		}

	}
	glPointSize(1);
}

/*
================
Brush_DrawXY
================
*/
void Brush_DrawXY(brush_t *b, ViewType nViewType, bool bSelected, const idVec3& color) {
	if ( b->hiddenBrush ) {
		return;
	}

	if (!(b->owner && (b->owner->eclass->nShowFlags & ECLASS_WORLDSPAWN))) {
    glPointSize(4);
    fhImmediateMode im;
		im.Color4f( 1.0f, 0.0f, 0.0f, 0.8f );
		im.Begin(GL_POINTS);
		im.Vertex3fv(b->owner->origin.ToFloatPtr());
		im.End();
	}

	Brush_DrawCurve( b, bSelected, false );

	if (b->pPatch) {
		Patch_DrawXY(b->pPatch);
		if (!g_bPatchShowBounds) {
			return;
		}
	}

	if (b->owner->eclass->fixedsize) {

 		DrawSpeaker(b, bSelected, true);
		if (g_PrefsDlg.m_bNewLightDraw && (b->owner->eclass->nShowFlags & ECLASS_LIGHT) && !(b->modelHandle || b->entityModel)) {
			idVec3	vCorners[4];
			float	fMid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

			vCorners[0][0] = b->mins[0];
			vCorners[0][1] = b->mins[1];
			vCorners[0][2] = fMid;

			vCorners[1][0] = b->mins[0];
			vCorners[1][1] = b->maxs[1];
			vCorners[1][2] = fMid;

			vCorners[2][0] = b->maxs[0];
			vCorners[2][1] = b->maxs[1];
			vCorners[2][2] = fMid;

			vCorners[3][0] = b->maxs[0];
			vCorners[3][1] = b->mins[1];
			vCorners[3][2] = fMid;

			idVec3 vTop;
			vTop[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
			vTop[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
			vTop[2] = b->maxs[2];

			idVec3 vBottom = vTop;
			vBottom[2] = b->mins[2];

      g_qeglobals.lineBuffer.Add(vTop, vCorners[0], color);
      g_qeglobals.lineBuffer.Add(vTop, vCorners[1], color);
      g_qeglobals.lineBuffer.Add(vTop, vCorners[2], color);
      g_qeglobals.lineBuffer.Add(vTop, vCorners[3], color);
      g_qeglobals.lineBuffer.Add(vBottom, vCorners[0], color);
      g_qeglobals.lineBuffer.Add(vBottom, vCorners[1], color);
      g_qeglobals.lineBuffer.Add(vBottom, vCorners[2], color);
      g_qeglobals.lineBuffer.Add(vBottom, vCorners[3], color);
      g_qeglobals.lineBuffer.Add(vCorners[0], vCorners[1], color);
      g_qeglobals.lineBuffer.Add(vCorners[1], vCorners[2], color);
      g_qeglobals.lineBuffer.Add(vCorners[2], vCorners[3], color);
      g_qeglobals.lineBuffer.Add(vCorners[3], vCorners[0], color);

			DrawBrushEntityName(b, color);
			DrawProjectedLight(b, bSelected, false);
			return;
		} else if (b->owner->eclass->nShowFlags & ECLASS_MISCMODEL) {
			// if (PaintedModel(b, false)) return;
		} else if (b->owner->eclass->nShowFlags & ECLASS_ENV) {
			Brush_DrawEnv( b, false, bSelected );
		} else if (b->owner->eclass->nShowFlags & ECLASS_COMBATNODE) {
			Brush_DrawCombatNode(b, false, bSelected);
		}

		if (b->owner->eclass->entityModel) {
			Brush_DrawModel( b, false, bSelected );
			DrawBrushEntityName(b, color);
      return;
		}

	}

	if (b->modelHandle > 0) {
		Brush_DrawEmitter( b, bSelected, false );
		Brush_DrawModel(b, false, bSelected);
		return;
	}

  const face_t* face = nullptr;
  int order;
	for (face = b->brush_faces, order = 0; face; face = face->next, order++) {
		// only draw polygons facing in a direction we care about
		if (nViewType == ViewType::XY) {
			if (face->plane[2] <= 0) {
				continue;
			}
		} else {
			if (nViewType == ViewType::XZ) {
				if (face->plane[1] <= 0) {
					continue;
				}
			} else {
				if (face->plane[0] <= 0) {
					continue;
				}
			}
		}

		const idWinding	*w = face->face_winding;
		if (!w) {
			continue;
		}

		//
		// if (b->alphaBrush && !(face->texdef.flags & SURF_ALPHA)) continue;
		// draw the polygon
		//

		for (int i = 0; i < w->GetNumPoints(); i++) {
      if (i + 1 < w->GetNumPoints())
        g_qeglobals.lineBuffer.Add( (*w)[i].ToVec3(), (*w)[i + 1].ToVec3(), color);
      else
        g_qeglobals.lineBuffer.Add( (*w)[i].ToVec3(), (*w)[0].ToVec3(), color);
		}

	}

	DrawBrushEntityName(b, color);
}

extern bool Sys_KeyDown(int key);
/*
================
Brush_Move
================
*/
void Brush_Move(brush_t *b, const idVec3 move, bool bSnap, bool updateOrigin) {
	int		i;
	face_t	*f;
	char	text[128];

	for (f = b->brush_faces; f; f = f->next) {
		idVec3	vTemp;
		VectorCopy(move, vTemp);

		if (g_PrefsDlg.m_bTextureLock) {
			Face_MoveTexture(f, vTemp);
		}

		for (i = 0; i < 3; i++) {
			VectorAdd(f->planepts[i], move, f->planepts[i]);
		}
	}

	bool controlDown = Sys_KeyDown(VK_CONTROL);
	Brush_Build(b, bSnap, true, false, !controlDown);

	if (b->pPatch) {
		Patch_Move(b->pPatch, move);
	}

	if ( b->owner->curve ) {
		b->owner->curve->Translate( move );
		b->owner->UpdateCurveData();
	}

	idVec3	temp;

	// PGM - keep the origin vector up to date on fixed size entities.
	if (b->owner->eclass->fixedsize || b->owner->HasModel() || (updateOrigin && b->owner->GetVectorForKey("origin", temp))) {
//		if (!b->entityModel) {
			bool adjustOrigin = true;
			if(b->trackLightOrigin) {
				b->owner->lightOrigin += move;
				sprintf(text, "%i %i %i", (int)b->owner->lightOrigin[0], (int)b->owner->lightOrigin[1], (int)b->owner->lightOrigin[2]);
				b->owner->SetKeyValue("light_origin", text);
				if (QE_SingleBrush(true, true)) {
					adjustOrigin = false;
				}
			}

			if (adjustOrigin && updateOrigin) {
				b->owner->origin += move;
				if (g_moveOnly) {
					sprintf(text, "%g %g %g", b->owner->origin[0], b->owner->origin[1], b->owner->origin[2]);
				} else {
					sprintf(text, "%i %i %i", (int)b->owner->origin[0], (int)b->owner->origin[1], (int)b->owner->origin[2]);
				}
				b->owner->SetKeyValue("origin", text);
			}

			// rebuild the light dragging points now that the origin has changed
			idVec3 offset;
			offset.Zero();
			if (controlDown) {
				offset.x = -move.x;
				offset.y = -move.y;
				offset.z = -move.z;
				Brush_UpdateLightPoints(b, offset);
			} else {
				offset.Zero();
				Brush_UpdateLightPoints(b, offset);
			}

		//}
		if (b->owner->eclass->nShowFlags & ECLASS_ENV) {
			const idKeyValue *arg  = b->owner->epairs.MatchPrefix( "body ", NULL );
			idStr val;
			idVec3 org;
			idAngles ang;
			while ( arg ) {
				sscanf( arg->GetValue(), "%f %f %f %f %f %f", &org.x, &org.y, &org.z, &ang.pitch, &ang.yaw, &ang.roll );
				org += move;
				val = org.ToString(8);
				val += " ";
				val += ang.ToString(8);
				b->owner->epairs.Set(arg->GetKey(), val);
				arg = b->owner->epairs.MatchPrefix( "body ", arg );
			}
		}
	}
}

/*
================
Brush_Print
================
*/
void Brush_Print(brush_t *b) {
	int nFace = 0;
	for (face_t * f = b->brush_faces; f; f = f->next) {
		common->Printf("Face %i\n", nFace++);
		common->Printf("%f %f %f\n", f->planepts[0][0], f->planepts[0][1], f->planepts[0][2]);
		common->Printf("%f %f %f\n", f->planepts[1][0], f->planepts[1][1], f->planepts[1][2]);
		common->Printf("%f %f %f\n", f->planepts[2][0], f->planepts[2][1], f->planepts[2][2]);
	}
}

/*
================
Brush_MakeSidedCone

  Makes the current brush have the given number of 2d sides and turns it into a cone
================
*/
void Brush_MakeSidedCone(int sides) {
	int			i;
	idVec3		mins, maxs;
	brush_t		*b;
	texdef_t	*texdef;
	face_t		*f;
	idVec3		mid;
	float		width;
	float		sv, cv;

	if (sides < 3) {
		Sys_Status("Bad sides number", 0);
		return;
	}

	if (!QE_SingleBrush()) {
		Sys_Status("Must have a single brush selected", 0);
		return;
	}

	b = selected_brushes.next;
	VectorCopy(b->mins, mins);
	VectorCopy(b->maxs, maxs);
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free(b);

	// find center of brush
	width = 8;
	for (i = 0; i < 2; i++) {
		mid[i] = (maxs[i] + mins[i]) * 0.5f;
		if (maxs[i] - mins[i] > width) {
			width = maxs[i] - mins[i];
		}
	}

	width *= 0.5f;

	b = Brush_Alloc();

	// create bottom face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[0][0] = mins[0];
	f->planepts[0][1] = mins[1];
	f->planepts[0][2] = mins[2];
	f->planepts[1][0] = maxs[0];
	f->planepts[1][1] = mins[1];
	f->planepts[1][2] = mins[2];
	f->planepts[2][0] = maxs[0];
	f->planepts[2][1] = maxs[1];
	f->planepts[2][2] = mins[2];

	for (i = 0; i < sides; i++) {
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		sv = sin(i * idMath::TWO_PI / sides);
		cv = cos(i * idMath::TWO_PI / sides);

		f->planepts[0][0] = floor( mid[0] + width * cv + 0.5f );
		f->planepts[0][1] = floor( mid[1] + width * sv + 0.5f );
		f->planepts[0][2] = mins[2];

		f->planepts[1][0] = mid[0];
		f->planepts[1][1] = mid[1];
		f->planepts[1][2] = maxs[2];

		f->planepts[2][0] = floor( f->planepts[0][0] - width * sv + 0.5f );
		f->planepts[2][1] = floor( f->planepts[0][1] + width * cv + 0.5f );
		f->planepts[2][2] = maxs[2];
	}

	Brush_AddToList(b, &selected_brushes);

	Entity_LinkBrush(world_entity, b);

	Brush_Build(b);

	Sys_UpdateWindows(W_ALL);
}

/*
================
Brush_MakeSidedSphere

  Makes the current brushhave the given number of 2d sides and turns it into a sphere
================
*/
void Brush_MakeSidedSphere(int sides) {
	int			i, j;
	idVec3		mins, maxs;
	brush_t		*b;
	texdef_t	*texdef;
	face_t		*f;
	idVec3		mid;
	float		radius;

	if (sides < 4) {
		Sys_Status("Bad sides number", 0);
		return;
	}

	if (!QE_SingleBrush()) {
		Sys_Status("Must have a single brush selected", 0);
		return;
	}

	b = selected_brushes.next;
	mins = b->mins;
	maxs = b->maxs;
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free(b);

	// find center of brush
	radius = 8;
	for ( i = 0; i < 3; i++ ) {
		mid[i] = (maxs[i] + mins[i]) * 0.5f;
		if (maxs[i] - mins[i] > radius) {
			radius = maxs[i] - mins[i];
		}
	}

	radius *= 0.5f;

	b = Brush_Alloc();

	for (i = 0; i < sides; i++) {
		for (j = 0; j < sides - 1; j++) {
			f = Face_Alloc();
			f->texdef = *texdef;
			f->next = b->brush_faces;
			b->brush_faces = f;

			f->planepts[0] = idPolar3(radius, idMath::TWO_PI * i / sides, idMath::PI * ((float)(j) / sides - 0.5f) ).ToVec3() + mid;
			f->planepts[1] = idPolar3(radius, idMath::TWO_PI * i / sides, idMath::PI * ((float)(j+1) / sides - 0.5f) ).ToVec3() + mid;
			f->planepts[2] = idPolar3(radius, idMath::TWO_PI * (i+1) / sides, idMath::PI * ((float)(j+1) / sides - 0.5f) ).ToVec3() + mid;
		}
	}

	Brush_AddToList(b, &selected_brushes);

	Entity_LinkBrush(world_entity, b);

	Brush_Build(b);

	Sys_UpdateWindows(W_ALL);
}

extern void Face_FitTexture_BrushPrimit(face_t *f, idVec3 mins, idVec3 maxs, float nHeight, float nWidth);

/*
================
Face_FitTexture
================
*/
void Face_FitTexture(face_t *face, float nHeight, float nWidth) {
	idVec3	mins, maxs;
	mins[0] = maxs[0] = 0;
	Face_FitTexture_BrushPrimit(face, mins, maxs, nHeight, nWidth);
}

/*
================
Brush_FitTexture
================
*/
void Brush_FitTexture(brush_t *b, float nHeight, float nWidth) {
	face_t	*face;
	for (face = b->brush_faces; face; face = face->next) {
		Face_FitTexture(face, nHeight, nWidth);
	}
}

void Brush_GetBounds( brush_t *b, idBounds &bo ) {
	if ( b == NULL ) {
		return;
	}

	bo.Clear();
	bo.AddPoint( b->mins );
	bo.AddPoint( b->maxs );

	if ( b->owner->curve ) {
		int c = b->owner->curve->GetNumValues();
		for ( int i = 0; i < c; i++ ) {
			bo.AddPoint ( b->owner->curve->GetValue( i ) );
		}
	}

}




void Brush_AddBrushLines(fhImmediateMode& im, const brush_t* brush, const brush_t* end, ViewType viewType) {
  for (; brush != end; brush = brush->next) {
    if (brush->hiddenBrush) {
      continue;
    }

    if (FilterBrush(brush)) {
      continue;
    }

    for (const face_t* face = brush->brush_faces; face; face = face->next) {
      // only draw polygons facing in a direction we care about
      if (viewType == ViewType::XY) {
        if (face->plane[2] <= 0) {
          continue;
        }
      }
      else {
        if (viewType == ViewType::XZ) {
          if (face->plane[1] <= 0) {
            continue;
          }
        }
        else {
          if (face->plane[0] <= 0) {
            continue;
          }
        }
      }

      const idWinding* w = face->face_winding;
      if (!w || w->GetNumPoints() < 2) {
        continue;
      }

      im.Vertex3fv((*w)[0].ToFloatPtr());
      for (int i = 1; i < (w->GetNumPoints() - 1); i++) {
        im.Vertex3fv((*w)[i].ToFloatPtr());
        im.Vertex3fv((*w)[i].ToFloatPtr());
      }
      im.Vertex3fv((*w)[w->GetNumPoints() - 1].ToFloatPtr());
      im.Vertex3fv((*w)[w->GetNumPoints() - 1].ToFloatPtr());
      im.Vertex3fv((*w)[0].ToFloatPtr());
    }
  }
}