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
#include "../../renderer/tr_local.h"
#include "../../renderer/model_local.h"	// for idRenderModelMD5
static int g_entityId = 1;

#define CURVE_TAG "curve_"

extern void Brush_Resize(brush_t *b, idVec3 vMin, idVec3 vMax);

int	entity_t::GetNumKeys() const
{
	int iCount = epairs.GetNumKeyVals();
	return iCount;
}

const char * entity_t::GetKeyString(int iIndex) const
{
	if ( iIndex < GetNumKeys() )
	{
		return epairs.GetKeyVal(iIndex)->GetKey().c_str();
	}

	assert(0);
	return NULL;
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
const char * entity_t::ValueForKey(const char *key) const {
	return epairs.GetString(key);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TrackMD3Angles(entity_t *e, const char *key, const char *value) {
	if ( idStr::Icmp(key, "angle") != 0 ) {
		return;
	}

	if ((e->eclass->fixedsize && e->eclass->nShowFlags & ECLASS_MISCMODEL) || e->HasModel()) {
		float	a = e->FloatForKey("angle");
		float	b = atof(value);
		if (a != b) {
			idVec3	vAngle;
			vAngle[0] = vAngle[1] = 0;
			vAngle[2] = -a;
			Brush_Rotate(e->brushes.onext, vAngle, e->origin, true);
			vAngle[2] = b;
			Brush_Rotate(e->brushes.onext, vAngle, e->origin, true);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void entity_t::SetKeyValue( const char *key, const char *value, bool trackAngles) {
	if (!key || !key[0]) {
		return;
	}

	if (trackAngles) {
		TrackMD3Angles(this, key, value);
	}

	epairs.Set(key, value);
	GetVectorForKey("origin", origin);

	// update sound in case this key was relevent
	UpdateSoundEmitter();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void entity_t::SetKeyVec3( const char *key, idVec3 v) {
	if (!key || !key[0]) {
		return;
	}

	idStr str;
	sprintf(str, "%g %g %g", v.x, v.y, v.z);
	epairs.Set(key, str);
	GetVectorForKey("origin", origin);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void entity_t::SetKeyMat3( const char *key, idMat3 m) {
	if (!key || !key[0]) {
		return;
	}

	idStr str;

	sprintf(str, "%g %g %g %g %g %g %g %g %g",m[0][0],m[0][1],m[0][2],m[1][0],m[1][1],m[1][2],m[2][0],m[2][1],m[2][2]);

	epairs.Set(key, str);
	GetVectorForKey("origin", origin);
}



/*
 =======================================================================================================================
 =======================================================================================================================
 */
void entity_t::DeleteKey( const char *key) {
	epairs.Delete(key);
	if (stricmp(key, "rotation") == 0) {
		rotation.Identity();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
float entity_t::FloatForKey( const char *key) {
	const char* k = ValueForKey(key);
	if (k && *k) {
		return atof(k);
	}

	return 0.0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int entity_t::IntForKey(const char *key) {
	const char* k = ValueForKey(key);
	return atoi(k);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool entity_t::GetVectorForKey(const char *key, idVec3 &vec) {
	const char* k = ValueForKey(key);
	if (k && strlen(k) > 0) {
		sscanf(k, "%f %f %f", &vec[0], &vec[1], &vec[2]);
		return true;
	}
	else {
		vec[0] = vec[1] = vec[2] = 0;
	}

	return false;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool entity_t::GetVector4ForKey(const char *key, idVec4 &vec) {
	const char* k = ValueForKey(key);
	if (k && strlen(k) > 0) {
		sscanf(k, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
		return true;
	}
	else {
		vec[0] = vec[1] = vec[2] = vec[3] = 0;
	}

	return false;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool entity_t::GetFloatForKey(const char *key, float *f) {
	const char* k = ValueForKey(key);
	if (k && strlen(k) > 0) {
		*f = atof(k);
		return true;
	}

	*f = 0;
	return false;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool entity_t::GetMatrixForKey(const char *key, idMat3 &mat) {
	const char* k = ValueForKey(key);
	if (k && strlen(k) > 0) {
		sscanf
		(
			k,
			"%f %f %f %f %f %f %f %f %f ",
			&mat[0][0],
			&mat[0][1],
			&mat[0][2],
			&mat[1][0],
			&mat[1][1],
			&mat[1][2],
			&mat[2][0],
			&mat[2][1],
			&mat[2][2]
		);
		return true;
	}
	else {
		mat.Identity();
	}

	return false;
}

/*
 =======================================================================================================================
    Entity_FreeEpairs Frees the entity epairs.
 =======================================================================================================================
 */
void entity_t::FreeEpairs() {
	epairs.Clear();
}

/*
 =======================================================================================================================
    Entity_AddToList
 =======================================================================================================================
 */
void entity_t::AddToList(entity_t *list) {
	if (next || prev) {
		Error("Entity_AddToList: allready linked");
	}

	next = list->next;
	list->next->prev = this;
	list->next = this;
	prev = list;
}

/*
 =======================================================================================================================
    Entity_RemoveFromList
 =======================================================================================================================
 */
void entity_t::RemoveFromList() {
	if ( !next || !prev ) {
		Error("Entity_RemoveFromList: not linked");
	}

	next->prev = prev;
	prev->next = next;
	next = prev = NULL;
}

/*
 =======================================================================================================================
    Entity_Free Frees the entity and any brushes is has. The entity is removed from the global entities list.
 =======================================================================================================================
 */
entity_t::~entity_t() {

	while ( brushes.onext != &brushes ) {
		Brush_Free(brushes.onext);
	}

	if ( next ) {
		next->prev = prev;
		prev->next = next;
	}

	FreeEpairs();
}

/*
 =======================================================================================================================
    Entity_MemorySize
 =======================================================================================================================
 */

int entity_t::MemorySize() const
{
	int size = sizeof( entity_t ) + epairs.Size();
	for( const brush_t* b = brushes.onext; b != &brushes; b = b->onext ) {
		size += Brush_MemorySize( b );
	}
	return( size );
}

/*
 =======================================================================================================================
    ParseEpair
 =======================================================================================================================
 */

struct EpairFixup {
	const char *name;
	int type;
};


const EpairFixup FloatFixups[] = {
	{ "origin", 1 },
	{ "rotation", 2 },
	{ "_color", 1 },
	{ "falloff", 0 },
	{ "light", 0 },
	{ "light_target", 1 },
	{ "light_up", 1 },
	{ "light_right", 1 },
	{ "light_start", 1 },
	{ "light_center", 1 },
	{ "light_end", 1 },
	{ "light_radius", 1 },
	{ "light_origin", 1 }
};

const int FixupCount = sizeof(FloatFixups) / sizeof(EpairFixup);

void FixFloats(idDict *dict) {
	int count = dict->GetNumKeyVals();
	for (int i = 0; i < count; i++) {
		const idKeyValue *kv = dict->GetKeyVal(i);
		for (int j = 0; j < FixupCount; j++) {
			if (kv->GetKey().Icmp(FloatFixups[j].name) == 0) {
				idStr val;
				if (FloatFixups[j].type == 1) {
					idVec3 v;
					sscanf(kv->GetValue().c_str(), "%f %f %f", &v.x, &v.y, &v.z);
					sprintf(val, "%g %g %g", v.x, v.y, v.z);
				} else if (FloatFixups[j].type == 2) {
					idMat3 mat;
					sscanf(kv->GetValue().c_str(),	"%f %f %f %f %f %f %f %f %f ",&mat[0][0],&mat[0][1],&mat[0][2],&mat[1][0],&mat[1][1],&mat[1][2],&mat[2][0],&mat[2][1],&mat[2][2]);
					sprintf(val, "%g %g %g %g %g %g %g %g %g",mat[0][0],mat[0][1],mat[0][2],mat[1][0],mat[1][1],mat[1][2],mat[2][0],mat[2][1],mat[2][2]);
				} else {
					float f = atof(kv->GetValue().c_str());
					sprintf(val, "%g", f);
				}
				dict->Set(kv->GetKey(), val);
				break;
			}
		}
	}
}

void ParseEpair(idDict *dict) {
	idStr key = token;
	GetToken(false);
	idStr val = token;

	if (key.Length() > 0) {
		dict->Set(key, val);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool entity_t::HasModel() const {
	const char	*model = ValueForKey("model");
	const char	*name = ValueForKey("name");
	if (model && *model) {
		if ( idStr::Icmp(model, name) ) {
			return true;
		}
	}

	return false;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
entity_t::entity_t() {
	prev = next = NULL;
	brushes.prev = brushes.next = NULL;
	brushes.oprev = brushes.onext = NULL;
	brushes.owner = NULL;
	undoId = 0;
	redoId = 0;
	entityId = g_entityId++;
	origin.Zero();
	eclass = NULL;
	lightOrigin.Zero();
	lightRotation.Identity();
	trackLightOrigin = false;
	rotation.Identity();
	lightDef = -1;
	modelDef = -1;
	soundEmitter = NULL;
	curve = NULL;
}

void entity_t::UpdateCurveData() {

	if ( this->curve == NULL ) {
		return;
	}

	const idKeyValue *kv = this->epairs.MatchPrefix( CURVE_TAG );
	if ( kv == NULL ) {
		if ( this->curve ) {
			delete this->curve;
			this->curve = NULL;
			if ( g_qeglobals.d_select_mode == sel_editpoint ) {
				g_qeglobals.d_select_mode = sel_brush;
			}
		}
		return;
	}

	int c = this->curve->GetNumValues();
	idStr str = va( "%i ( ", c );
	idVec3 v;
	for ( int i = 0; i < c; i++ ) {
		v = this->curve->GetValue( i );
		str += " ";
		str += v.ToString();
		str += " ";
	}
	str += " )";

	this->epairs.Set( kv->GetKey(), str );
}

idCurve<idVec3>* entity_t::MakeCurve() {
	const idKeyValue *kv = this->epairs.MatchPrefix( CURVE_TAG );
	if ( kv ) {
		idStr str = kv->GetKey().Right( kv->GetKey().Length() - strlen( CURVE_TAG ) );
		if ( str.Icmp( "CatmullRomSpline" ) == 0 ) {
			return new idCurve_CatmullRomSpline<idVec3>();
		} else if ( str.Icmp( "Nurbs" ) == 0 ) {
			return new idCurve_NURBS<idVec3>();
		}
	}
	return NULL;
}

void entity_t::SetCurveData() {

	this->curve = this->MakeCurve();
	const idKeyValue *kv = this->epairs.MatchPrefix( CURVE_TAG );
	if ( kv && this->curve ) {
		idLexer lex;
		lex.LoadMemory( kv->GetValue(), kv->GetValue().Length(), "_curve" );
		int numPoints = lex.ParseInt();
		if ( numPoints > 0 ) {
			float *fp = new float[numPoints * 3];
			lex.Parse1DMatrix( numPoints * 3, fp );
			int time = 0;
			for ( int i = 0; i < numPoints * 3; i += 3 ) {
				idVec3 v;
				v.x = fp[i];
				v.y = fp[i+1];
				v.z = fp[i+2];
				this->curve->AddValue( time, v );
				time += 100;
			}
			delete []fp;
		}
	}

}

void entity_t::PostParse(brush_t *pList) {
	bool		has_brushes;
	eclass_t	*e;
	brush_t		*b;
	idVec3		mins, maxs, zero;
	idBounds bo;

	zero.Zero();

	SetCurveData();

	if (brushes.onext == &brushes) {
		has_brushes = false;
	}
	else {
		has_brushes = true;
	}

	bool needsOrigin = !GetVectorForKey("origin", origin);
	const char	*pModel = ValueForKey("model");

	const char *cp = ValueForKey("classname");

	if (strlen(cp)) {
		e = Eclass_ForName(cp, has_brushes);
	} else {
		const char *cp2 = ValueForKey("name");
		if (strlen(cp2)) {
			char buff[1024];
			strcpy(buff, cp2);
			int len = strlen(buff);
			while ((isdigit(buff[len-1]) || buff[len-1] == '_') && len > 0) {
				buff[len-1] = '\0';
				len--;
			}
			e = Eclass_ForName(buff, has_brushes);
			SetKeyValue("classname", buff, false);
		} else {
			e = Eclass_ForName("", has_brushes);
		}
	}

	idStr str;

	if (e->defArgs.GetString("model", "", str) && e->entityModel == NULL) {
		e->entityModel = gameEdit->ANIM_GetModelFromEntityDef( &e->defArgs );
	}

	eclass = e;

	bool hasModel = HasModel();

	if (hasModel) {
		eclass->defArgs.GetString("model", "", str);
		if (str.Length()) {
			hasModel = false;
			epairs.Delete("model");
		}
	}

	if (e->nShowFlags & ECLASS_WORLDSPAWN) {
		origin.Zero();
		needsOrigin = false;
		epairs.Delete( "model" );
	} else if (e->nShowFlags & ECLASS_LIGHT) {
		if (GetVectorForKey("light_origin", lightOrigin)) {
			GetMatrixForKey("light_rotation", lightRotation);
			trackLightOrigin = true;
		} else if (hasModel) {
			SetKeyValue("light_origin", ValueForKey("origin"));
			lightOrigin = origin;
			if (GetMatrixForKey("rotation", lightRotation)) {
				SetKeyValue("light_rotation", ValueForKey("rotation"));
			}
			trackLightOrigin = true;
		}
	} else if ( e->nShowFlags & ECLASS_ENV ) {
		// need to create an origin from the bones here
		idVec3 org;
		idAngles ang;
		bo.Clear();
		bool hasBody = false;
		const idKeyValue *arg = epairs.MatchPrefix( "body ", NULL );
		while ( arg ) {
			sscanf( arg->GetValue(), "%f %f %f %f %f %f", &org.x, &org.y, &org.z, &ang.pitch, &ang.yaw, &ang.roll );
			bo.AddPoint( org );
			arg = epairs.MatchPrefix( "body ", arg );
			hasBody = true;
		}
		if (hasBody) {
			origin = bo.GetCenter();
		}
	}

	if (e->fixedsize || hasModel) {			// fixed size entity
		if (brushes.onext != &brushes) {
			for (b = brushes.onext; b != &brushes; b = b->onext) {
				b->entityModel = true;
			}
		}

		if (hasModel) {
			// model entity
			idRenderModel *modelHandle = renderModelManager->FindModel( pModel );

			if ( dynamic_cast<idRenderModelPrt*>( modelHandle ) || dynamic_cast<idRenderModelLiquid*>( modelHandle ) ) {
				bo.Zero();
				bo.ExpandSelf( 12.0f );
			} else {
				bo = modelHandle->Bounds( NULL );
			}

			VectorCopy(bo[0], mins);
			VectorCopy(bo[1], maxs);
			for (int i = 0; i < 3; i++) {
				if (mins[i] == maxs[i]) {
					mins[i]--;
					maxs[i]++;
				}
			}
			VectorAdd(mins, origin, mins);
			VectorAdd(maxs, origin, maxs);
			b = Brush_Create(mins, maxs, &e->texdef);
			b->modelHandle = modelHandle;

			float		yaw = 0;
			bool		convertAngles = GetFloatForKey("angle", &yaw);
			extern void Brush_Rotate(brush_t *b, idMat3 matrix, idVec3 origin, bool bBuild);
			extern void Brush_Rotate(brush_t *b, idVec3 rot, idVec3 origin, bool bBuild);

			if (convertAngles) {
				idVec3	rot(0, 0, yaw);
				Brush_Rotate(b, rot, origin, false);
			}

			if (GetMatrixForKey("rotation", rotation)) {
				idBounds bo2;
				bo2.FromTransformedBounds(bo, origin, rotation);
				b->owner = this;
				Brush_Resize(b, bo2[0], bo2[1]);
			}
			Entity_LinkBrush(this, b);
		}

		if (!hasModel || (eclass->nShowFlags & ECLASS_LIGHT && hasModel)) {
			// create a custom brush
			if (trackLightOrigin) {
				mins = e->mins + lightOrigin;
				maxs = e->maxs + lightOrigin;
			} else {
				mins = e->mins + origin;
				maxs = e->maxs + origin;
			}

			b = Brush_Create(mins, maxs, &e->texdef);
			GetMatrixForKey("rotation", rotation);
			Entity_LinkBrush(this, b);
			b->trackLightOrigin = trackLightOrigin;
			if ( e->texdef.name == NULL ) {
				brushprimit_texdef_t bp;
				texdef_t td;
				td.SetName( eclass->defMaterial );
				Brush_SetTexture( b, &td, &bp, false );
			}
		}
	} else {	// brush entity
		if (brushes.next == &brushes) {
			printf("Warning: Brush entity with no brushes\n");
		}

		if (!needsOrigin) {
			idStr cn = ValueForKey("classname");
			idStr name = ValueForKey("name");
			idStr model = ValueForKey("model");
			if (cn.Icmp("func_static") == 0) {
				if (name.Icmp(model) == 0) {
					needsOrigin = true;
				}
			}
		}

		if (needsOrigin) {
			idVec3	mins, maxs, mid;
			int		i;
			char	text[32];
			mins[0] = mins[1] = mins[2] = 999999;
			maxs[0] = maxs[1] = maxs[2] = -999999;

			// add in the origin
			for (b = brushes.onext; b != &brushes; b = b->onext) {
				Brush_Build(b, true, false, false);
				for (i = 0; i < 3; i++) {
					if (b->mins[i] < mins[i]) {
						mins[i] = b->mins[i];
					}

					if (b->maxs[i] > maxs[i]) {
						maxs[i] = b->maxs[i];
					}
				}
			}

			for (i = 0; i < 3; i++) {
				origin[i] = (mins[i] + ((maxs[i] - mins[i]) / 2));
			}

			sprintf(text, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
			SetKeyValue("origin", text);
		}

		if (!(e->nShowFlags & ECLASS_WORLDSPAWN)) {
			if (e->defArgs.FindKey("model") == NULL && (pModel == NULL || (pModel && strlen(pModel) == 0))) {
				SetKeyValue("model", ValueForKey("name"));
			}
		}
		else {
			DeleteKey("origin");
		}
	}

	// add all the brushes to the main list
	if (pList) {
		for (b = brushes.onext; b != &brushes; b = b->onext) {
			b->next = pList->next;
			pList->next->prev = b;
			b->prev = pList;
			pList->next = b;
		}
	}

	FixFloats(&epairs);
}

/*
 =======================================================================================================================
    Entity_Parse If onlypairs is set, the classname info will not be looked up, and the entity will not be added to the
    global list. Used for parsing the project.
 =======================================================================================================================
 */
entity_t *Entity_Parse(bool onlypairs, brush_t *pList) {
	entity_t	*ent;

	if (!GetToken(true)) {
		return NULL;
	}

	if (strcmp(token, "{")) {
		Error("ParseEntity: { not found");
	}

	ent = new entity_t();
	ent->brushes.onext = ent->brushes.oprev = &ent->brushes;
	ent->origin.Zero();

	int n = 0;
	do {
		if (!GetToken(true)) {
			Warning("ParseEntity: EOF without closing brace");
			return NULL;
		}

		if (!strcmp(token, "}")) {
			break;
		}

		if (!strcmp(token, "{")) {
			ent->GetVectorForKey("origin", ent->origin);
			brush_t *b = Brush_Parse(ent->origin);
			if (b != NULL) {
				b->owner = ent;

				// add to the end of the entity chain
				b->onext = &ent->brushes;
				b->oprev = ent->brushes.oprev;
				ent->brushes.oprev->onext = b;
				ent->brushes.oprev = b;
			}
			else {
				break;
			}
		}
		else {
			ParseEpair(&ent->epairs);
		}
	} while (1);

	if (onlypairs) {
		return ent;
	}

	ent->PostParse(pList);
	return ent;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void VectorMidpoint(idVec3 va, idVec3 vb, idVec3 &out) {
	for (int i = 0; i < 3; i++) {
		out[i] = va[i] + ((vb[i] - va[i]) / 2);
	}
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
bool IsBrushSelected(const brush_t *bSel) {
	for (const brush_t * b = selected_brushes.next; b != NULL && b != &selected_brushes; b = b->next) {
		if (b == bSel) {
			return true;
		}
	}

	return false;
}

//
// =======================================================================================================================
//    Entity_WriteSelected
// =======================================================================================================================
//
void entity_t::WriteSelected(FILE *file) {
	const brush_t *b;

	for (b = brushes.onext; b != &brushes; b = b->onext) {
		if (IsBrushSelected(b)) {
			break;	// got one
		}
	}

	if (b == &brushes) {
		return;		// nothing selected
	}

	// if fixedsize, calculate a new origin based on the current brush position
	if (eclass->fixedsize || HasModel()) {
		idVec3	origin;

		if (!GetVectorForKey("origin", origin)) {
			char text[128];
			VectorSubtract(brushes.onext->mins, eclass->mins, origin);
			sprintf(text, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
			SetKeyValue("origin", text);
		}
	}

	fprintf(file, "{\n");

	for (int j = 0; j < epairs.GetNumKeyVals(); j++) {
		fprintf(file, "\"%s\" \"%s\"\n", epairs.GetKeyVal(j)->GetKey().c_str(), epairs.GetKeyVal(j)->GetValue().c_str());
	}

	if (!HasModel()) {
		int count = 0;
		for (brush_t* b = brushes.onext; b != &brushes; b = b->onext) {
			if (eclass->fixedsize && !b->entityModel) {
				continue;
			}
			if (IsBrushSelected(b)) {
				fprintf(file, "// brush %i\n", count);
				count++;
				Brush_Write( b, file, origin, ( g_PrefsDlg.m_bNewMapFormat != FALSE ) );
			}
		}
	}

	fprintf(file, "}\n");
}

//
// =======================================================================================================================
//    Entity_WriteSelected to a CMemFile
// =======================================================================================================================
//
void entity_t::WriteSelected(CMemFile *pMemFile) {
	brush_t *b;
	idVec3	origin;
	char	text[128];
	int		count;

	for (b = brushes.onext; b != &brushes; b = b->onext) {
		if (IsBrushSelected(b)) {
			break;	// got one
		}
	}

	if (b == &brushes) {
		return;		// nothing selected
	}

	// if fixedsize, calculate a new origin based on the current brush position
	if (eclass->fixedsize || HasModel()) {
		if (!GetVectorForKey("origin", origin)) {
			VectorSubtract(brushes.onext->mins, eclass->mins, origin);
			sprintf(text, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
			SetKeyValue("origin", text);
		}
	}

	MemFile_fprintf(pMemFile, "{\n");

	count = epairs.GetNumKeyVals();
	for (int j = 0; j < count; j++) {
		MemFile_fprintf(pMemFile, "\"%s\" \"%s\"\n", epairs.GetKeyVal(j)->GetKey().c_str(), epairs.GetKeyVal(j)->GetValue().c_str());
	}

	if (!HasModel()) {
		count = 0;
		for (b = brushes.onext; b != &brushes; b = b->onext) {
			if (eclass->fixedsize && !b->entityModel) {
				continue;
			}
			if (IsBrushSelected(b)) {
				MemFile_fprintf(pMemFile, "// brush %i\n", count);
				count++;
				Brush_Write( b, pMemFile, origin, ( g_PrefsDlg.m_bNewMapFormat != FALSE ) );
			}
		}
	}

	MemFile_fprintf(pMemFile, "}\n");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void entity_t::SetName(const char *name) {
	CString oldName = ValueForKey("name");
	CString oldModel = ValueForKey("model");
	SetKeyValue("name", name);
	if (oldName == oldModel) {
		SetKeyValue("model", name);
	}
}

extern bool Entity_NameIsUnique(const char *name);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void entity_t::Name(bool force) {
	const char	*name = ValueForKey("name");

	if (!force && name && name[0]) {
		return;
	}

	if (name && name[0] && Entity_NameIsUnique(name)) {
		return;
	}

	bool	setModel = false;
	if (name[0]) {
		const char	*model = ValueForKey("model");
		if (model[0]) {
			if ( idStr::Icmp(model, name) == 0 ) {
				setModel = true;
			}
		}
	}

	const char *eclass = ValueForKey("classname");
	if (eclass && eclass[0]) {
		idStr str = cvarSystem->GetCVarString( "radiant_nameprefix" );
		int id = Map_GetUniqueEntityID(str, eclass);
		if (str.Length()) {
			SetKeyValue("name", va("%s_%s_%i", str.c_str(), eclass, id));
		} else {
			SetKeyValue("name", va("%s_%i", eclass, id));
		}
		if (setModel) {
			if (str.Length()) {
				SetKeyValue("model", va("%s_%s_%i", str.c_str(), eclass, id));
			} else {
				SetKeyValue("model", va("%s_%i",  eclass, id));
			}
		}
	}
}

/*
 =======================================================================================================================
    Entity_Create Creates a new entity out of the selected_brushes list. If the entity class is fixed size, the brushes
    are only used to find a midpoint. Otherwise, the brushes have their ownership transfered to the new entity.
 =======================================================================================================================
 */
entity_t *Entity_Create(eclass_t *entityClass, bool forceFixed) {
	entity_t	*e;
	brush_t		*b;
	idVec3		mins, maxs, origin;
	char		text[32];
	texdef_t td;
	brushprimit_texdef_t bp;

	// check to make sure the brushes are ok
	for (b = selected_brushes.next; b != &selected_brushes; b = b->next) {
		if (b->owner != world_entity) {
			Sys_Status("Entity NOT created, brushes not all from world\n");
			Sys_Beep();
			return NULL;
		}
	}

	idStr str;
	if (entityClass->defArgs.GetString("model", "", str) && entityClass->entityModel == NULL) {
		entityClass->entityModel = gameEdit->ANIM_GetModelFromEntityDef( &entityClass->defArgs );
	}

	// create it
	e = new entity_t();
	e->brushes.onext = e->brushes.oprev = &e->brushes;
	e->eclass = entityClass;
	e->epairs.Copy(entityClass->args);
	e->SetKeyValue("classname", entityClass->name);
	e->Name(false);

	// add the entity to the entity list
	e->AddToList(&entities);

	if (entityClass->fixedsize) {
		//
		// just use the selection for positioning b = selected_brushes.next; for (i=0 ;
		// i<3 ; i++) { e->origin[i] = b->mins[i] - c->mins[i]; }
		//
		Select_GetMid(e->origin);
		VectorCopy(e->origin, origin);

		// create a custom brush
		VectorAdd(entityClass->mins, e->origin, mins);
		VectorAdd(entityClass->maxs, e->origin, maxs);

		b = Brush_Create(mins, maxs, &entityClass->texdef);

		Entity_LinkBrush(e, b);

		if (entityClass->defMaterial.Length()) {
			td.SetName(entityClass->defMaterial);
			Brush_SetTexture(b, &td, &bp, false);
		}


		// delete the current selection
		Select_Delete();

		// select the new brush
		b->next = b->prev = &selected_brushes;
		selected_brushes.next = selected_brushes.prev = b;

		Brush_Build(b);
	}
	else {

		Select_GetMid(origin);

		// change the selected brushes over to the new entity
		for (b = selected_brushes.next; b != &selected_brushes; b = b->next) {
			Entity_UnlinkBrush(b);
			Entity_LinkBrush(e, b);
			Brush_Build(b); // so the key brush gets a name
			if (entityClass->defMaterial.Length()) {
				td.SetName(entityClass->defMaterial);
				Brush_SetTexture(b, &td, &bp, false);
			}

		}

		//for (int i = 0; i < 3; i++) {
		//	origin[i] = vMin[i] + vMax[i] * 0.5;
		//}

		if (!forceFixed) {
			e->SetKeyValue("model", e->ValueForKey("name"));
		}
	}

	sprintf(text, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
	e->SetKeyValue("origin", text);
	VectorCopy(origin, e->origin);

	Sys_UpdateWindows(W_ALL);
	return e;
}

void Brush_MakeDirty(brush_t *b) {
	for (face_t *f = b->brush_faces; f; f = f->next) {
		f->dirty = true;
	}
}
/*
 =======================================================================================================================
    Entity_LinkBrush
 =======================================================================================================================
 */
void Entity_LinkBrush(entity_t *e, brush_t *b) {
	if (b->oprev || b->onext) {
		Error("Entity_LinkBrush: Allready linked");
	}

	Brush_MakeDirty(b);

	b->owner = e;

	b->onext = e->brushes.onext;
	b->oprev = &e->brushes;
	e->brushes.onext->oprev = b;
	e->brushes.onext = b;
}

/*
 =======================================================================================================================
    Entity_UnlinkBrush
 =======================================================================================================================
 */
void Entity_UnlinkBrush(brush_t *b) {
	// if (!b->owner || !b->onext || !b->oprev)
	if (!b->onext || !b->oprev) {
		Error("Entity_UnlinkBrush: Not currently linked");
	}

	b->onext->oprev = b->oprev;
	b->oprev->onext = b->onext;
	b->onext = b->oprev = NULL;
	b->owner = NULL;
}

/*
 =======================================================================================================================
    Entity_Clone
 =======================================================================================================================
 */
entity_t *entity_t::Clone() const {
	entity_t	*n = new entity_t();
	n->brushes.onext = n->brushes.oprev = &n->brushes;
	n->eclass = eclass;
	n->rotation = rotation;
	n->origin = origin;
	n->AddToList(&entities);
	n->epairs  = epairs;

	return n;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
entity_t *FindEntity(const char *pszKey, const char *pszValue) {
	entity_t* pe = entities.next;

	for (; pe != NULL && pe != &entities; pe = pe->next) {
		if (!strcmp(pe->ValueForKey(pszKey), pszValue)) {
			return pe;
		}
	}

	return NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
entity_t *FindEntityInt(const char *pszKey, int iValue) {
	entity_t* pe = entities.next;

	for (; pe != NULL && pe != &entities; pe = pe->next) {
		if (pe->IntForKey(pszKey) == iValue) {
			return pe;
		}
	}

	return NULL;
}

/*
====================
Entity_UpdateSoundEmitter

Deletes the soundEmitter if the entity should not emit a sound due
to it not having one, being filtered away, or the sound mode being turned off.

Creates or updates the soundEmitter if needed
====================
*/
void entity_t::UpdateSoundEmitter() {
	bool	playing = false;

	// if an entity doesn't have any brushes at all, don't do anything
	// if the brush isn't displayed (filtered or culled), don't do anything
	if ( g_pParentWnd->GetCameraWindow()->GetSoundMode()
		&& this->brushes.onext != &this->brushes && !FilterBrush(this->brushes.onext) ) {
		// check for sounds
		const char *v = ValueForKey( "s_shader" );
		if ( v[0] ) {
			refSound_t	sound;

			gameEdit->ParseSpawnArgsToRefSound( &this->epairs, &sound );
			if ( !sound.waitfortrigger ) {	// waitfortrigger will not start playing immediately
				if ( !this->soundEmitter ) {
					this->soundEmitter = g_qeglobals.sw->AllocSoundEmitter();
				}
				playing = true;
				this->soundEmitter->UpdateEmitter( this->origin, 0, &sound.parms );
				// always play on a single channel, so updates always override
				this->soundEmitter->StartSound( sound.shader, SCHANNEL_ONE );
			}
		}
	}

	// delete the soundEmitter if not used
	if ( !playing && this->soundEmitter ) {
		this->soundEmitter->Free( true );
		this->soundEmitter = NULL;
	}
}

