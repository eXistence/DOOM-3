/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2016 Johannes Ohlemacher

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
#include "GLDrawable.h"
#include "../../renderer/tr_local.h"
#include "../../renderer/ImmediateMode.h"

extern bool Sys_KeyDown( int key );

static int viewAngle = -98;

#ifdef __linux__
//FIXME(johl): currently only implemented on Windows,, change interface of GLDrawable to get rid of these?
#define VK_MENU 0
#define VK_SHIFT 0

bool Sys_KeyDown( int ) {
	return false;
}
#endif

idGLDrawable::idGLDrawable() {
	scale = 1.0;
	xOffset = 0.0;
	yOffset = 0.0;
	handleMove = false;
	realTime = 0;

}

void idGLDrawable::buttonDown( MouseButton _button, float x, float y ) {
	pressX = x;
	pressY = y;
	button = _button;
	if (button == MouseButton::Right) {
		handleMove = true;
	}
}

void idGLDrawable::buttonUp( MouseButton button, float x, float y ) {
	handleMove = false;
}

void idGLDrawable::mouseMove( float x, float y ) {
	if (!handleMove) {
		return;
	}
	
	if (Sys_KeyDown( VK_MENU )) {
		// scale
		float *px = &x;
		float *px2 = &pressX;

		if (idMath::Diff(y, pressY) > idMath::Diff(x, pressX)) {
			px = &y;
			px2 = &pressY;
		}

		if (*px > *px2) {
			// zoom in
			scale += 0.1f;
			if (scale > 10.0f) {
				scale = 10.0f;
			}
		}
		else if (*px < *px2) {
			// zoom out
			scale -= 0.1f;
			if (scale <= 0.001f) {
				scale = 0.001f;
			}
		}

		*px2 = *px;
		//::SetCursorPos(pressX, pressY);

	}
	else if (Sys_KeyDown( VK_SHIFT )) {
		// rotate
	}
	else {
		// origin
		if (x != pressX) {
			xOffset += (x - pressX);
			pressX = x;
		}
		if (y != pressY) {
			yOffset -= (y - pressY);
			pressY = y;
		}
		//::SetCursorPos(pressX, pressY);
	}

	Update();
}

void idGLDrawable::draw( int x, int y, int w, int h ) {
	GL_State( GLS_DEFAULT );
	glViewport( x, y, w, h );
	glScissor( x, y, w, h );

	float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	glClearBufferfv( GL_COLOR, 0, clearColor );

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	globalImages->BindNull();
	fhImmediateMode im;
	im.Color3f( 1, 1, 1 );

	im.Begin( GL_LINE_LOOP );
	im.Color3f( 1, 0, 0 );
	im.Vertex2f( x + 3, y + 3 );
	im.Color3f( 0, 1, 0 );
	im.Vertex2f( x + 3, h - 3 );
	im.Color3f( 0, 0, 1 );
	im.Vertex2f( w - 3, h - 3 );
	im.Color3f( 1, 1, 1 );
	im.Vertex2f( w - 3, y + 3 );
	im.End();
}

idGLDrawableWorld::idGLDrawableWorld() {
	world = NULL;
	worldModel = NULL;
	InitWorld();
}

idGLDrawableWorld::~idGLDrawableWorld() {
	delete world;
}

void idGLDrawableWorld::AddTris( srfTriangles_t *tris, const idMaterial *mat ) {
	modelSurface_t	surf;
	surf.geometry = tris;
	surf.shader = mat;
	worldModel->AddSurface( surf );
}

void idGLDrawableWorld::draw( int x, int y, int w, int h ) {

}

void idGLDrawableWorld::InitWorld() {
	if (world == NULL) {
		world = renderSystem->AllocRenderWorld();
	}
	if (worldModel == NULL) {
		worldModel = renderModelManager->AllocModel();
	}
	world->InitFromMap( NULL );
	worldModel->InitEmpty( va( "GLWorldModel_%i", Sys_Milliseconds() ) );
}

void idGLDrawableWorld::InitLight( const idVec3& position ) {
	renderLight_t	parms;
	idDict spawnArgs;
	spawnArgs.Set( "classname", "light" );
	spawnArgs.Set( "name", "light_1" );
	spawnArgs.SetVector( "origin", position );
	idStr str;
	sprintf( str, "%f %f %f", light, light, light );
	spawnArgs.Set( "_color", str );
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &parms );
	lightDef = world->AddLightDef( &parms );
}

void idGLDrawableWorld::mouseMove( float x, float y ) {
	if (handleMove) {
		Update();
		if (button == MouseButton::Left) {
			float cury = (float)(2 * x - rect.z) / rect.z;
			float curx = (float)(2 * y - rect.w) / rect.w;
			idVec3 to( -curx, -cury, 0.0f );
			to.ProjectSelfOntoSphere( radius );
			lastPress.ProjectSelfOntoSphere( radius );
			idVec3 axis;
			axis.Cross( to, lastPress );
			float len = (lastPress - to).Length() / (2.0f * radius);
			len = idMath::ClampFloat( -1.0f, 1.0f, len );
			float phi = 2.0f * asin( len );

			axis.Normalize();
			axis *= sin( phi / 2.0f );
			idQuat rot( axis.z, axis.y, axis.x, cos( phi / 2.0f ) );
			rot.Normalize();

			rotation *= rot;
			rotation.Normalize();

			lastPress = to;
			lastPress.z = 0.0f;
		}
		else {
			bool doScale = Sys_KeyDown( VK_MENU );
			bool doLight = Sys_KeyDown( VK_SHIFT );
			if (doLight) {
				// scale
				float *px = &x;
				float *px2 = &pressX;

				if (idMath::Diff(y, pressY) > idMath::Diff(x, pressX)) {
					px = &y;
					px2 = &pressY;
				}

				if (*px > *px2) {
					light += 0.05f;
					if (light > 2.0f) {
						light = 2.0f;
					}
				}
				else if (*px < *px2) {
					light -= 0.05f;
					if (light < 0.0f) {
						light = 0.0f;
					}
				}
				*px2 = *px;
				//::SetCursorPos( pressX, pressY );
			}
			else {
				// origin
				if (x != pressX) {
					if (doScale) {
						zOffset += (x - pressX);
						scale = Min( 10.0f, scale + (x - pressX)*0.01f );
					}
					else {
						xOffset += (x - pressX);
					}
					pressX = x;
				}
				if (y != pressY) {
					if (doScale) {
						zOffset -= (y - pressY);
						scale = Max( 0.001f, scale - (y - pressY)*0.01f );
					}
					else {
						yOffset -= (y - pressY);
					}
					pressY = y;
				}
				//::SetCursorPos(pressX, pressY);
			}
		}
	}
}
void idGLDrawableWorld::buttonDown( MouseButton button, float x, float y ) {
	pressX = x;
	pressY = y;

	lastPress.y = -(float)(2 * x - rect.z) / rect.z;
	lastPress.x = -(float)(2 * y - rect.w) / rect.w;
	lastPress.z = 0.0f;
	this->button = button;
	if (button == MouseButton::Right || button == MouseButton::Left) {
		handleMove = true;
	}
}

void idGLDrawableMaterial::draw( int x, int y, int w, int h ) {
	const idMaterial *mat = material;
	if (mat) {
		glViewport( x, y, w, h );
		glScissor( x, y, w, h );

		float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
		glClearBufferfv( GL_COLOR, 0, clearColor );

		if (worldDirty) {
			InitWorld();
			InitLight( idVec3( 0, 0, 0 ) );

			idImage *img = (mat->GetNumStages() > 0) ? mat->GetStage( 0 )->texture.image : mat->GetEditorImage();

			if (img == NULL) {
				common->Warning( "Unable to load image for preview for %s", mat->GetName() );
				return;
			}

			int width = img->uploadWidth;
			int height = img->uploadHeight;

			width *= scale;
			height *= scale;

			srfTriangles_t *tris = worldModel->AllocSurfaceTriangles( 4, 6 );
			tris->numVerts = 4;
			tris->numIndexes = 6;

			tris->indexes[0] = 0;
			tris->indexes[1] = 1;
			tris->indexes[2] = 2;
			tris->indexes[3] = 3;
			tris->indexes[4] = 1;
			tris->indexes[5] = 0;

			tris->verts[0].xyz.x = 64;
			tris->verts[0].xyz.y = -xOffset + 0 - width / 2;
			tris->verts[0].xyz.z = yOffset + 0 - height / 2;
			tris->verts[0].st.x = 1;
			tris->verts[0].st.y = 1;

			tris->verts[1].xyz.x = 64;
			tris->verts[1].xyz.y = -xOffset + width / 2;
			tris->verts[1].xyz.z = yOffset + height / 2;
			tris->verts[1].st.x = 0;
			tris->verts[1].st.y = 0;

			tris->verts[2].xyz.x = 64;
			tris->verts[2].xyz.y = -xOffset + 0 - width / 2;
			tris->verts[2].xyz.z = yOffset + height / 2;
			tris->verts[2].st.x = 1;
			tris->verts[2].st.y = 0;

			tris->verts[3].xyz.x = 64;
			tris->verts[3].xyz.y = -xOffset + width / 2;
			tris->verts[3].xyz.z = yOffset + 0 - height / 2;
			tris->verts[3].st.x = 0;
			tris->verts[3].st.y = 1;

			tris->verts[0].normal = tris->verts[1].xyz.Cross( tris->verts[3].xyz );
			tris->verts[1].normal = tris->verts[2].normal = tris->verts[3].normal = tris->verts[0].normal;
			AddTris( tris, mat );

			worldModel->FinishSurfaces();

			renderEntity_t worldEntity;

			memset( &worldEntity, 0, sizeof( worldEntity ) );
			if (mat->HasGui()) {
				worldEntity.gui[0] = mat->GlobalGui();
			}
			worldEntity.hModel = worldModel;
			worldEntity.axis = mat3_default;
			worldEntity.shaderParms[0] = 1;
			worldEntity.shaderParms[1] = 1;
			worldEntity.shaderParms[2] = 1;
			worldEntity.shaderParms[3] = 1;
			modelDef = world->AddEntityDef( &worldEntity );

			worldDirty = false;
		}

		renderView_t	refdef;
		// render it
		renderSystem->BeginFrame( w, h );
		memset( &refdef, 0, sizeof( refdef ) );
		refdef.vieworg.Set( viewAngle, 0, 0 );

		refdef.viewaxis = idAngles( 0, 0, 0 ).ToMat3();
		refdef.shaderParms[0] = 1;
		refdef.shaderParms[1] = 1;
		refdef.shaderParms[2] = 1;
		refdef.shaderParms[3] = 1;

		refdef.width = SCREEN_WIDTH;
		refdef.height = SCREEN_HEIGHT;
		refdef.fov_x = 90;
		refdef.fov_y = 2 * atan( (float)h / w ) * idMath::M_RAD2DEG;

		refdef.time = eventLoop->Milliseconds();

		world->RenderScene( &refdef );

		renderSystem->EndFrame();

		GL_ModelViewMatrix.LoadIdentity();
	}

}

void idGLDrawableMaterial::setMedia( const char *name ) {
	idImage *img = NULL;
	if (name && *name) {
		material = declManager->FindMaterial( name );
		if (material) {
			const shaderStage_t *stage = (material->GetNumStages() > 0) ? material->GetStage( 0 ) : NULL;
			if (stage) {
				img = stage->texture.image;
			}
			else {
				img = material->GetEditorImage();
			}
		}
	}
	else {
		material = NULL;
	}
	// set scale to get a good fit

	if (material && img) {

		float size = (img->uploadWidth > img->uploadHeight) ? img->uploadWidth : img->uploadHeight;
		// use 128 as base scale of 1.0
		scale = 128.0 / size;
	}
	else {
		scale = 1.0;
	}
	xOffset = 0.0;
	yOffset = 0.0;
	worldDirty = true;
}

idGLDrawableModel::idGLDrawableModel( const char *name ) {
	worldModel = renderModelManager->FindModel( name );
	light = 1.0;
	worldDirty = true;
}

idGLDrawableModel::idGLDrawableModel() {
	worldModel = renderModelManager->DefaultModel();
	light = 1.0;
}

void idGLDrawableModel::setMedia( const char *name ) {
	worldModel = renderModelManager->FindModel( name );
	worldDirty = true;
	xOffset = 0.0;
	yOffset = 0.0;
	zOffset = -128;
	rotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	radius = 2.6f;
	lastPress.Zero();
}

void idGLDrawableModel::SetSkin( const char *skin ) {
	skinStr = skin;
}

void idGLDrawableModel::draw( int x, int y, int w, int h ) {
	if (!worldModel) {
		return;
	}
	if (worldModel->IsDynamicModel() != DM_STATIC) {
		//return;
	}

	rect.Set( x, y, w, h );

	glViewport( x, y, w, h );
	glScissor( x, y, w, h );

	float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	glClearBufferfv( GL_COLOR, 0, clearColor );

	if (worldDirty) {
		//InitWorld();
		world->InitFromMap( NULL );
		InitLight( idVec3( -128, 0, 0 ) );

		idDict spawnArgs;
		renderEntity_t worldEntity;
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		spawnArgs.Clear();
		spawnArgs.Set( "classname", "func_static" );
		spawnArgs.Set( "name", spawnArgs.GetString( "model" ) );
		spawnArgs.Set( "origin", "0 0 0" );
		if (skinStr.Length()) {
			spawnArgs.Set( "skin", skinStr );
		}
		gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &worldEntity );
		worldEntity.hModel = worldModel;

		worldEntity.axis = rotation.ToMat3();

		worldEntity.shaderParms[0] = 1;
		worldEntity.shaderParms[1] = 1;
		worldEntity.shaderParms[2] = 1;
		worldEntity.shaderParms[3] = 1;
		modelDef = world->AddEntityDef( &worldEntity );

		worldDirty = false;
	}

	renderView_t	refdef;
	// render it
	renderSystem->BeginFrame( w, h );
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg.Set( zOffset, xOffset, -yOffset );

	refdef.viewaxis = idAngles( 0, 0, 0 ).ToMat3();
	refdef.shaderParms[0] = 1;
	refdef.shaderParms[1] = 1;
	refdef.shaderParms[2] = 1;
	refdef.shaderParms[3] = 1;

	refdef.width = SCREEN_WIDTH;
	refdef.height = SCREEN_HEIGHT;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan( (float)h / w ) * idMath::M_RAD2DEG;

	refdef.time = eventLoop->Milliseconds();

	world->RenderScene( &refdef );

	renderSystem->EndFrame();

	GL_ModelViewMatrix.LoadIdentity();
}



