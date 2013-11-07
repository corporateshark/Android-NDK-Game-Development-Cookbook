#pragma once

#include "Platform.h"

#include "iObject.h"
#include "iIntrusivePtr.h"

#include "LGLAPI.h"

#include <vector>

class clVertexAttribs;

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
