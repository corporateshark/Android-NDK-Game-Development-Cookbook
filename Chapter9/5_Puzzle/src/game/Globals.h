#pragma once

#include "GLClasses.h"
#include "LGL.h"

#include "Downloader.h"
#include "FileSystem.h"
#include "Event.h"

#include "GalleryTable.h"
#include "FlowUI.h"
#include "GUI.h"
#include "Canvas.h"
#include "Game.h"

extern clPuzzle g_Game;

extern sLGLAPI* LGL3;

extern clPtr<clDownloader> g_Downloader;
extern clPtr<iAsyncQueue> g_Events;

extern clPtr<clGLTexture> g_Texture;

extern bool g_InGallery;

extern clPtr<clFlowUI> g_Flow;

extern clPtr<clGUI> g_GUI;

extern clPtr<clGallery> g_Gallery;

extern clPtr<clWorkerThread> g_Loader;

extern clPtr<clFileSystem> g_FS;

extern clPtr<clTextRenderer > g_TextRenderer;

extern int g_Font;

extern clPtr<clCanvas> g_Canvas;
