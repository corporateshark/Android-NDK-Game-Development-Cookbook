package com.packtpub.ndkcookbook.app11;

import android.app.Activity;
import android.os.Environment;
import android.os.Bundle;
import android.util.Log;

import java.io.*;

public class App11Activity extends Activity
{
	static
	{
		System.loadLibrary( "App11" );
	}

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
	}

	public static native void OnCreateNative( String ExternalStorage );
	public static native void SetAPKName( String APKName );
};
