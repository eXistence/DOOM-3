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

void		Eclass_Cleanup();
void		Eclass_InitForSourceDirectory();
eclass_t *	Eclass_ForName( const char *name, bool has_brushes );

//TODO(johl): rename to 'EditorEntity', change from struct to class, add all
//            those Entity_* functions as proper member functions
struct entity_t {
	entity_t	*prev, *next;
	brush_t		brushes;					// head/tail of list
	int			undoId, redoId, entityId;	// used for undo/redo
	idVec3		origin;
	qhandle_t	lightDef;
	qhandle_t	modelDef;
	idSoundEmitter* soundEmitter;
	const eclass_t* eclass;
	idDict		epairs;
	idMat3		rotation;
	idVec3 		lightOrigin;		// for lights that have been combined with models
	idMat3		lightRotation;		// ''
	bool		trackLightOrigin;
	idCurve<idVec3> *curve;

	entity_t();
	~entity_t();

	const char *ValueForKey( const char *key ) const;
	int			GetNumKeys() const;
	const char *GetKeyString( int iIndex ) const;
	void		SetKeyValue( const char *key, const char *value, bool trackAngles = true );
	void		DeleteKey( const char *key );
	float		FloatForKey( const char *key );
	int			IntForKey( const char *key );
	bool		GetVectorForKey( const char *key, idVec3 &vec );
	bool		GetVector4ForKey( const char *key, idVec4 &vec );
	bool		GetFloatForKey( const char *key, float *f );
	void		SetKeyVec3( const char *key, idVec3 v );
	void		SetKeyMat3( const char *key, idMat3 m );
	bool		GetMatrixForKey( const char *key, idMat3 &mat );
	bool    HasVec3ForKey(const char *key);

	void		UpdateSoundEmitter();
	idCurve<idVec3>* MakeCurve();
	void		UpdateCurveData();
	void		SetCurveData();

	void		FreeEpairs();
	int			MemorySize() const;

	void		WriteSelected( FILE *f );
	void		WriteSelected( CMemFile* );

	entity_t *	Clone() const;
	void		AddToList( entity_t *list );
	void		RemoveFromList();
	bool		HasModel() const;


	void        PostParse(brush_t *pList);

	void		SetName(const char *name );

	//Timo : used for parsing epairs in brush primitive
	void		Name(bool force );

};

void		ParseEpair(idDict *dict);


entity_t *	Entity_Parse (bool onlypairs, brush_t* pList = NULL);
entity_t *	Entity_Create (eclass_t *c, bool forceFixed = false);

void		Entity_LinkBrush (entity_t *e, brush_t *b);
void		Entity_UnlinkBrush (brush_t *b);
entity_t *	FindEntity(const char *pszKey, const char *pszValue);
entity_t *	FindEntityInt(const char *pszKey, int iValue);

bool		IsBrushSelected(const brush_t* bSel);
