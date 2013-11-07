package com.packtpub.ndkcookbook.app14;

import android.app.Activity;
import android.os.Bundle;
import android.view.Surface;
import android.view.MotionEvent;
import android.view.KeyEvent;

import java.util.Locale;

public class App14Activity extends Activity
{
	static
	{
		System.loadLibrary( "App14" );
	}

	public static GLView m_View = null;

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		SetAPKName( getApplication().getApplicationInfo().sourceDir );
		SetLocaleName( Locale.getDefault().getLanguage() );

		OnCreateNative( "" );

		m_View = new GLView( getApplication() );

		setContentView( m_View );
	}

	public static native void OnCreateNative( String ExternalStorage );
	public static native void SetSurface( Surface surface );
	public static native void SetSurfaceSize( int width, int height );
	public static native void DrawFrame();
	public static native void SetAPKName( String APKName );
	public static native void SetLocaleName( String LocaleName );
};
