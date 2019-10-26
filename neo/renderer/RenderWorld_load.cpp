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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"


/*
================
idRenderWorldLocal::FreeWorld
================
*/
void idRenderWorldLocal::FreeWorld() {
	int i;

	// this will free all the lightDefs and entityDefs
	FreeDefs();

	// free all the portals and check light/model references
	for ( i = 0 ; i < numPortalAreas ; i++ ) {
		portalArea_t	*area;
		portal_t		*portal, *nextPortal;

		area = &portalAreas[i];
		for ( portal = area->portals ; portal ; portal = nextPortal ) {
			nextPortal = portal->next;
			delete portal->w;
			R_StaticFree( portal );
		}

		// there shouldn't be any remaining lightRefs or entityRefs
		if ( area->lightRefs.areaNext != &area->lightRefs ) {
			common->Error( "FreeWorld: unexpected remaining lightRefs" );
		}
		if ( area->entityRefs.areaNext != &area->entityRefs ) {
			common->Error( "FreeWorld: unexpected remaining entityRefs" );
		}
	}

	if ( portalAreas ) {
		R_StaticFree( portalAreas );
		portalAreas = NULL;
		numPortalAreas = 0;
		R_StaticFree( areaScreenRect );
		areaScreenRect = NULL;
	}

	if ( doublePortals ) {
		R_StaticFree( doublePortals );
		doublePortals = NULL;
		numInterAreaPortals = 0;
	}

	if ( areaNodes ) {
		R_StaticFree( areaNodes );
		areaNodes = NULL;
	}

	// free all the inline idRenderModels
	for ( i = 0 ; i < localModels.Num() ; i++ ) {
		renderModelManager->RemoveModel( localModels[i] );
		delete localModels[i];
	}
	localModels.Clear();

	areaReferenceAllocator.Shutdown();
	interactionAllocator.Shutdown();
	areaNumRefAllocator.Shutdown();

	mapName = "<FREED>";
}

/*
================
idRenderWorldLocal::TouchWorldModels
================
*/
void idRenderWorldLocal::TouchWorldModels( void ) {
	for (int  i = 0 ; i < localModels.Num() ; i++) {
		renderModelManager->CheckModel( localModels[i]->Name() );
	}
}

/*
================
idRenderWorldLocal::ParseModel
================
*/
static idRenderModel* ParseModel( idLexer *src ) {
	idToken			token;

	src->ExpectTokenString( "{" );

	// parse the name
	src->ExpectAnyToken( &token );

	idRenderModel* model = renderModelManager->AllocModel();
	model->InitEmpty( token );

	int numSurfaces = src->ParseInt();
	if ( numSurfaces < 0 ) {
		src->Error( "R_ParseModel: bad numSurfaces" );
	}

	for ( int i = 0 ; i < numSurfaces ; i++ ) {
		src->ExpectTokenString( "{" );

		src->ExpectAnyToken( &token );

		modelSurface_t	surf;
		surf.shader = declManager->FindMaterial( token );

		((idMaterial*)surf.shader)->AddReference();

		srfTriangles_t* tri = R_AllocStaticTriSurf();
		surf.geometry = tri;

		tri->numVerts = src->ParseInt();
		tri->numIndexes = src->ParseInt();

		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		for ( int j = 0 ; j < tri->numVerts ; j++ ) {
			float	vec[8];

			src->Parse1DMatrix( 8, vec );

			tri->verts[j].xyz[0] = vec[0];
			tri->verts[j].xyz[1] = vec[1];
			tri->verts[j].xyz[2] = vec[2];
			tri->verts[j].st[0] = vec[3];
			tri->verts[j].st[1] = vec[4];
			tri->verts[j].normal[0] = vec[5];
			tri->verts[j].normal[1] = vec[6];
			tri->verts[j].normal[2] = vec[7];
		}

		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
		for ( int j = 0 ; j < tri->numIndexes ; j++ ) {
			tri->indexes[j] = src->ParseInt();
		}
		src->ExpectTokenString( "}" );

		// add the completed surface to the model
		model->AddSurface( surf );
	}

	src->ExpectTokenString( "}" );

	model->FinishSurfaces();

	return model;
}


/*
================
idRenderWorldLocal::ParseOccluder
================
*/
static idRenderModel* ParseOccluder( idLexer *src ) {
	idToken			token;

	src->ExpectTokenString( "{" );

	// parse the name
	src->ExpectAnyToken( &token );

	idRenderModel* model = renderModelManager->AllocModel();
	model->InitEmpty( token );

	int numSurfaces = src->ParseInt();
	if (numSurfaces < 0) {
		src->Error( "R_ParseModel: bad numSurfaces" );
	}

	for (int i = 0; i < numSurfaces; i++) {
		src->ExpectTokenString( "{" );

		src->ExpectAnyToken( &token );

		modelSurface_t	surf;
		surf.shader = declManager->FindMaterial( token );

		((idMaterial*)surf.shader)->AddReference();

		srfTriangles_t* tri = R_AllocStaticTriSurf();
		surf.geometry = tri;

		tri->numVerts = src->ParseInt();
		tri->numIndexes = src->ParseInt();
		bool texcoords = (src->ParseInt() != 1);

		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		for (int j = 0; j < tri->numVerts; j++) {
			float	vec[5];

			if(texcoords) {
				src->Parse1DMatrix( 5, vec );
			} else {
				src->Parse1DMatrix( 3, vec );
			}

			tri->verts[j].xyz[0] = vec[0];
			tri->verts[j].xyz[1] = vec[1];
			tri->verts[j].xyz[2] = vec[2];
			tri->verts[j].st[0] = vec[3];
			tri->verts[j].st[1] = vec[4];
		}

		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
		for (int j = 0; j < tri->numIndexes; j++) {
			tri->indexes[j] = src->ParseInt();
		}
		src->ExpectTokenString( "}" );

		// add the completed surface to the model
		model->AddSurface( surf );
	}

	src->ExpectTokenString( "}" );

	model->FinishSurfaces();

	return model;
}

/*
================
idRenderWorldLocal::ParseShadowModel
================
*/
static idRenderModel* ParseShadowModel( idLexer *src ) {
	idRenderModel	*model;
	idToken			token;
	int				j;
	srfTriangles_t	*tri;
	modelSurface_t	surf;

	src->ExpectTokenString( "{" );

	// parse the name
	src->ExpectAnyToken( &token );

	model = renderModelManager->AllocModel();
	model->InitEmpty( token );

	surf.shader = tr.defaultMaterial;

	tri = R_AllocStaticTriSurf();
	surf.geometry = tri;

	tri->numVerts = src->ParseInt();
	tri->numShadowIndexesNoCaps = src->ParseInt();
	tri->numShadowIndexesNoFrontCaps = src->ParseInt();
	tri->numIndexes = src->ParseInt();
	tri->shadowCapPlaneBits = src->ParseInt();

	R_AllocStaticTriSurfShadowVerts( tri, tri->numVerts );
	tri->bounds.Clear();
	for ( j = 0 ; j < tri->numVerts ; j++ ) {
		float	vec[8];

		src->Parse1DMatrix( 3, vec );
		tri->shadowVertexes[j].xyz[0] = vec[0];
		tri->shadowVertexes[j].xyz[1] = vec[1];
		tri->shadowVertexes[j].xyz[2] = vec[2];
		tri->shadowVertexes[j].xyz[3] = 1;		// no homogenous value

		tri->bounds.AddPoint( tri->shadowVertexes[j].xyz.ToVec3() );
	}

	R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
	for ( j = 0 ; j < tri->numIndexes ; j++ ) {
		tri->indexes[j] = src->ParseInt();
	}

	// add the completed surface to the model
	model->AddSurface( surf );

	src->ExpectTokenString( "}" );

	// we do NOT do a model->FinishSurfaceces, because we don't need sil edges, planes, tangents, etc.
//	model->FinishSurfaces();

	return model;
}

/*
================
idRenderWorldLocal::SetupAreaRefs
================
*/
void idRenderWorldLocal::SetupAreaRefs() {
	int		i;

	connectedAreaNum = 0;
	for ( i = 0 ; i < numPortalAreas ; i++ ) {
		portalAreas[i].areaNum = i;
		portalAreas[i].lightRefs.areaNext =
		portalAreas[i].lightRefs.areaPrev =
			&portalAreas[i].lightRefs;
		portalAreas[i].entityRefs.areaNext =
		portalAreas[i].entityRefs.areaPrev =
			&portalAreas[i].entityRefs;
	}
}

/*
================
idRenderWorldLocal::ParseInterAreaPortals
================
*/
void idRenderWorldLocal::ParseInterAreaPortals( idLexer *src ) {
	int i, j;

	src->ExpectTokenString( "{" );

	numPortalAreas = src->ParseInt();
	if ( numPortalAreas < 0 ) {
		src->Error( "R_ParseInterAreaPortals: bad numPortalAreas" );
		return;
	}
	portalAreas = (portalArea_t *)R_ClearedStaticAlloc( numPortalAreas * sizeof( portalAreas[0] ) );
	areaScreenRect = (idScreenRect *) R_ClearedStaticAlloc( numPortalAreas * sizeof( idScreenRect ) );

	// set the doubly linked lists
	SetupAreaRefs();

	numInterAreaPortals = src->ParseInt();
	if ( numInterAreaPortals < 0 ) {
		src->Error(  "R_ParseInterAreaPortals: bad numInterAreaPortals" );
		return;
	}

	doublePortals = (doublePortal_t *)R_ClearedStaticAlloc( numInterAreaPortals *
		sizeof( doublePortals [0] ) );

	for ( i = 0 ; i < numInterAreaPortals ; i++ ) {
		int		numPoints, a1, a2;
		idWinding	*w;
		portal_t	*p;

		numPoints = src->ParseInt();
		a1 = src->ParseInt();
		a2 = src->ParseInt();

		w = new idWinding( numPoints );
		w->SetNumPoints( numPoints );
		for ( j = 0 ; j < numPoints ; j++ ) {
			src->Parse1DMatrix( 3, (*w)[j].ToFloatPtr() );
			// no texture coordinates
			(*w)[j][3] = 0;
			(*w)[j][4] = 0;
		}

		// add the portal to a1
		p = (portal_t *)R_ClearedStaticAlloc( sizeof( *p ) );
		p->intoArea = a2;
		p->doublePortal = &doublePortals[i];
		p->w = w;
		p->w->GetPlane( p->plane );

		p->next = portalAreas[a1].portals;
		portalAreas[a1].portals = p;

		doublePortals[i].portals[0] = p;

		// reverse it for a2
		p = (portal_t *)R_ClearedStaticAlloc( sizeof( *p ) );
		p->intoArea = a1;
		p->doublePortal = &doublePortals[i];
		p->w = w->Reverse();
		p->w->GetPlane( p->plane );

		p->next = portalAreas[a2].portals;
		portalAreas[a2].portals = p;

		doublePortals[i].portals[1] = p;
	}

	src->ExpectTokenString( "}" );
}

/*
================
idRenderWorldLocal::ParseNodes
================
*/
void idRenderWorldLocal::ParseNodes( idLexer *src ) {
	int			i;

	src->ExpectTokenString( "{" );

	numAreaNodes = src->ParseInt();
	if ( numAreaNodes < 0 ) {
		src->Error( "R_ParseNodes: bad numAreaNodes" );
	}
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc( numAreaNodes * sizeof( areaNodes[0] ) );

	for ( i = 0 ; i < numAreaNodes ; i++ ) {
		areaNode_t	*node;

		node = &areaNodes[i];

		src->Parse1DMatrix( 4, node->plane.ToFloatPtr() );
		node->children[0] = src->ParseInt();
		node->children[1] = src->ParseInt();
	}

	src->ExpectTokenString( "}" );
}

/*
================
idRenderWorldLocal::CommonChildrenArea_r
================
*/
int idRenderWorldLocal::CommonChildrenArea_r( areaNode_t *node ) {
	int	nums[2];

	for ( int i = 0 ; i < 2 ; i++ ) {
		if ( node->children[i] <= 0 ) {
			nums[i] = -1 - node->children[i];
		} else {
			nums[i] = CommonChildrenArea_r( &areaNodes[ node->children[i] ] );
		}
	}

	// solid nodes will match any area
	if ( nums[0] == AREANUM_SOLID ) {
		nums[0] = nums[1];
	}
	if ( nums[1] == AREANUM_SOLID ) {
		nums[1] = nums[0];
	}

	int	common;
	if ( nums[0] == nums[1] ) {
		common = nums[0];
	} else {
		common = CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	node->commonChildrenArea = common;

	return common;
}

/*
=================
idRenderWorldLocal::ClearWorld

Sets up for a single area world
=================
*/
void idRenderWorldLocal::ClearWorld() {
	numPortalAreas = 1;
	portalAreas = (portalArea_t *)R_ClearedStaticAlloc( sizeof( portalAreas[0] ) );
	areaScreenRect = (idScreenRect *) R_ClearedStaticAlloc( sizeof( idScreenRect ) );

	SetupAreaRefs();

	// even though we only have a single area, create a node
	// that has both children pointing at it so we don't need to
	//
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc( sizeof( areaNodes[0] ) );
	areaNodes[0].plane[3] = 1;
	areaNodes[0].children[0] = -1;
	areaNodes[0].children[1] = -1;
}

/*
=================
idRenderWorldLocal::FreeDefs

dump all the interactions
=================
*/
void idRenderWorldLocal::FreeDefs() {
	int		i;

	generateAllInteractionsCalled = false;

	if ( interactionTable ) {
		R_StaticFree( interactionTable );
		interactionTable = NULL;
	}

	// free all lightDefs
	for ( i = 0 ; i < lightDefs.Num() ; i++ ) {
		idRenderLightLocal	*light;

		light = lightDefs[i];
		if ( light && light->world == this ) {
			FreeLightDef( i );
			lightDefs[i] = NULL;
		}
	}

	// free all entityDefs
	for ( i = 0 ; i < entityDefs.Num() ; i++ ) {
		idRenderEntityLocal	*mod;

		mod = entityDefs[i];
		if ( mod && mod->world == this ) {
			FreeEntityDef( i );
			entityDefs[i] = NULL;
		}
	}
}

/*
=================
idRenderWorldLocal::InitFromMap

A NULL or empty name will make a world without a map model, which
is still useful for displaying a bare model
=================
*/
bool idRenderWorldLocal::InitFromMap( const char *name ) {
	// if this is an empty world, initialize manually
	if (!name || !name[0]) {
		FreeWorld();
		mapName.Clear();
		ClearWorld();
		return true;
	}

	if(!LoadProc(name))
		return false;

	return LoadOcl(name);
}

/*
=====================
idRenderWorldLocal::LoadProc
=====================
*/
bool idRenderWorldLocal::LoadProc(const char* name) {
	assert(name && name[0] && "name must not be empty at this point");

	// load it
	idStr filename = name;
	filename.SetFileExtension( PROC_FILE_EXT );

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp;
	fileSystem->ReadFile( filename, NULL, &currentTimeStamp );

	if (name == mapName) {
		if (currentTimeStamp != FILE_NOT_FOUND_TIMESTAMP && currentTimeStamp == mapTimeStamp) {
			common->Printf( "idRenderWorldLocal::LoadProc: retaining existing map\n" );
			FreeDefs();
			TouchWorldModels();
			AddWorldModelEntities();
			ClearPortalStates();
			return true;
		}
		common->Printf( "idRenderWorldLocal::LoadProc: timestamp has changed, reloading.\n" );
	}

	FreeWorld();

	idLexer src( filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
	if (!src.IsLoaded()) {
		common->Printf( "idRenderWorldLocal::LoadProc: %s not found\n", filename.c_str() );
		ClearWorld();
		return false;
	}

	mapName = name;
	mapTimeStamp = currentTimeStamp;

	// if we are writing a demo, archive the load command
	if (session->writeDemo) {
		WriteLoadMap();
	}

	idToken token;
	if (!src.ReadToken( &token ) || token.Icmp( PROC_FILE_ID )) {
		common->Printf( "idRenderWorldLocal::LoadProc: bad id '%s' instead of '%s'\n", token.c_str(), PROC_FILE_ID );
		return false;
	}

	// parse the file
	while (1) {
		if (!src.ReadToken( &token )) {
			break;
		}

		if (token == "model") {
			idRenderModel* lastModel = ParseModel( &src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		if (token == "shadowModel") {
			idRenderModel* lastModel = ParseShadowModel( &src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		if (token == "interAreaPortals") {
			ParseInterAreaPortals( &src );
			continue;
		}

		if (token == "nodes") {
			ParseNodes( &src );
			continue;
		}

		src.Error( "idRenderWorldLocal::LoadProc: bad token \"%s\"", token.c_str() );
	}

	// if it was a trivial map without any areas, create a single area
	if (!numPortalAreas) {
		ClearWorld();
	}

	// find the points where we can early-our of reference pushing into the BSP tree
	CommonChildrenArea_r( &areaNodes[0] );

	AddWorldModelEntities();
	ClearPortalStates();

	// done!
	return true;
}

/*
=====================
idRenderWorldLocal::LoadOcl
=====================
*/
bool idRenderWorldLocal::LoadOcl( const char* name ) {
	assert(name && name[0] && "name must not be empty at this point");

	// load it
	idStr filename = name;
	filename.SetFileExtension( OCL_FILE_EXT );

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp;
	fileSystem->ReadFile( filename, NULL, &currentTimeStamp );
/*
	if (name == mapName) {
		if (currentTimeStamp != FILE_NOT_FOUND_TIMESTAMP && currentTimeStamp == mapTimeStamp) {
			common->Printf( "idRenderWorldLocal::LoadOcl: retaining existing map\n" );
			return true;
		}
		common->Printf( "idRenderWorldLocal::LoadOcl: timestamp has changed, reloading.\n" );
	}
*/
	idLexer src( filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
	if (!src.IsLoaded()) {
		common->Warning( "idRenderWorldLocal::LoadOcl: %s not found\n", filename.c_str() );
		return true;
	}

	idToken token;
	if (!src.ReadToken( &token ) || token.Icmp( OCL_FILE_ID )) {
		common->Printf( "idRenderWorldLocal::LoadOcl: bad id '%s' instead of '%s'\n", token.c_str(), OCL_FILE_ID );
		return false;
	}

	// parse the file
	while (1) {
		if (!src.ReadToken( &token )) {
			break;
		}

		if (token == "occluder") {
			idRenderModel* lastModel = ParseOccluder( &src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		src.Error( "idRenderWorldLocal::LoadOcl: bad token \"%s\"", token.c_str() );
	}

	// done!
	return true;
}

/*
=====================
idRenderWorldLocal::ClearPortalStates
=====================
*/
void idRenderWorldLocal::ClearPortalStates() {
	int		i, j;

	// all portals start off open
	for ( i = 0 ; i < numInterAreaPortals ; i++ ) {
		doublePortals[i].blockingBits = PS_BLOCK_NONE;
	}

	// flood fill all area connections
	for ( i = 0 ; i < numPortalAreas ; i++ ) {
		for ( j = 0 ; j < NUM_PORTAL_ATTRIBUTES ; j++ ) {
			connectedAreaNum++;
			FloodConnectedAreas( &portalAreas[i], j );
		}
	}
}

/*
================
CombineModelSurfaces

Frees the model and returns a new model with all triangles combined
into one surface
================
*/
static idRenderModel *CombineModelSurfaces( idRenderModel *model, const char* name ) {
	int		totalVerts;
	int		totalIndexes;
	int		numIndexes;
	int		numVerts;
	int		i, j;

	totalVerts = 0;
	totalIndexes = 0;

	for (i = 0; i < model->NumSurfaces(); i++) {
		const modelSurface_t	*surf = model->Surface( i );

		totalVerts += surf->geometry->numVerts;
		totalIndexes += surf->geometry->numIndexes;
	}

	srfTriangles_t *newTri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( newTri, totalVerts );
	R_AllocStaticTriSurfIndexes( newTri, totalIndexes );

	newTri->numVerts = totalVerts;
	newTri->numIndexes = totalIndexes;

	newTri->bounds.Clear();

	idDrawVert *verts = newTri->verts;
	glIndex_t *indexes = newTri->indexes;
	numIndexes = 0;
	numVerts = 0;
	for (i = 0; i < model->NumSurfaces(); i++) {
		const modelSurface_t *surf = model->Surface( i );
		const srfTriangles_t *tri = surf->geometry;

		memcpy( verts + numVerts, tri->verts, tri->numVerts * sizeof(tri->verts[0]) );
		for (j = 0; j < tri->numIndexes; j++) {
			indexes[numIndexes + j] = numVerts + tri->indexes[j];
		}
		newTri->bounds.AddBounds( tri->bounds );
		numIndexes += tri->numIndexes;
		numVerts += tri->numVerts;
	}

	modelSurface_t surf;

	surf.id = 0;
	surf.geometry = newTri;
	surf.shader = tr.defaultMaterial;

	idRenderModel *newModel = renderModelManager->AllocModel();
	newModel->InitEmpty(name);
	newModel->AddSurface( surf );

	renderModelManager->FreeModel( model );

	return newModel;
}


/*
=====================
idRenderWorldLocal::AddWorldModelEntities
=====================
*/
void idRenderWorldLocal::AddWorldModelEntities() {
	// add the world model for each portal area
	// we can't just call AddEntityDef, because that would place the references
	// based on the bounding box, rather than explicitly into the correct area
	for ( int i = 0 ; i < numPortalAreas ; i++ ) {
		idRenderEntityLocal* def = new idRenderEntityLocal;

		// try and reuse a free spot
		int index = entityDefs.FindNull();
		if ( index == -1 ) {
			index = entityDefs.Append(def);
		} else {
			entityDefs[index] = def;
		}

		def->index = index;
		def->world = this;

		def->parms.hModel = renderModelManager->FindModel( va("_area%i", i ) );
		if ( def->parms.hModel->IsDefaultModel() || !def->parms.hModel->IsStaticWorldModel() ) {
			common->Error( "idRenderWorldLocal::InitFromMap: bad area model lookup" );
		}

		idRenderModel *hModel = def->parms.hModel;

		//helper model to merge all static and fully opaque world surfaces,
		//so depth information for shadow maps for those surfaces can be rendered
		//with just one draw call
		idRenderModel* staticOccluderModel = renderModelManager->AllocModel();

		for ( int j = 0; j < hModel->NumSurfaces(); j++ ) {
			const modelSurface_t *surf = hModel->Surface( j );

			if ( surf->shader->GetName() == idStr( "textures/smf/portal_sky" ) ) {
				def->needsPortalSky = true;
			}

			if ( surf->geometry->numVerts <= 0 ) {
				continue;
			}

			//if surface cast a shadow and is fully opaque, add a copy of it the staticOccluderModel helper model
			if(surf->shader->SurfaceCastsShadow() && surf->shader->Coverage() == MC_OPAQUE) {
				modelSurface_s newSurf;
				newSurf.shader = declManager->FindMaterial("_default");
				newSurf.id = 0;

				newSurf.geometry = R_AllocStaticTriSurf();
				newSurf.geometry->numVerts = surf->geometry->numVerts;
				newSurf.geometry->numIndexes = surf->geometry->numIndexes;

				R_AllocStaticTriSurfVerts( newSurf.geometry, newSurf.geometry->numVerts );
				R_AllocStaticTriSurfIndexes( newSurf.geometry, newSurf.geometry->numIndexes );

				memcpy(newSurf.geometry->indexes, surf->geometry->indexes, sizeof(surf->geometry->indexes[0]) * newSurf.geometry->numIndexes);
				memcpy(newSurf.geometry->verts, surf->geometry->verts, sizeof(surf->geometry->verts[0]) * newSurf.geometry->numVerts);

				staticOccluderModel->AddSurface(newSurf);
			}
		}

		//combine all surfaces of helper model to one single surface and use that
		staticOccluderModel = CombineModelSurfaces(staticOccluderModel, va("_area_%d_occluders", i));

		if(staticOccluderModel->NumSurfaces() > 0 && staticOccluderModel->Surface(0)->geometry->numVerts > 0) {
			// add it to the model manager list
			renderModelManager->AddModel( staticOccluderModel );
			// save it in the list to free when clearing this map
			localModels.Append( staticOccluderModel );
			//use as static occluder model
			def->staticOccluderModel = staticOccluderModel;
		} else {
			//static occluder model is empty, no need to keep it.
			renderModelManager->FreeModel(staticOccluderModel);
			def->staticOccluderModel = nullptr;
		}


		def->referenceBounds = def->parms.hModel->Bounds();

		def->parms.axis[0][0] = 1;
		def->parms.axis[1][1] = 1;
		def->parms.axis[2][2] = 1;

		R_AxisToModelMatrix( def->parms.axis, def->parms.origin, def->modelMatrix );

		// in case an explicit shader is used on the world, we don't
		// want it to have a 0 alpha or color
		def->parms.shaderParms[0] =
		def->parms.shaderParms[1] =
		def->parms.shaderParms[2] =
		def->parms.shaderParms[3] = 1;

		AddEntityRefToArea( def, &portalAreas[i] );
	}
}

/*
=====================
CheckAreaForPortalSky
=====================
*/
bool idRenderWorldLocal::CheckAreaForPortalSky( int areaNum ) {
	areaReference_t	*ref;

	assert( areaNum >= 0 && areaNum < numPortalAreas );

	for ( ref = portalAreas[areaNum].entityRefs.areaNext; ref->entity; ref = ref->areaNext ) {
		assert( ref->area == &portalAreas[areaNum] );

		if ( ref->entity && ref->entity->needsPortalSky ) {
			return true;
		}
	}

	return false;
}
