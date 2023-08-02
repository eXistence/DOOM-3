/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

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

#pragma once

class idImage;
enum class pixelFormat_t;

class fhFramebuffer {
public:
	     fhFramebuffer( const char* name, int w, int h, idImage* color, idImage* depth );

	bool IsDefault() const;
	int  GetWidth() const;
	int  GetHeight() const;
	int  GetSamples() const;
	pixelFormat_t GetColorFormat() const { return colorFormat; }
	pixelFormat_t GetDepthFormat() const { return depthFormat; }

	void Purge();
	void Bind();
	void Resize(int width, int height, int samples = 1) {
		Resize(width, height, samples, colorFormat, depthFormat);
	}
	void Resize(int width, int height, int samples, pixelFormat_t colorFormat, pixelFormat_t depthFormat);

	int GetID() { // JW : for clearing framebuffer effects
		return num; 
	}

	idImage* GetColorAttachment() {
		return colorAttachment;
	}

	idImage* GetDepthAttachment() {
		return depthAttachment;
	}

	static fhFramebuffer* GetCurrentDrawBuffer() {
		if (currentDrawBuffer == nullptr) {
			currentDrawBuffer = defaultFramebuffer;
		}
		return currentDrawBuffer;
	}

	static fhFramebuffer* defaultFramebuffer;
	static fhFramebuffer* renderFramebuffer;
	static fhFramebuffer* shadowmapFramebuffer;
	static fhFramebuffer* currentDepthFramebuffer;
	static fhFramebuffer* currentRenderFramebuffer;
	static fhFramebuffer* currentRenderFramebuffer2;
	static fhFramebuffer* bloomFramebuffer;
	static fhFramebuffer* bloomTmpFramebuffer;

	static void Init();
	static void PurgeAll();
	static void BlitColor( fhFramebuffer* source, fhFramebuffer* dest );
	static void BlitColor( fhFramebuffer* source, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, fhFramebuffer* dest );
	static void BlitDepth( fhFramebuffer* source, fhFramebuffer* dest );

private:
	static fhFramebuffer* currentDrawBuffer;
	static void Blit(
		fhFramebuffer* source, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight,
		fhFramebuffer* dest, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight,
		GLint bufferMask, textureFilter_t filter);

	void SetDrawBuffer();
	void Allocate();

	int      width;
	int      height;
	int      samples;
	pixelFormat_t colorFormat;
	pixelFormat_t depthFormat;
	GLuint   num;
	idImage* colorAttachment;
	idImage* depthAttachment;
	idStr    name;
};

