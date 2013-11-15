/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "LGL.h"
#include "WinViewport.h"

sLGLAPI* LGL3 = NULL;

void OnDrawFrame()
{
	LGL3->glClearColor( 1.0, 0.0, 0.0, 0.0 );
	LGL3->glClear( GL_COLOR_BUFFER_BIT );
}

LRESULT CALLBACK MyFunc( HWND h, UINT msg, WPARAM w, LPARAM p )
{
	switch ( msg )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
	}

	return DefWindowProc( h, msg, w, p );
}

int main()
{
	WinViewport* MainWindow = new WinViewport( 400, 400, "OpenGL3", "MyWindowClass", &MyFunc, true );

	// This isn't deleted, memory leak.
	LGL3 = new sLGLAPI;

	LGL::clGLExtRetriever* OpenGL = new LGL::clGLExtRetriever;
	OpenGL->Reload( LGL3 );

	const int GLVerMajor = 3;
	const int GLVerMinor = 2;

	WinViewport* TempWindow = new WinViewport( 0, 0, "TempWindow", "TempWindowClass", &DefWindowProc, false );

	// create dummy context to get pointers to GL extensions functions
	GL_CONTEXT_TYPE TempContext  = OpenGL->CreateContextFull( LGL3, TempWindow->GetDeviceContext(), 32, 24, 0, 0, GLVerMajor, GLVerMinor );

	// create the main context
	GL_CONTEXT_TYPE Context  = OpenGL->CreateContextFull( LGL3, MainWindow->GetDeviceContext(), 32, 24, 0, 0, GLVerMajor, GLVerMinor );

	// clean up temp stuff
	OpenGL->DeleteContext( LGL3, TempWindow->GetDeviceContext(), TempContext );
	delete( TempWindow );
	OpenGL->MakeCurrent( LGL3, MainWindow->GetDeviceContext(), Context );
	OpenGL->Reload( LGL3 );

	printf( "OpenGL version: %s\n", LGL3->glGetString( GL_VERSION ) );
	printf( "OpenGL renderer: %s\n", LGL3->glGetString( GL_RENDERER ) );
	printf( "OpenGL vendor: %s\n", LGL3->glGetString( GL_VENDOR ) );

	MSG msg;

	while ( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );

		OnDrawFrame();

		SwapBuffers( MainWindow->GetDeviceContext() );
	}

	delete( MainWindow );
	delete( OpenGL );
	delete( LGL3 );

	return msg.wParam;
}
