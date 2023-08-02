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
#include "ImmediateMode.h"
#include "RenderProgram.h"
#include "Framebuffer.h"

frameData_t		*frameData;
backEndState_t	backEnd;

static idCVar r_fxaa("r_fxaa", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "");
static idCVar r_bloomThreshold("r_bloomThreshold", "0.9", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "");
static idCVar r_bloomBlurPasses("r_bloomBlurPasses", "4", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "");
static idCVar r_bloomFactor("r_bloomFactor", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "");
static idCVar r_hdrPixelFormat("r_hdrPixelFormat", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "");

static const unsigned vertexLayoutAttributes[] = {
	//None:
	0,
	//Shadow:
	(1 << fhRenderProgram::vertex_attrib_position_shadow),
	//ShadowSilhouette:
	(1 << fhRenderProgram::vertex_attrib_position),
	//Simple
	(1 << fhRenderProgram::vertex_attrib_position)
	| (1 << fhRenderProgram::vertex_attrib_texcoord)
	| (1 << fhRenderProgram::vertex_attrib_color),
	//Draw
	(1 << fhRenderProgram::vertex_attrib_position)
	| (1 << fhRenderProgram::vertex_attrib_texcoord)
	| (1 << fhRenderProgram::vertex_attrib_normal)
	| (1 << fhRenderProgram::vertex_attrib_color)
	| (1 << fhRenderProgram::vertex_attrib_binormal)
	| (1 << fhRenderProgram::vertex_attrib_tangent),
	//DrawPosOnly
	(1 << fhRenderProgram::vertex_attrib_position),
	//DrawPosTexOnly
	(1 << fhRenderProgram::vertex_attrib_position)
	| (1 << fhRenderProgram::vertex_attrib_texcoord),
	//DrawPosColorOnly
	(1 << fhRenderProgram::vertex_attrib_position)
	| (1 << fhRenderProgram::vertex_attrib_color),
	//DrawPosColorTexOnly
	(1 << fhRenderProgram::vertex_attrib_position)
	| (1 << fhRenderProgram::vertex_attrib_texcoord)
	| (1 << fhRenderProgram::vertex_attrib_color)
};
static_assert(sizeof( vertexLayoutAttributes ) / sizeof( vertexLayoutAttributes[0] ) == (size_t)fhVertexLayout::COUNT, "");

static fhVertexLayout currentVertexLayout = fhVertexLayout::None;

/*
======================
RB_SetDefaultGLState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
======================
*/
void RB_SetDefaultGLState(void) {
	currentVertexLayout = fhVertexLayout::None;
	RB_LogComment("--- R_SetDefaultGLState ---\n");

	//
	// make sure our GL state vector is set correctly
	//
	memset(&backEnd.glState, 0, sizeof(backEnd.glState));
	backEnd.glState.forceGlState = true;

	glColorMask(1, 1, 1, 1);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_ALWAYS);

	glCullFace(GL_FRONT_AND_BACK);

	if (r_useScissor.GetBool()) {
		auto fb = fhFramebuffer::GetCurrentDrawBuffer();
		glScissor( 0, 0, fb->GetWidth(), fb->GetHeight() );
	}
}


/*
====================
RB_LogComment
====================
*/
void RB_LogComment( const char *comment, ... ) {
#if 0
  va_list marker;

  if ( !tr.logFile ) {
    return;
  }

  fprintf( tr.logFile, "// " );
  va_start( marker, comment );
  vfprintf( tr.logFile, comment, marker );
  va_end( marker );
#endif
}

//=============================================================================

/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( int cullType ) {
	if ( backEnd.glState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) {
		glDisable( GL_CULL_FACE );
	} else  {
		if ( backEnd.glState.faceCulling == CT_TWO_SIDED ) {
			glEnable( GL_CULL_FACE );
		}

		if ( cullType == CT_BACK_SIDED ) {
			if ( backEnd.viewDef->isMirror ) {
				glCullFace( GL_FRONT );
			} else {
				glCullFace( GL_BACK );
			}
		} else {
			if ( backEnd.viewDef->isMirror ) {
				glCullFace( GL_BACK );
			} else {
				glCullFace( GL_FRONT );
			}
		}
	}

	backEnd.glState.faceCulling = cullType;
}

/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State( int stateBits ) {
	int	diff;

	if ( !r_useStateCaching.GetBool() || backEnd.glState.forceGlState ) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = -1;
		backEnd.glState.forceGlState = false;
	} else {
		diff = stateBits ^ backEnd.glState.glStateBits;
		if ( !diff ) {
			return;
		}
	}

	//
	// check depthFunc bits
	//
	if ( diff & ( GLS_DEPTHFUNC_EQUAL | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_ALWAYS ) ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			glDepthFunc( GL_EQUAL );
		} else if ( stateBits & GLS_DEPTHFUNC_ALWAYS ) {
			glDepthFunc( GL_ALWAYS );
		} else {
			glDepthFunc( GL_LEQUAL );
		}
	}


	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor, dstFactor;

		switch ( stateBits & GLS_SRCBLEND_BITS ) {
		case GLS_SRCBLEND_ZERO:
			srcFactor = GL_ZERO;
			break;
		case GLS_SRCBLEND_ONE:
			srcFactor = GL_ONE;
			break;
		case GLS_SRCBLEND_DST_COLOR:
			srcFactor = GL_DST_COLOR;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			srcFactor = GL_ONE_MINUS_DST_COLOR;
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			srcFactor = GL_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			srcFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			srcFactor = GL_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			srcFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			srcFactor = GL_SRC_ALPHA_SATURATE;
			break;
		default:
			srcFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid src blend state bits\n" );
			break;
		}

		switch ( stateBits & GLS_DSTBLEND_BITS ) {
		case GLS_DSTBLEND_ZERO:
			dstFactor = GL_ZERO;
			break;
		case GLS_DSTBLEND_ONE:
			dstFactor = GL_ONE;
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			dstFactor = GL_SRC_COLOR;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			dstFactor = GL_ONE_MINUS_SRC_COLOR;
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			dstFactor = GL_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			dstFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			dstFactor = GL_DST_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			dstFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		default:
			dstFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid dst blend state bits\n" );
			break;
		}

		glBlendFunc( srcFactor, dstFactor );
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK ) {
		if ( stateBits & GLS_DEPTHMASK ) {
			glDepthMask( GL_FALSE );
		} else {
			glDepthMask( GL_TRUE );
		}
	}

	//
	// check colormask
	//
	if ( diff & (GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK|GLS_ALPHAMASK) ) {
		GLboolean		r, g, b, a;
		r = ( stateBits & GLS_REDMASK ) ? 0 : 1;
		g = ( stateBits & GLS_GREENMASK ) ? 0 : 1;
		b = ( stateBits & GLS_BLUEMASK ) ? 0 : 1;
		a = ( stateBits & GLS_ALPHAMASK ) ? 0 : 1;
		glColorMask( r, g, b, a );
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	backEnd.glState.glStateBits = stateBits;
}

void  GL_UseProgram( const fhRenderProgram* program ) {
	if (backEnd.currentProgram == program) {
		return;
	}

	if(program) {
		program->Bind();
	}
	else {
		fhRenderProgram::Unbind();
	}

	backEnd.currentProgram = program;
}

void GL_SetVertexLayout( fhVertexLayout layout ) {
	if(currentVertexLayout == layout || layout == fhVertexLayout::None) {
		return;
	}

	const unsigned current = vertexLayoutAttributes[(int)currentVertexLayout];
	const unsigned target = vertexLayoutAttributes[(int)layout];

	for (unsigned i = 0; i < 7 ; ++i) {
		const unsigned bit = (1 << i);
		const bool isEnabled = (current & bit) != 0;
		const bool shouldBeEnabled = (target & bit) != 0;

		if (shouldBeEnabled && !isEnabled) {
			glEnableVertexAttribArray(i);
		}
		else if (!shouldBeEnabled && isEnabled) {
			glDisableVertexAttribArray(i);
		}
	}

	currentVertexLayout = layout;
}

template<typename T>
static const void* attributeOffset( T offset, const void* attributeOffset )
{
	return reinterpret_cast<const void*>((std::ptrdiff_t)offset + (std::ptrdiff_t)attributeOffset);
}

template<typename T>
static const void* attributeOffset( T offset, int attributeOffset )
{
	return reinterpret_cast<const void*>((std::ptrdiff_t)offset + (std::ptrdiff_t)attributeOffset);
}

void GL_SetupVertexAttributes( fhVertexLayout layout, int offset ) {
	GL_SetVertexLayout( layout );

	switch(currentVertexLayout) {
	case fhVertexLayout::None:
		break;
	case fhVertexLayout::Shadow:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position_shadow, 4, GL_FLOAT, false, sizeof(shadowCache_t), attributeOffset(offset, 0) );
		break;
	case fhVertexLayout::ShadowSilhouette:
		glVertexAttribPointer(fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(shadowCache_t), attributeOffset(offset, 0));
		break;
	case fhVertexLayout::Simple:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(fhSimpleVert), attributeOffset( offset, fhSimpleVert::xyzOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_texcoord, 2, GL_FLOAT, false, sizeof(fhSimpleVert), attributeOffset( offset, fhSimpleVert::texcoordOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_color, 4, GL_UNSIGNED_BYTE, false, sizeof(fhSimpleVert), attributeOffset( offset, fhSimpleVert::colorOffset ) );
		break;
	case fhVertexLayout::DrawPosOnly:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::xyzOffset) );
		break;
	case fhVertexLayout::DrawPosColorOnly:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::xyzOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_color, 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::colorOffset ) );
		break;
	case fhVertexLayout::DrawPosColorTexOnly:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::xyzOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_color, 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::colorOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_texcoord, 2, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::texcoordOffset ) );
		break;
	case fhVertexLayout::DrawPosTexOnly:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::xyzOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_texcoord, 2, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::texcoordOffset ) );
		break;
	case fhVertexLayout::Draw:
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_position, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::xyzOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_texcoord, 2, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::texcoordOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_normal, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::normalOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_color, 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::colorOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_binormal, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::binormalOffset ) );
		glVertexAttribPointer( fhRenderProgram::vertex_attrib_tangent, 3, GL_FLOAT, false, sizeof(idDrawVert), attributeOffset( offset, idDrawVert::tangentOffset ) );
		break;
	case fhVertexLayout::COUNT:
	default:
		assert( false && "invalid vertex layout" );
		break;
	}
}

joGLMatrixStack::joGLMatrixStack(int mode) : matrixmode(mode), size(0) {
  LoadIdentity();
}

void joGLMatrixStack::Load(const float* m) {
  memcpy(Data(size), m, sizeof(Matrix));
}

void joGLMatrixStack::LoadIdentity() {
  static const float identity [16] =
  { 1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1 };

  Load(&identity[0]);
}

void joGLMatrixStack::Push() {
  assert(size + 1 < max_stack_size);

  memcpy(&stack[size+1], &stack[size], sizeof(stack[0]));
  size++;
}

void joGLMatrixStack::Pop() {
  if(size > 0) {
    size--;
  }
}

void joGLMatrixStack::Ortho(float left, float right, float bottom, float top, float nearClip, float farClip) {
  const float a = 2.0f/(right - left);
  const float b = 2.0f/(top - bottom);
  const float c = -2.0f/(farClip - nearClip);
  const float d = -((right + left)/(right - left));
  const float e = -((top + bottom)/(top - bottom));
  const float f = -((farClip + nearClip)/(farClip - nearClip));

  const float m[] = {
    a, 0, 0, 0,
    0, b, 0, 0,
    0, 0, c, 0,
    d, e, f, 1
  };

  Load(&m[0]);
}

void joGLMatrixStack::Rotate(float angle, float x, float y, float z) {
  const float mag = sqrtf(x * x + y * y + z * z);

  if (mag > 0.0f)
  {
    const float sinAngle = sinf(DEG2RAD(angle));
    const float cosAngle = cosf(DEG2RAD(angle));

    x /= mag;
    y /= mag;
    z /= mag;

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float yz = y * z;
    const float zx = z * x;
    const float xs = x * sinAngle;
    const float ys = y * sinAngle;
    const float zs = z * sinAngle;
    const float oneMinusCos = 1.0f - cosAngle;

    float rotMat[4][4];

    rotMat[0][0] = (oneMinusCos * xx) + cosAngle;
    rotMat[1][0] = (oneMinusCos * xy) - zs;
    rotMat[2][0] = (oneMinusCos * zx) + ys;
    rotMat[3][0] = 0.0F;

    rotMat[0][1] = (oneMinusCos * xy) + zs;
    rotMat[1][1] = (oneMinusCos * yy) + cosAngle;
    rotMat[2][1] = (oneMinusCos * yz) - xs;
    rotMat[3][1] = 0.0F;

    rotMat[0][2] = (oneMinusCos * zx) - ys;
    rotMat[1][2] = (oneMinusCos * yz) + xs;
    rotMat[2][2] = (oneMinusCos * zz) + cosAngle;
    rotMat[3][2] = 0.0F;

    rotMat[0][3] = 0.0F;
    rotMat[1][3] = 0.0F;
    rotMat[2][3] = 0.0F;
    rotMat[3][3] = 1.0F;

    float current[16];
    Get(&current[0]);

    float result[16];
    myGlMultMatrix(&rotMat[0][0], &current[0], &result[0]);

    Load(&result[0]);
  }
}

void joGLMatrixStack::Translate(float x, float y, float z) {
  const float translate[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    x, y, z, 1
  };

  float current[16];
  Get(&current[0]);

  float result[16];
  myGlMultMatrix(&translate[0], &current[0], &result[0]);

  Load(&result[0]);
}

void joGLMatrixStack::Get(float* dst) const {
  memcpy(dst, Data(size), sizeof(Matrix));
}

float* joGLMatrixStack::Data(int StackIndex) {
  return &stack[StackIndex].m[0];
}

const float* joGLMatrixStack::Data(int StackIndex) const {
  return &stack[StackIndex].m[0];
}

joGLMatrixStack GL_ProjectionMatrix(GL_PROJECTION);
joGLMatrixStack GL_ModelViewMatrix(GL_MODELVIEW);

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RB_SetGL2D

This is not used by the normal game paths, just by some tools
=============
*/
void RB_SetGL2D( void ) {
	// set 2D virtual screen size
	auto fb = fhFramebuffer::GetCurrentDrawBuffer();

	glViewport( 0, 0, fb->GetWidth(), fb->GetHeight() );
	if ( r_useScissor.GetBool() ) {
		glScissor( 0, 0, fb->GetWidth(), fb->GetHeight() );
	}

	GL_ProjectionMatrix.LoadIdentity();
	GL_ProjectionMatrix.Ortho( 0, 640, 480, 0, 0, 1 ); // always assume 640x480 virtual coordinates

	GL_ModelViewMatrix.LoadIdentity();

	GL_State( GLS_DEPTHFUNC_ALWAYS |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_STENCIL_TEST );
}



/*
=============
RB_SetBuffer

=============
*/
static void	RB_SetBuffer( const void *data ) {
	// see which draw buffer we want to render the frame to

	auto cmd = (const setBufferCommand_t *)data;

	backEnd.frameCount = cmd->frameCount;

	if (r_useFramebuffer.GetBool()) {
		if (r_mode.IsModified()) {
			int w, h, a;
			if (R_GetModeInfo(w, h, a, r_mode.GetInteger())) {
				glConfig.vidWidth = w;
				glConfig.vidHeight = h;
			}

			r_mode.ClearModified();
		}

		const float scale = r_framebufferScale.GetFloat();
		const int samples = Min( r_multiSamples.GetInteger(), glConfig.maxSamples );
		const int renderWidth = glConfig.vidWidth * scale;
		const int renderHeight = glConfig.vidHeight * scale;

		pixelFormat_t colorFormat;
		switch (r_hdrPixelFormat.GetInteger()) {
		case 1:
			colorFormat = pixelFormat_t::RGBA_32F;
			break;
		default:
			colorFormat = pixelFormat_t::RGBA;
			break;
		}

		fhFramebuffer::renderFramebuffer->Resize(renderWidth, renderHeight, samples, colorFormat, fhFramebuffer::renderFramebuffer->GetDepthFormat());
		fhFramebuffer::currentRenderFramebuffer->Resize(renderWidth, renderHeight, 1, colorFormat, fhFramebuffer::currentRenderFramebuffer->GetDepthFormat());
		fhFramebuffer::currentRenderFramebuffer2->Resize(renderWidth, renderHeight, 1, colorFormat, fhFramebuffer::currentRenderFramebuffer2->GetDepthFormat());
		fhFramebuffer::renderFramebuffer->Bind();

		glViewport(0, 0, renderWidth, renderHeight);
		glScissor(0, 0, renderWidth, renderHeight);
	}

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if ( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool() ) {
		float c[4];
		if ( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 ) {
			c[3] = 1.0f;
		} else if ( r_clear.GetInteger() == 2 ) {
			c[0] = 0.0f;
			c[1] = 0.0f;
			c[2] = 0.0f;
			c[3] = 1.0f;
		} else if ( r_showOverDraw.GetBool() ) {
			c[0] = 1.0f;
			c[1] = 1.0f;
			c[2] = 1.0f;
			c[3] = 1.0f;
		} else {
			c[0] = 0.4f;
			c[1] = 0.0f;
			c[2] = 0.25f;
			c[3] = 1.0f;
		}

		glClearBufferfv( GL_COLOR, 0, c );
	}
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages( void ) {
	int		i;
	idImage	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	glFinish();

	start = Sys_Milliseconds();

	for ( i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[i];

		if ( image->texnum == idImage::TEXTURE_NOT_LOADED && image->partialImage == NULL ) {
			continue;
		}

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages.GetInteger() == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		fhImmediateMode im;
		im.Color3f(1,1,1);
		im.SetTexture(image);
		im.Begin (GL_QUADS);
		im.TexCoord2f( 0, 0 );
		im.Vertex2f( x, y );
		im.TexCoord2f( 1, 0 );
		im.Vertex2f( x + w, y );
		im.TexCoord2f( 1, 1 );
		im.Vertex2f( x + w, y + h );
		im.TexCoord2f( 0, 1 );
		im.Vertex2f( x, y + h );
		im.End();
	}

	glFinish();

	end = Sys_Milliseconds();
	common->Printf( "%i msec to draw all images\n", end - start );
}

static const fhRenderMatrix projMatrix2D = fhRenderMatrix::CreateOrthographicMatrix(0, 1, 0, 1, -1, 1);

static void RB_RenderFullscreenPass() {
	fhImmediateMode im(true);
	im.Begin(GL_TRIANGLES);
	im.Color4f(1, 1, 1, 1);

	im.TexCoord2f(-1, 0);
	im.Vertex2f(-1, 0);

	im.TexCoord2f(0.5, 1.5);
	im.Vertex2f(0.5, 1.5);

	im.TexCoord2f(2, 0);
	im.Vertex2f(2, 0);
	im.End();
}

static void RB_BrightnessGamma(fhFramebuffer* src, fhFramebuffer* dst) {
	assert(r_useFramebuffer.GetBool());
	assert(src->GetSamples() <= 1);

	GL_State(GLS_DEFAULT);

	dst->Bind();
	glViewport(0, 0, dst->GetWidth(), dst->GetHeight());
	glScissor(0, 0, dst->GetWidth(), dst->GetHeight());

	GL_UseProgram(brightnessGammaProgram);
	src->GetColorAttachment()->Bind(1);
	fhRenderProgram::SetModelViewMatrix(fhRenderMatrix::identity.ToFloatPtr());
	fhRenderProgram::SetProjectionMatrix(projMatrix2D.ToFloatPtr());
	fhRenderProgram::SetShaderParm(0, idVec4(r_brightness.GetFloat(), r_gamma.GetFloat(), 0, 0));

	RB_RenderFullscreenPass();

	dst->Bind();
}

static fhFramebuffer* RB_Resolve(fhFramebuffer* src) {
	if (src->GetSamples() > 1) {
		auto resolve = fhFramebuffer::currentRenderFramebuffer;
		resolve->Resize(src->GetWidth(), src->GetHeight(), 1, src->GetColorFormat(), src->GetDepthFormat());

		glViewport(0, 0, src->GetWidth(), src->GetHeight());
		glScissor(0, 0, src->GetWidth(), src->GetHeight());
		fhFramebuffer::BlitColor(src, resolve);

		src = resolve;
	}

	return src;
}

static void RB_PostProcess(fhFramebuffer* src, fhFramebuffer* dst) {
	assert(r_useFramebuffer.GetBool());
	const bool enableBloom = r_bloomFactor.GetFloat() > 0.1f;

	src = RB_Resolve(src);

	GL_State(GLS_DEFAULT);

	if (enableBloom) {
		fhFramebuffer::bloomFramebuffer->Bind();
		glViewport(0, 0, fhFramebuffer::bloomFramebuffer->GetWidth(), fhFramebuffer::bloomFramebuffer->GetHeight());
		glScissor(0, 0, fhFramebuffer::bloomFramebuffer->GetWidth(), fhFramebuffer::bloomFramebuffer->GetHeight());

		GL_UseProgram(bloomProgram);
		src->GetColorAttachment()->Bind(1);
		fhRenderProgram::SetModelViewMatrix(fhRenderMatrix::identity.ToFloatPtr());
		fhRenderProgram::SetProjectionMatrix(projMatrix2D.ToFloatPtr());
		idVec4 parm0;
		parm0.x = r_bloomThreshold.GetFloat();
		fhRenderProgram::SetShaderParm(0, parm0);
		RB_RenderFullscreenPass();

		auto blurBloom = [&](){
			fhFramebuffer::bloomTmpFramebuffer->Bind();
			fhFramebuffer::bloomFramebuffer->GetColorAttachment()->Bind(1);
			fhRenderProgram::SetShaderParm(0, idVec4(1.0 / fhFramebuffer::bloomFramebuffer->GetWidth(), 0, 0, 0));
			RB_RenderFullscreenPass();

			fhFramebuffer::bloomFramebuffer->Bind();
			fhFramebuffer::bloomTmpFramebuffer->GetColorAttachment()->Bind(1);
			fhRenderProgram::SetShaderParm(0, idVec4(0, 1.0 / fhFramebuffer::bloomFramebuffer->GetHeight(), 0, 0));
			RB_RenderFullscreenPass();
		};

		GL_UseProgram(blurProgram);
		fhRenderProgram::SetModelViewMatrix(fhRenderMatrix::identity.ToFloatPtr());
		fhRenderProgram::SetProjectionMatrix(projMatrix2D.ToFloatPtr());

		for (int i = 0; i < r_bloomBlurPasses.GetInteger(); ++i) {
			blurBloom();
		}
	}

	dst->Bind();
	glViewport(0, 0, dst->GetWidth(), dst->GetHeight());
	glScissor(0, 0, dst->GetWidth(), dst->GetHeight());

	GL_UseProgram(postprocessProgram);
	src->GetColorAttachment()->Bind(1);
	fhFramebuffer::bloomTmpFramebuffer->GetColorAttachment()->Bind(2);
	fhRenderProgram::SetModelViewMatrix(fhRenderMatrix::identity.ToFloatPtr());
	fhRenderProgram::SetProjectionMatrix(projMatrix2D.ToFloatPtr());

	auto shaderparms = idVec4(0, 0, 0, 0);

	if (r_fxaa.GetBool()) {
		shaderparms.x = src->GetWidth();
		shaderparms.y = src->GetHeight();
	}

	if (enableBloom) {
		shaderparms.z = r_bloomFactor.GetFloat();
	}

	fhRenderProgram::SetShaderParm(0, shaderparms);

	RB_RenderFullscreenPass();

	dst->Bind();
}


/*
=============
RB_SwapBuffers

=============
*/
static void	RB_SwapBuffers(const void *data, fhFramebuffer* src) {
	if (r_useFramebuffer.GetBool()) {
		RB_BrightnessGamma(src, fhFramebuffer::defaultFramebuffer);
	}

	// force a gl sync if requested
	if ( r_finish.GetBool() ) {
		glFinish();
	}

	// texture swapping test
	if (r_showImages.GetInteger() != 0) {
		RB_ShowImages();
	}

    RB_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

    GLimp_SwapBuffers();
}

/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
static void	RB_CopyRender( const void *data, bool copyFromDefaultFramebuffer ) {
	if ( r_skipCopyTexture.GetBool() ) {
		return;
	}

	auto cmd = static_cast<const copyRenderCommand_t*>(data);

    RB_LogComment( "***************** RB_CopyRender *****************\n" );

	if (auto image = cmd->image) {
		//TODO(johl): Can we get rid of RB_CopyRender completely?
		//            If we know in advance we will copy render to a texture, we could render directly into that texture!
		//            CopyRender and CropRender commands are stored in demo files, is that an issue?
		image->AllocateStorage( pixelFormat_t::RGB, cmd->imageWidth, cmd->imageHeight, 1, 1 );

		auto src = fhFramebuffer::GetCurrentDrawBuffer();

		if (src->GetSamples() > 1) {
			//TODO(johl): resolving the full buffer seems like overkill for small subviews (things like security cameras are usually only 512x512)
			auto resolve = fhFramebuffer::currentRenderFramebuffer;
			resolve->Resize( src->GetWidth(), src->GetHeight() );

			glViewport( 0, 0, src->GetWidth(), src->GetHeight() );
			glScissor( 0, 0, src->GetWidth(), src->GetHeight() );
			fhFramebuffer::BlitColor( src, resolve );

			src = resolve;
		}

		fhFramebuffer framebuffer("tmp", cmd->imageWidth, cmd->imageHeight, image, nullptr );
		fhFramebuffer::BlitColor( src, cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight, &framebuffer );
		framebuffer.Purge();
		if ( image == fhFramebuffer::currentRenderFramebuffer->GetColorAttachment() ) // JW : need to do below to clear the mirror hell effect in Mars City 2 so it doesn't leave artifacts in subsequent uses of reflection, imp fireball heat distortion etc.
		{
			fhFramebuffer *currentDrawBuffer = fhFramebuffer::GetCurrentDrawBuffer();
			fhFramebuffer::currentRenderFramebuffer->Bind();
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fhFramebuffer::currentRenderFramebuffer->GetID() );
			int  samples = fhFramebuffer::currentRenderFramebuffer->GetSamples();
			auto target = samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
			glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, fhFramebuffer::currentRenderFramebuffer->GetColorAttachment()->texnum, 0 ); // just reattaching it why
			currentDrawBuffer->Bind();
		}
	}
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
fhFramebuffer* RB_ExecuteBackEndCommands( const emptyCommand_t *cmds ) {
	fhFramebuffer* finalFramebuffer = nullptr;

	// r_debugRenderToTexture
	int	c_draw3d = 0, c_draw2d = 0, c_setBuffers = 0, c_swapBuffers = 0, c_copyRenders = 0;

	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return finalFramebuffer;
	}

	const auto backEndStartTime = Sys_Milliseconds();

	RB_PrintDebugOutput();

	// needed for editor rendering
	RB_SetDefaultGLState();

	// upload any image loads that have completed
	globalImages->CompleteBackgroundImageLoads();

	bool postprocessDone = !r_useFramebuffer.GetBool();

	for ( ; cmds ; cmds = (const emptyCommand_t *)cmds->next ) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW:
			{
				auto cmd = (const drawSurfsCommand_t *)cmds;
				if (cmd->viewDef->viewEntitys) {
					c_draw3d++;
				}
				else {
					c_draw2d++;
					if (!postprocessDone) {
						assert(fhFramebuffer::currentRenderFramebuffer2->GetWidth() == fhFramebuffer::renderFramebuffer->GetWidth());
						assert(fhFramebuffer::currentRenderFramebuffer2->GetHeight() == fhFramebuffer::renderFramebuffer->GetHeight());
						RB_PostProcess(fhFramebuffer::renderFramebuffer, fhFramebuffer::currentRenderFramebuffer2);
						postprocessDone = true;
					}
				}
				RB_DrawView(cmds);
			}
			break;
		case RC_SET_BUFFER:
			RB_SetBuffer( cmds );
			c_setBuffers++;
			break;
		case RC_SWAP_BUFFERS:
			{
				if (c_draw2d > 0) {
					finalFramebuffer = RB_Resolve(postprocessDone ? fhFramebuffer::currentRenderFramebuffer2 : fhFramebuffer::renderFramebuffer);
				}
				else {
					RB_PostProcess(fhFramebuffer::renderFramebuffer, fhFramebuffer::currentRenderFramebuffer2);
					finalFramebuffer = fhFramebuffer::currentRenderFramebuffer2;
				}
			}
			RB_SwapBuffers(cmds, finalFramebuffer);
			c_swapBuffers++;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender(cmds, postprocessDone);
			c_copyRenders++;
			break;
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
	}

	// go back to the default texture so the editor doesn't mess up a bound image
	//glBindTexture( GL_TEXTURE_2D, 0 );
	backEnd.glState.tmu[0].currentTexture = 0;

	// stop rendering on this thread
	const auto backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msec = backEndFinishTime - backEndStartTime;

	if ( r_debugRenderToTexture.GetInteger() == 1 ) {
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, SwpBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_swapBuffers, c_copyRenders, backEnd.c_copyFrameBuffer );
		backEnd.c_copyFrameBuffer = 0;
	}

	return finalFramebuffer;
}
