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

#include "Engine.h"

#include "GLSL.h"
#include "VertexAttribs.h"
#include "LGL/LGL.h"
#include "LGL/LGLAPI.h"

#include <stdlib.h>

extern sLGLAPI* LGL3;

clGLSLShaderProgram::clGLSLShaderProgram( const std::string& VShader, const std::string& FShader )
	: FVertexShader( VShader )
	, FFragmentShader( FShader )
	, FUniforms()
	, FProgramID( 0 )
	, FVertexShaderID( 0 )
	, FFragmentShaderID( 0 )
{
	RelinkShaderProgram();
}

clGLSLShaderProgram::~clGLSLShaderProgram()
{
	LGL3->glDeleteProgram( FProgramID );

	LGL3->glDeleteShader( FVertexShaderID );
	LGL3->glDeleteShader( FFragmentShaderID );
}

Luint clGLSLShaderProgram::AttachShaderID( Luint Target, const std::string& ShaderCode, Luint OldShaderID )
{
#if defined( USE_OPENGL_3 )
	std::string ShaderStr      = "#version 150 core\n";
#else
	std::string ShaderStr      = "#version 100\n";
	ShaderStr             += "precision highp float;\n";
	ShaderStr             += "#define USE_OPENGL_ES_2\n";
#endif

	std::string ShaderCodeUsed = ShaderCode;

#if !defined( USE_OPENGL_3 )
	// downgrade GLSL 1.50 to GLSL 1.00
	ShaderCodeUsed = Str_ReplaceAllSubStr( ShaderCodeUsed, "texture(", "texture2D(" );

	if ( Target == GL_VERTEX_SHADER )
	{
		ShaderCodeUsed = Str_ReplaceAllSubStr( ShaderCodeUsed, "in ", "attribute " );
		ShaderCodeUsed = Str_ReplaceAllSubStr( ShaderCodeUsed, "out ", "varying " );
	}

	if ( Target == GL_FRAGMENT_SHADER )
	{
		ShaderCodeUsed = Str_ReplaceAllSubStr( ShaderCodeUsed, "out vec4 out_FragColor;", "" );
		ShaderCodeUsed = Str_ReplaceAllSubStr( ShaderCodeUsed, "out_FragColor", "gl_FragColor" );
		ShaderCodeUsed = Str_ReplaceAllSubStr( ShaderCodeUsed, "in ", "varying " );
	}

#endif

	ShaderStr += ShaderCodeUsed;

	Luint Shader = LGL3->glCreateShader( Target );

	const char* Code = ShaderStr.c_str();

	LGL3->glShaderSource( Shader, 1, &Code, NULL );

	LOGI( "Compiling shader for stage: %X\n", Target );

	LGL3->glCompileShader( Shader );

	if ( !CheckStatus( Shader, GL_COMPILE_STATUS, "Shader wasn''t compiled:" ) )
	{
		LGL3->glDeleteShader( Shader );

		return OldShaderID;
	}

	if ( OldShaderID )
	{
		LGL3->glDeleteShader( OldShaderID );
	}

	return Shader;
}

bool clGLSLShaderProgram::CheckStatus( Luint ObjectID, Lenum Target, const std::string& Message ) const
{
	Lint   SuccessFlag = 0;
	Lsizei Length      = 0;
	Lsizei MaxLength   = 0;

	//
	if ( LGL3->glIsProgram( ObjectID ) )
	{
		LGL3->glGetProgramiv( ObjectID, Target, &SuccessFlag );
		LGL3->glGetProgramiv( ObjectID, GL_INFO_LOG_LENGTH, &MaxLength );

		char* Log = ( char* )alloca( MaxLength );

		LGL3->glGetProgramInfoLog( ObjectID, MaxLength, &Length, Log );

		LOGI( "Program info:\n%s\n", Log );
	}
	else if ( LGL3->glIsShader( ObjectID ) )
	{
		LGL3->glGetShaderiv( ObjectID, Target, &SuccessFlag );
		LGL3->glGetShaderiv( ObjectID, GL_INFO_LOG_LENGTH, &MaxLength );

		char* Log = ( char* )alloca( MaxLength );

		LGL3->glGetShaderInfoLog( ObjectID, MaxLength, &Length, Log );

		LOGI( "Shader info:\n%s\n", Log );
	}

	return SuccessFlag != 0;
}

bool clGLSLShaderProgram::RelinkShaderProgram()
{
	Luint ProgramID = LGL3->glCreateProgram();

	FVertexShaderID = AttachShaderID( GL_VERTEX_SHADER, FVertexShader, FVertexShaderID );

	if ( FVertexShaderID ) { LGL3->glAttachShader( ProgramID, FVertexShaderID ); }

	FFragmentShaderID = AttachShaderID( GL_FRAGMENT_SHADER, FFragmentShader, FFragmentShaderID );

	if ( FFragmentShaderID ) { LGL3->glAttachShader( ProgramID, FFragmentShaderID ); }

	BindDefaultLocations( ProgramID );

	LGL3->glLinkProgram( ProgramID );

	if ( !CheckStatus( ProgramID, GL_LINK_STATUS, "Program wasn''t linked\n" ) )
	{
		LOGI( "INTERNAL ERROR: Error while shader relinking\n" );

		return false;
	}

	// replace old program
	LGL3->glDeleteProgram( FProgramID );

	FProgramID = ProgramID;

	RebindAllUniforms();

	return true;
}

void clGLSLShaderProgram::BindDefaultLocations( Luint ProgramID )
{
	// bind default attribs
	LGL3->glBindAttribLocation( ProgramID, L_VS_VERTEX,   "in_Vertex" );      // vec3
	LGL3->glBindAttribLocation( ProgramID, L_VS_TEXCOORD, "in_TexCoord" );    // vec2
	LGL3->glBindAttribLocation( ProgramID, L_VS_NORMAL,   "in_Normal" );      // vec3
	LGL3->glBindAttribLocation( ProgramID, L_VS_COLORS,   "in_Color" );       // vec4

	// bind default fragment program output
	LGL3->glBindFragDataLocation( ProgramID, 0, "out_FragColor" );

	// bind samplers
	LGL3->glUniform1i( LGL3->glGetUniformLocation( FProgramID, "Texture0" ), 0 );
}

void clGLSLShaderProgram::RebindAllUniforms()
{
	Bind();

	FUniforms.clear();

	Lint ActiveUniforms;
	char Buff[256];

	LGL3->glGetProgramiv( FProgramID, GL_ACTIVE_UNIFORMS, &ActiveUniforms );

	for ( int i = 0; i != ActiveUniforms; ++i )
	{
		Lsizei Length;
		Lint   Size;
		Lenum  Type;

		LGL3->glGetActiveUniform( FProgramID, i, sizeof( Buff ), &Length, &Size, &Type, Buff );

		std::string Name( Buff, Length );

		LOGI( "Active uniform: %s at location: %i\n", Name.c_str(), LGL3->glGetUniformLocation( FProgramID, Name.c_str() ) );

		sUniform Uniform( Name );

		Uniform.FLocation = LGL3->glGetUniformLocation( FProgramID, Name.c_str() );

		FUniforms.push_back( Uniform );
	}
}

void clGLSLShaderProgram::SetUniformNameFloat( const std::string& Name, const float Float )
{
	Lint Loc = CreateUniform( Name );

	LGL3->glUniform1f( Loc, Float );
}

void clGLSLShaderProgram::SetUniformNameFloatArray( const std::string& Name, int Count, const float& Float )
{
	Lint Loc = CreateUniform( Name );

	LGL3->glUniform1fv( Loc, Count, &Float );
}

void clGLSLShaderProgram::SetUniformNameVec3Array( const std::string& Name, int Count, const LVector3& Vector )
{
	Lint Loc = CreateUniform( Name );

	LGL3->glUniform3fv( Loc, Count, Vector.ToFloatPtr() );
}

void clGLSLShaderProgram::SetUniformNameVec4Array( const std::string& Name, int Count, const LVector4& Vector )
{
	Lint Loc = CreateUniform( Name );

	LGL3->glUniform4fv( Loc, Count, Vector.ToFloatPtr() );
}

void clGLSLShaderProgram::SetUniformNameMat4Array( const std::string& Name, int Count, const LMatrix4& Matrix )
{
	Lint Loc = CreateUniform( Name );

	LGL3->glUniformMatrix4fv( Loc, Count, false, Matrix.ToFloatPtr() );
}

Lint clGLSLShaderProgram::CreateUniform( const std::string& Name )
{
	for ( size_t i = 0; i != FUniforms.size(); ++i )
	{
		if ( FUniforms[i].FName == Name ) { return FUniforms[i].FLocation; }
	}

	return -1;
}

void clGLSLShaderProgram::Bind()
{
	LGL3->glUseProgram( FProgramID );
}
