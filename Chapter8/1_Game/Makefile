OBJDIR=obj
CC = gcc

INCLUDE_DIRS=\
	-I src \
	-I src/GL \
	-I src/LGL \
	-I src/core \
	-I src/graphics \
	-I src/game \
	-I src/fs \
	-I src/sound \
	-I src/threading \
	-I src/include \
	-I src/include/vorbis \
	-I src/include/modplug \

CFLAGS=$(INCLUDE_DIRS) -g -std=gnu++11

OBJS=\
	$(OBJDIR)/LGL.o \
	$(OBJDIR)/Wrapper_Windows.o \
	$(OBJDIR)/iIntrusivePtr.o \
	$(OBJDIR)/Geometry.o \
	$(OBJDIR)/VecMath.o \
	$(OBJDIR)/Canvas.o \
	$(OBJDIR)/GLClasses.o \
	$(OBJDIR)/Bitmap.o \
	$(OBJDIR)/FileSystem.o \
	$(OBJDIR)/Archive.o \
	$(OBJDIR)/libcompress.o \
	$(OBJDIR)/Globals.o \
	$(OBJDIR)/Pentomino.o \
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
	$(OBJDIR)/TextRenderer.o \
	$(OBJDIR)/ft_load.o \

$(OBJDIR)/ft_load.o:
	$(CC) $(CFLAGS) -c src/graphics/ft_load.cpp -o $(OBJDIR)/ft_load.o

$(OBJDIR)/TextRenderer.o:
	$(CC) $(CFLAGS) -c src/graphics/TextRenderer.cpp -o $(OBJDIR)/TextRenderer.o

$(OBJDIR)/Gestures.o:
	$(CC) $(CFLAGS) -c src/graphics/Gestures.cpp -o $(OBJDIR)/Gestures.o

$(OBJDIR)/Multitouch.o:
	$(CC) $(CFLAGS) -c src/graphics/Multitouch.cpp -o $(OBJDIR)/Multitouch.o

$(OBJDIR)/Audio.o:
	$(CC) $(CFLAGS) -c src/sound/Audio.cpp -o $(OBJDIR)/Audio.o

$(OBJDIR)/WorkerThread.o:
	$(CC) $(CFLAGS) -c src/threading/WorkerThread.cpp -o $(OBJDIR)/WorkerThread.o

$(OBJDIR)/tinythread.o:
	$(CC) $(CFLAGS) -c src/threading/tinythread.cpp -o $(OBJDIR)/tinythread.o

$(OBJDIR)/Thread.o:
	$(CC) $(CFLAGS) -c src/threading/Thread.cpp -o $(OBJDIR)/Thread.o

$(OBJDIR)/Event.o:
	$(CC) $(CFLAGS) -c src/threading/Event.cpp -o $(OBJDIR)/Event.o

$(OBJDIR)/LAL.o:
	$(CC) $(CFLAGS) -c src/sound/LAL.cpp -o $(OBJDIR)/LAL.o

$(OBJDIR)/Decoders.o:
	$(CC) $(CFLAGS) -c src/sound/Decoders.cpp -o $(OBJDIR)/Decoders.o

$(OBJDIR)/Engine.o:
	$(CC) $(CFLAGS) -c src/Engine.cpp -o $(OBJDIR)/Engine.o

$(OBJDIR)/FI_Utils.o:
	$(CC) $(CFLAGS) -c src/graphics/FI_Utils.cpp -o $(OBJDIR)/FI_Utils.o

$(OBJDIR)/Globals.o:
	$(CC) $(CFLAGS) -c src/game/Globals.cpp -o $(OBJDIR)/Globals.o

$(OBJDIR)/Pentomino.o:
	$(CC) $(CFLAGS) -c src/game/Pentomino.cpp -o $(OBJDIR)/Pentomino.o

$(OBJDIR)/VecMath.o:
	$(CC) $(CFLAGS) -c src/core/VecMath.cpp -o $(OBJDIR)/VecMath.o

$(OBJDIR)/Geometry.o:
	$(CC) $(CFLAGS) -c src/graphics/Geometry.cpp -o $(OBJDIR)/Geometry.o

$(OBJDIR)/iIntrusivePtr.o:
	$(CC) $(CFLAGS) -c src/core/iIntrusivePtr.cpp -o $(OBJDIR)/iIntrusivePtr.o

$(OBJDIR)/LGL.o:
	$(CC) $(CFLAGS) -c src/LGL/LGL.cpp -o $(OBJDIR)/LGL.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c src/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o

$(OBJDIR)/Canvas.o:
	$(CC) $(CFLAGS) -c src/graphics/Canvas.cpp -o $(OBJDIR)/Canvas.o

$(OBJDIR)/GLClasses.o:
	$(CC) $(CFLAGS) -c src/LGL/GLClasses.cpp -o $(OBJDIR)/GLClasses.o

$(OBJDIR)/Bitmap.o:
	$(CC) $(CFLAGS) -c src/graphics/Bitmap.cpp -o $(OBJDIR)/Bitmap.o

$(OBJDIR)/FileSystem.o:
	$(CC) $(CFLAGS) -c src/fs/FileSystem.cpp -o $(OBJDIR)/FileSystem.o

$(OBJDIR)/Archive.o:
	$(CC) $(CFLAGS) -c src/fs/Archive.cpp -o $(OBJDIR)/Archive.o

$(OBJDIR)/libcompress.o:
	$(CC) -c src/fs/libcompress.c -o $(OBJDIR)/libcompress.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++ -g
