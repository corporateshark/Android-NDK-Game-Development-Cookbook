/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Platform.h"
#include "VecMath.h"
#include "iObject.h"
#include "iIntrusivePtr.h"

#include <vector>
#include <string>

class clVertexAttribs;
class clGLVertexArray;
class clGLSLShaderProgram;
class clGLTexture;
class clTextRenderer;

class clCanvas: public iObject
{
public:
	clCanvas();
	virtual ~clCanvas() {};

	void Rect3D( const LMatrix4& Proj, const LMatrix4& MV,
	             const LVector3& V1,
	             const LVector3& V2,
	             const LVector3& V3,
	             const LVector3& V4,
	             const clPtr<clGLTexture>& Texture,
	             const clPtr<clGLSLShaderProgram>& SP );
	void Rect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color );
	void TexturedRect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color, const clPtr<clGLTexture>& Texture );
	void TexturedRect2DTiled( float X1, float Y1, float X2, float Y2, int TilesX, int TilesY, const LVector4& Color, const clPtr<clGLTexture>& Texture );
	void TexturedRect2DClipped( float X1, float Y1, float X2, float Y2, const LVector4& Color, const clPtr<clGLTexture>& Texture, const LVector4& ClipRect );
	void TextStr( float X1, float Y1, float X2, float Y2, const std::string& Str, int Size, const LVector4& Color, const clPtr<clTextRenderer>& TR, int FontID );

	clPtr<clGLVertexArray> GetFullscreenRect() const { return FRectVA; }

private:
	clPtr<clVertexAttribs> FRect;
	clPtr<clGLVertexArray> FRectVA;
	clPtr<clVertexAttribs> FRect3D;
	clPtr<clGLVertexArray> FRect3DVA;
	clPtr<clGLSLShaderProgram> FRectSP;
	clPtr<clGLSLShaderProgram> FTexRectSP;
	clPtr<clGLSLShaderProgram> FRect3DSP;
};
