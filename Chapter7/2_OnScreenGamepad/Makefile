OBJDIR=obj
CC = gcc

INCLUDE_DIRS=-I include \
	-I src \
	-I src/core \
	-I src/fs \
	-I src/sound \
	-I src/threading 

CFLAGS=-g -std=gnu++0x $(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/Event.o \
	$(OBJDIR)/FileSystem.o \
	$(OBJDIR)/tinythread.o \
	$(OBJDIR)/Thread.o \
	$(OBJDIR)/libcompress.o \
	$(OBJDIR)/Archive.o \
	$(OBJDIR)/Rendering.o \
	$(OBJDIR)/Wrapper_Windows.o \
	$(OBJDIR)/FI_Utils.o \
	$(OBJDIR)/Gestures.o \
	$(OBJDIR)/Multitouch.o \
	$(OBJDIR)/Bitmap.o \
	$(OBJDIR)/ScreenJoystick.o \
	$(OBJDIR)/iIntrusivePtr.o \
	$(OBJDIR)/Engine.o


$(OBJDIR)/Engine.o:
	$(CC) $(CFLAGS) -c src/Engine.cpp -o $(OBJDIR)/Engine.o

$(OBJDIR)/iIntrusivePtr.o:
	$(CC) $(CFLAGS) -c src/core/iIntrusivePtr.cpp -o $(OBJDIR)/iIntrusivePtr.o

$(OBJDIR)/FI_Utils.o:
	$(CC) $(CFLAGS) -c src/FI_Utils.cpp -o $(OBJDIR)/FI_Utils.o

$(OBJDIR)/ScreenJoystick.o:
	$(CC) $(CFLAGS) -c src/ScreenJoystick.cpp -o $(OBJDIR)/ScreenJoystick.o

$(OBJDIR)/Gestures.o:
	$(CC) $(CFLAGS) -c src/Gestures.cpp -o $(OBJDIR)/Gestures.o

$(OBJDIR)/Multitouch.o:
	$(CC) $(CFLAGS) -c src/Multitouch.cpp -o $(OBJDIR)/Multitouch.o

$(OBJDIR)/VecMath.o:
	$(CC) $(CFLAGS) -c src/VecMath.cpp -o $(OBJDIR)/VecMath.o

$(OBJDIR)/Bitmap.o:
	$(CC) $(CFLAGS) -c src/Bitmap.cpp -o $(OBJDIR)/Bitmap.o

$(OBJDIR)/libcompress.o:
	$(CC) -c src/fs/libcompress.c -o $(OBJDIR)/libcompress.o

$(OBJDIR)/Archive.o:
	$(CC) $(CFLAGS) -c src/fs/Archive.cpp -o $(OBJDIR)/Archive.o

$(OBJDIR)/FileSystem.o:
	$(CC) $(CFLAGS) -c src/fs/FileSystem.cpp -o $(OBJDIR)/FileSystem.o

$(OBJDIR)/Event.o:
	$(CC) $(CFLAGS) -c src/threading/Event.cpp -o $(OBJDIR)/Event.o

$(OBJDIR)/Thread.o:
	$(CC) $(CFLAGS) -c src/threading/Thread.cpp -o $(OBJDIR)/Thread.o

$(OBJDIR)/tinythread.o:
	$(CC) $(CFLAGS) -c src/threading/tinythread.cpp -o $(OBJDIR)/tinythread.o

$(OBJDIR)/Rendering.o:
	$(CC) $(CFLAGS) -c src/Rendering.cpp -o $(OBJDIR)/Rendering.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c src/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++
