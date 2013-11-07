/**
 * \file GLSL.h
 * \brief OpenGL Shading Language
 * \version 0.6.00
 * \date 09/05/2011
 * \author Sergey Kosarevsky, 2005-2011
 * \author support@linderdaum.com http://www.linderdaum.com
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
