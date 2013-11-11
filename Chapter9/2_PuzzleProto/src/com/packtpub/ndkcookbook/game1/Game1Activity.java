/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
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
