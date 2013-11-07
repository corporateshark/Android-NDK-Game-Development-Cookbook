#pragma once

#include "Globals.h"
#include "GLClasses.h"
#include "Canvas.h"
#include "FlowUI.h"
#include "ImageTypes.h"

class clPage_Gallery: public clGUIPage
{
public:
	clPage_Gallery()
	{
	}

	virtual void Render()
	{
		RenderDirect( g_Flow );
	}

	virtual void OnActivation() { g_InGallery = true; }
	virtual void OnDeactivation() { g_InGallery = false; }

	virtual void Update( float DT )
	{
		g_Flow->FFlinger->Update( DT );
	}

private:
	void RenderDirect( clPtr<clFlowUI> Control )
	{
		LGL3->glClearColor( 0.3f, 0.0f, 0.0f, 0.0f );
		LGL3->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		int Num = Control->FNumImg;
		int CurImg = Control->GetCurrentImage();
		float Dist = ( float )( Num * OneImageSize );

		if ( Num < 1 ) { return; }

		// index = [curr - 2 .. curr + 2]
		/// Left -> Right -> Selected rendering order
		int ImgOrder[] = { CurImg - 3, CurImg - 2, CurImg - 1, CurImg + 3, CurImg + 2, CurImg + 1, CurImg };

		for ( int in_i = 0 ; in_i < 7 ; in_i++ )
		{
			int i = ImgOrder[in_i];

			if ( i < 0 ) { i += ( 1 - ( ( int )( i / Num ) ) ) * Num; }

			if ( i >= Num ) { i -= ( ( int )( i / Num ) ) * Num; }

			if ( i < Num && i > -1 )
			{
				vec3 Pt[4];
				Control->QuadCoords( Pt, Control->FFlinger->FValue - ( float )( i ) * OneImageSize );

				clPtr<sImageDescriptor> Img = g_Gallery->GetImage( i );
				clPtr<clGLTexture> Txt = Img ? Img->FTexture : g_Texture;
				g_Canvas->Rect3D( Control->FProjection, Control->FView, Pt[1], Pt[0], Pt[3], Pt[2], Txt, NULL );
			}
		}
	}
};
