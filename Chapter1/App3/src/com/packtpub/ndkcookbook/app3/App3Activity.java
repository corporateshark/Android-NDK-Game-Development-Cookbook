package com.packtpub.ndkcookbook.app3;

import android.app.Activity;
import android.os.Bundle;
import android.view.Surface;

public class App3Activity extends Activity
{
	static
	{
		System.loadLibrary( "App3" );
	}

	public static GLView m_View = null;

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		OnCreateNative();

		m_View = new GLView( getApplication() );

		setContentView( m_View );
	}

	public static native void OnCreateNative();
	public static native void SetSurface( Surface surface );
	public static native void SetSurfaceSize( int width, int height );
	public static native void DrawFrame();
};
