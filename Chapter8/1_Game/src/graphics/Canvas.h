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
