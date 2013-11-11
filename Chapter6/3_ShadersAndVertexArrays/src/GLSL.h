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

#include <vector>

struct sUniform
{
public:
	explicit sUniform( const std::string& Name ): FName( Name ), FLocation( -1 ) {};
	sUniform( int Location, const std::string& Name ): FName( Name ), FLocation( Location ) {};
	std::string      FName;
	Lint         FLocation;
};

class clGLSLShaderProgram
{
public:
	clGLSLShaderProgram( const std::string& VShader, const std::string& FShader );
	virtual ~clGLSLShaderProgram();

	bool       RelinkShaderProgram();
	void       Bind();
	Lint       CreateUniform( const std::string& Name );

	void       SetUniformNameFloat( const std::string& Name, const float Float );
	void       SetUniformNameFloatArray( const std::string& Name, int Count, const float& Float );
	void       SetUniformNameVec3Array( const std::string& Name, int Count, const LVector3& Vector );
	void       SetUniformNameVec4Array( const std::string& Name, int Count, const LVector4& Vector );
	void       SetUniformNameMat4Array( const std::string& Name, int Count, const LMatrix4& Matrix );

private:
	void     BindDefaultLocations( Luint ProgramID );
	Lint     FindUniformLocation( const std::string& Name );
	Luint    AttachShaderID( Luint Target, const std::string& ShaderCode, Luint OldShaderID );
	bool     CheckStatus( Luint ObjectID, Lenum Target, const std::string& Message ) const;
	void     RebindAllUniforms();
private:
	std::string FVertexShader;
	std::string FFragmentShader;
	Luint FVertexShaderID;
	Luint FFragmentShaderID;

	std::vector<sUniform> FUniforms;

	/// OpenGL shader program ID
	Luint              FProgramID;
	/// OpenGL shader IDs
	std::vector<Luint> FShaderID;
};
