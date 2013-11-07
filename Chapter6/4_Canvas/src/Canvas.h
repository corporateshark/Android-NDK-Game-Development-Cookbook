#pragma once

#include "Platform.h"
#include "VecMath.h"
#include "iIntrusivePtr.h"

#include <vector>

class clVertexAttribs;
class clGLVertexArray;
class clGLSLShaderProgram;
class clGLTexture;

class clCanvas
{
public:
	clCanvas();
	virtual ~clCanvas() {};

	void Rect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color );
	void TexturedRect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color, const clPtr<clGLTexture>& Texture );

private:
	clPtr<clVertexAttribs> FRect;
	clPtr<clGLVertexArray> FRectVA;
	clPtr<clGLSLShaderProgram> FRectSP;
	clPtr<clGLSLShaderProgram> FTexRectSP;
};
