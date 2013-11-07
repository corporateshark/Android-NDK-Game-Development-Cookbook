#pragma once

#include "Platform.h"
#include "VecMath.h"

#include <vector>

#include "Core/iObject.h"

class clBitmap;
class clImage;
class VertexAttribs;

struct sUniform
{
public:
	explicit sUniform( const std::string& Name ): FName( Name ), FLocation( -1 ) {};
	sUniform( int Location, const std::string& Name ): FName( Name ), FLocation( Location ) {};
	std::string      FName;
	Lint         FLocation;
};

class clGLSLShaderProgram: public iObject
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

class clGLTexture: public iObject
{
public:
	clGLTexture();
	virtual ~clGLTexture();

	void    Bind( int TextureUnit ) const;
	void    LoadFromBitmap( const clPtr<clBitmap>& Bitmap );
	void    SetImage( const clPtr<clImage>& Image );
	void    SetClamping( Lenum Clamping );

protected:
	void    SetFormat( Lenum Target, Lenum InternalFormat, Lenum Format, int Width, int Height );
	void    CommitChanges();

private:
	Luint         FTexID;
	///
	Lenum         FInternalFormat;
	Lenum         FFormat;
};

class clGLVertexArray: public iObject
{
public:
	clGLVertexArray();
	virtual ~clGLVertexArray();

	void Draw( bool Wireframe ) const;
	void SetVertexAttribs( const clPtr<clVertexAttribs>& Attribs );

private:
	void Bind() const;

private:
	Luint FVBOID;
	Luint FVAOID;

	/// VBO offsets
	std::vector<const void*> FAttribVBOOffset;

	/// pointers to the actual data from clVertexAttribs
	std::vector<const void*> FEnumeratedStreams;

	clPtr<clVertexAttribs> FAttribs;
};
