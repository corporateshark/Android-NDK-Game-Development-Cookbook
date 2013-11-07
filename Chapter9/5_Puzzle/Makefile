OBJDIR=obj
CC = gcc

INCLUDE_DIRS=\
	-I . \
	-I src \
	-I src/game \
	-I src/carousel \
	-I ../Engine/ \
	-I ../Engine/GL \
	-I ../Engine/LGL \
	-I ../Engine/core \
	-I ../Engine/graphics \
	-I ../Engine/fs \
	-I ../Engine/sound \
	-I ../Engine/network \
	-I ../Engine/threading \
	-I ../Engine/include \
	-I ../Engine/include/vorbis \
	-I ../Engine/include/modplug \

CFLAGS=$(INCLUDE_DIRS) -g -std=gnu++0x

OBJS=\
	$(OBJDIR)/LGL.o \
	$(OBJDIR)/Wrapper_Windows.o \
	$(OBJDIR)/iIntrusivePtr.o \
	$(OBJDIR)/Game.o \
	$(OBJDIR)/Geometry.o \
	$(OBJDIR)/VecMath.o \
	$(OBJDIR)/Canvas.o \
	$(OBJDIR)/GLClasses.o \
	$(OBJDIR)/Bitmap.o \
	$(OBJDIR)/FileSystem.o \
	$(OBJDIR)/Archive.o \
	$(OBJDIR)/libcompress.o \
	$(OBJDIR)/FI_Utils.o \
	$(OBJDIR)/Engine.o \
	$(OBJDIR)/Decoders.o \
	$(OBJDIR)/LAL.o \
	$(OBJDIR)/Event.o \
	$(OBJDIR)/Thread.o \
	$(OBJDIR)/tinythread.o \
	$(OBJDIR)/WorkerThread.o \
	$(OBJDIR)/Audio.o \
	$(OBJDIR)/Gestures.o \
	$(OBJDIR)/Multitouch.o \
	$(OBJDIR)/GUI.o \
	$(OBJDIR)/Page_MainMenu.o \
	$(OBJDIR)/TextRenderer.o \
	$(OBJDIR)/FlowFlinger.o \
	$(OBJDIR)/ft_load.o \
	$(OBJDIR)/DownloadTask.o \
	$(OBJDIR)/Downloader.o \
	$(OBJDIR)/Picasa.o \
	$(OBJDIR)/CurlWrap.o \
	$(OBJDIR)/GalleryTable.o \
	$(OBJDIR)/ImageTypes.o \
	$(OBJDIR)/Globals.o

$(OBJDIR)/Globals.o:
	$(CC) $(CFLAGS) -c src/game/Globals.cpp -o $(OBJDIR)/Globals.o

$(OBJDIR)/GalleryTable.o:
	$(CC) $(CFLAGS) -c src/game/GalleryTable.cpp -o $(OBJDIR)/GalleryTable.o

$(OBJDIR)/ImageTypes.o:
	$(CC) $(CFLAGS) -c src/game/ImageTypes.cpp -o $(OBJDIR)/ImageTypes.o

$(OBJDIR)/Page_MainMenu.o:
	$(CC) $(CFLAGS) -c src/game/Page_MainMenu.cpp -o $(OBJDIR)/Page_MainMenu.o

$(OBJDIR)/FlowFlinger.o:
	$(CC) $(CFLAGS) -c src/carousel/FlowFlinger.cpp -o $(OBJDIR)/FlowFlinger.o

$(OBJDIR)/Game.o:
	$(CC) $(CFLAGS) -c src/game/Game.cpp -o $(OBJDIR)/Game.o

$(OBJDIR)/ft_load.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/ft_load.cpp -o $(OBJDIR)/ft_load.o

$(OBJDIR)/TextRenderer.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/TextRenderer.cpp -o $(OBJDIR)/TextRenderer.o

$(OBJDIR)/Gestures.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/Gestures.cpp -o $(OBJDIR)/Gestures.o

$(OBJDIR)/Multitouch.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/Multitouch.cpp -o $(OBJDIR)/Multitouch.o

$(OBJDIR)/Audio.o:
	$(CC) $(CFLAGS) -c ../Engine/sound/Audio.cpp -o $(OBJDIR)/Audio.o

$(OBJDIR)/DownloadTask.o:
	$(CC) $(CFLAGS) -c ../Engine/network/DownloadTask.cpp -o $(OBJDIR)/DownloadTask.o

$(OBJDIR)/Downloader.o:
	$(CC) $(CFLAGS) -c ../Engine/network/Downloader.cpp -o $(OBJDIR)/Downloader.o

$(OBJDIR)/Picasa.o:
	$(CC) $(CFLAGS) -c ../Engine/network/Picasa.cpp -o $(OBJDIR)/Picasa.o

$(OBJDIR)/CurlWrap.o:
	$(CC) $(CFLAGS) -c ../Engine/network/CurlWrap.cpp -o $(OBJDIR)/CurlWrap.o

$(OBJDIR)/WorkerThread.o:
	$(CC) $(CFLAGS) -c ../Engine/threading/WorkerThread.cpp -o $(OBJDIR)/WorkerThread.o

$(OBJDIR)/tinythread.o:
	$(CC) $(CFLAGS) -c ../Engine/threading/tinythread.cpp -o $(OBJDIR)/tinythread.o

$(OBJDIR)/Thread.o:
	$(CC) $(CFLAGS) -c ../Engine/threading/Thread.cpp -o $(OBJDIR)/Thread.o

$(OBJDIR)/Event.o:
	$(CC) $(CFLAGS) -c ../Engine/threading/Event.cpp -o $(OBJDIR)/Event.o

$(OBJDIR)/LAL.o:
	$(CC) $(CFLAGS) -c ../Engine/sound/LAL.cpp -o $(OBJDIR)/LAL.o

$(OBJDIR)/Decoders.o:
	$(CC) $(CFLAGS) -c ../Engine/sound/Decoders.cpp -o $(OBJDIR)/Decoders.o

$(OBJDIR)/Engine.o:
	$(CC) $(CFLAGS) -c ../Engine/Engine.cpp -o $(OBJDIR)/Engine.o

$(OBJDIR)/FI_Utils.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/FI_Utils.cpp -o $(OBJDIR)/FI_Utils.o

$(OBJDIR)/VecMath.o:
	$(CC) $(CFLAGS) -c ../Engine/core/VecMath.cpp -o $(OBJDIR)/VecMath.o

$(OBJDIR)/Geometry.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/Geometry.cpp -o $(OBJDIR)/Geometry.o

$(OBJDIR)/iIntrusivePtr.o:
	$(CC) $(CFLAGS) -c ../Engine/core/iIntrusivePtr.cpp -o $(OBJDIR)/iIntrusivePtr.o

$(OBJDIR)/GUI.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/GUI.cpp -o $(OBJDIR)/GUI.o

$(OBJDIR)/LGL.o:
	$(CC) $(CFLAGS) -c ../Engine/LGL/LGL.cpp -o $(OBJDIR)/LGL.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c ../Engine/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o

$(OBJDIR)/Canvas.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/Canvas.cpp -o $(OBJDIR)/Canvas.o

$(OBJDIR)/GLClasses.o:
	$(CC) $(CFLAGS) -c ../Engine/LGL/GLClasses.cpp -o $(OBJDIR)/GLClasses.o

$(OBJDIR)/Bitmap.o:
	$(CC) $(CFLAGS) -c ../Engine/graphics/Bitmap.cpp -o $(OBJDIR)/Bitmap.o

$(OBJDIR)/FileSystem.o:
	$(CC) $(CFLAGS) -c ../Engine/fs/FileSystem.cpp -o $(OBJDIR)/FileSystem.o

$(OBJDIR)/Archive.o:
	$(CC) $(CFLAGS) -c ../Engine/fs/Archive.cpp -o $(OBJDIR)/Archive.o

$(OBJDIR)/libcompress.o:
	$(CC) -c ../Engine/fs/libcompress.c -o $(OBJDIR)/libcompress.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++ -g
