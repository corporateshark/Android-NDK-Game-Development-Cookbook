package com.packtpub.ndkcookbook.game1;

import android.app.Activity;
import android.os.Environment;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.MotionEvent;

import java.io.*;

public class Game1Activity extends Activity
{
	static
	{
		System.loadLibrary( "Game1" );
	}

	public static GLView m_View = null;

	protected String GetDefaultExternalStoragePrefix()
	{
		return Environment.getExternalStorageDirectory().getPath() + "/external_sd/Android/data/" + getApplication().getPackageName();
	}

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		String ExternalStoragePrefix = GetDefaultExternalStoragePrefix();

		SetAPKName( getApplication().getApplicationInfo().sourceDir );

		String state = Environment.getExternalStorageState();

		if ( !Environment.MEDIA_MOUNTED.equals( state ) || Environment.MEDIA_MOUNTED_READ_ONLY.equals( state ) )
		{
			ExternalStoragePrefix = this.getDir( getApplication().getPackageName(), MODE_PRIVATE ).getPath();
		}

		new File( ExternalStoragePrefix + "/cache" ).mkdirs();
		new File( ExternalStoragePrefix + "/files" ).mkdirs();

		// check the storage is writable
		try
		{
			new File( ExternalStoragePrefix + "/cache" ).mkdirs();
			File F = new File( ExternalStoragePrefix + "/cache/engine.log" );
			F.createNewFile();
			F.delete();
		}
		catch ( IOException e )
		{
			Log.e( "App11", "Unable to open log file: falling back to internal storage" );
			ExternalStoragePrefix = this.getDir( getApplication().getPackageName(), MODE_PRIVATE ).getPath();
		}

		OnCreateNative( ExternalStoragePrefix );

		m_View = new GLView( getApplication() );

		setContentView( m_View );
	}

	@Override protected void onPause()
	{
		super.onPause();

		ExitNative();
	}

	@Override public boolean onTouchEvent( MotionEvent event )
	{
		return ProcessTouchEvent( event );
	}

	private static final int L_MOTION_MOVE = 0;
	private static final int L_MOTION_UP   = 1;
	private static final int L_MOTION_DOWN = 2;

	private static final int MOTION_START = -1;
	private static final int MOTION_END   = -2;

	private static final int LK_LBUTTON = 1;

	public static boolean ProcessTouchEvent( MotionEvent event )
	{
		// drop everything
		SendMotion( -1, 0, 0, false, L_MOTION_MOVE );

		int E     = event.getAction() & MotionEvent.ACTION_MASK;
		int nIndex     = event.getActionIndex();
		int nPointerID = event.getPointerId( ( event.getAction() & MotionEvent.ACTION_POINTER_INDEX_MASK ) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT );

		try
		{
			int x = ( int ) event.getX();
			int y = ( int ) event.getY();

			if ( E == MotionEvent.ACTION_DOWN )
			{
				for ( int i = 0; i != event.getPointerCount(); i++ )
				{
					SendMotion( event.getPointerId( i ), ( int )event.getX( i ), ( int )event.getY( i ), true, L_MOTION_DOWN );
				}
			}

			if ( E == MotionEvent.ACTION_UP || E == MotionEvent.ACTION_CANCEL )
			{
				SendMotion( -2, 0, 0, false, L_MOTION_UP );
				return E <= MotionEvent.ACTION_MOVE;
			}

			// secondary pointers go up or down
			int maskedEvent = event.getActionMasked();

			if ( maskedEvent == MotionEvent.ACTION_POINTER_DOWN )
			{
				for ( int i = 0; i != event.getPointerCount(); i++ )
				{
					SendMotion( event.getPointerId( i ), ( int )event.getX( i ), ( int )event.getY( i ), true, L_MOTION_DOWN );
				}
			}

			if ( maskedEvent == MotionEvent.ACTION_POINTER_UP )
			{
				for ( int i = 0; i != event.getPointerCount(); i++ )
				{
					SendMotion( event.getPointerId( i ), ( int )event.getX( i ), ( int )event.getY( i ), i != nPointerID, L_MOTION_UP );
				}

				SendMotion( nPointerID, ( int )event.getX( nPointerID ), ( int )event.getY( nPointerID ), false, L_MOTION_MOVE );
			}

			if ( E == MotionEvent.ACTION_MOVE )
			{
				for ( int i = 0; i != event.getPointerCount(); i++ )
				{
					SendMotion( event.getPointerId( i ), ( int )event.getX( i ), ( int )event.getY( i ), true, L_MOTION_MOVE );
				}
			}

		} // try
		catch ( java.lang.IllegalArgumentException e )
		{
		}

		SendMotion( -2, 0, 0, false, L_MOTION_MOVE );
		return E <= MotionEvent.ACTION_MOVE;
	}

	public static native void OnCreateNative( String ExternalStorage );
	public static native void SetSurface( Surface surface );
	public static native void SetSurfaceSize( int width, int height );
	public static native void DrawFrame();
	public static native void SetAPKName( String APKName );
	public static native void ExitNative();
	public static native void SendMotion( int PointerID, int x, int y, boolean Pressed, int Flag );
};
