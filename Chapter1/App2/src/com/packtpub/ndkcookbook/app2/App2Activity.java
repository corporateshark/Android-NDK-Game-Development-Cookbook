package com.packtpub.ndkcookbook.app2;

import android.app.Activity;
import android.os.Bundle;

public class App2Activity extends Activity
{
	static
	{
		System.loadLibrary( "App2" );
	}

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		onCreateNative();
	}

	public static native void onCreateNative();
};
