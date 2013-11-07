package com.packtpub.ndkcookbook.app7;

import android.app.Activity;
import android.os.Bundle;
import android.view.Surface;

public class App7Activity extends Activity
{
	static
	{
		System.loadLibrary( "App7" );
	}

	public static GLView m_View = null;

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		SetAPKName( getApplication().getApplicationInfo().sourceDir );

		OnCreateNative();

		m_View = new GLView( getApplication() );

		setContentView( m_View );
	}

	public static native void OnCreateNative();
	public static native void SetSurface( Surface surface );
	public static native void SetSurfaceSize( int width, int height );
	public static native void DrawFrame();
	public static native void SetAPKName( String APKName );
};
