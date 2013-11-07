#pragma once

#include "Downloader.h"
#include "FileSystem.h"
#include "Event.h"

extern clPtr<clDownloader> g_Downloader;
extern clPtr<iAsyncQueue> g_Events;

extern clPtr<clWorkerThread> g_Loader;

extern clPtr<clFileSystem> g_FS;
