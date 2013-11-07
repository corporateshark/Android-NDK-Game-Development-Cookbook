#pragma once

#include "iIntrusivePtr.h"

class clBitmap;
class iIStream;

/// Unload FreeImage library
void FreeImage_Unload();

/// Load image from stream into the resource with optional vertical flip
bool FreeImage_LoadFromStream( clPtr<iIStream> IStream, const clPtr<clBitmap>& Resource, bool DoFlipV );
