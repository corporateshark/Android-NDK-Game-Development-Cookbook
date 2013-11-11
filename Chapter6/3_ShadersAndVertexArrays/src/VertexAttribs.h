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
#include "fs/Streams.h"

#include <vector>

// vertex streams mapping (used as vertex shader inputs)
const int L_VS_VERTEX   = 0; // vec3
const int L_VS_TEXCOORD = 1; // vec2
const int L_VS_NORMAL   = 2; // vec3
const int L_VS_COLORS   = 3; // vec4

const int L_VS_TOTAL_ATTRIBS = L_VS_COLORS + 1;

/// number of float components in every stream
const int L_VS_VEC_COMPONENTS[ L_VS_TOTAL_ATTRIBS ] = { 3, 2, 3, 4 };

/// Container for vertex attribs (think about it like a mesh without internal hierarchy)
class clVertexAttribs: public iObject
{
public:
	clVertexAttribs();
	clVertexAttribs( size_t Vertices );

	void   SetActiveVertexCount( size_t Count ) { FActiveVertexCount = Count; }
	size_t GetActiveVertexCount() const { return FActiveVertexCount; }

	/// map all vertex attribs (vertices, texcoords, normals, etc) to L_VS_TOTAL_ATTRIBS vertex streams
	const std::vector<const void*>& EnumerateVertexStreams() const;

	void Restart( size_t ReserveVertices );
	void EmitVertexV( const LVector3& Vec );
	void EmitVertex( float X, float Y, float Z ) { EmitVertexV( LVector3( X, Y, Z ) ); };
	void SetTexCoord( float U, float V, float W ) { SetTexCoordV( LVector2( U, V ) ); };
	void SetTexCoordV( const LVector2& V );
	void SetNormalV( const LVector3& Vec );
	void SetColorV( const LVector4& Vec );

public:

	/// vertex position X, Y, Z
	std::vector<LVector3> FVertices;

	/// vertex tex coord U, V
	std::vector<LVector2> FTexCoords;

	/// vertex normal in object space
	std::vector<LVector3> FNormals;

	/// vertex RGBA colors
	std::vector<LVector4> FColors;

private:
	/// current number of acive vertices used for rendering
	size_t FActiveVertexCount;

	/// per-vertex generation
	LVector2 FCurrentTexCoords;
	LVector3 FCurrentNormal;
	LVector4 FCurrentColor;
	/// precached vertex streams
	mutable std::vector<const void*> FStreams;
};
