OBJDIR=obj.win32
CC = gcc

INCLUDE_DIRS=-I include\
	-I src \
	-I src/core \
	-I src/fs \
	-I src/sound \
	-I src/threading 

CFLAGS=$(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/LAL.o \
	$(OBJDIR)/iIntrusivePtr.o \
	$(OBJDIR)/Event.o \
	$(OBJDIR)/FileSystem.o \
	$(OBJDIR)/tinythread.o \
	$(OBJDIR)/Thread.o \
	$(OBJDIR)/WorkerThread.o \
	$(OBJDIR)/libcompress.o \
	$(OBJDIR)/Archive.o \
	$(OBJDIR)/Rendering.o \
	$(OBJDIR)/Wrapper_Windows.o

$(OBJDIR)/LAL.o:
	$(CC) $(CFLAGS) -c src/sound/LAL.cpp -o $(OBJDIR)/LAL.o

$(OBJDIR)/iIntrusivePtr.o:
	$(CC) $(CFLAGS) -c src/core/iIntrusivePtr.cpp -o $(OBJDIR)/iIntrusivePtr.o

$(OBJDIR)/libcompress.o:
	$(CC) $(CFLAGS) -c src/fs/libcompress.c -o $(OBJDIR)/libcompress.o

$(OBJDIR)/Archive.o:
	$(CC) $(CFLAGS) -c src/fs/Archive.cpp -o $(OBJDIR)/Archive.o

$(OBJDIR)/FileSystem.o:
	$(CC) $(CFLAGS) -c src/fs/FileSystem.cpp -o $(OBJDIR)/FileSystem.o

$(OBJDIR)/Event.o:
	$(CC) $(CFLAGS) -c src/threading/Event.cpp -o $(OBJDIR)/Event.o

$(OBJDIR)/Thread.o:
	$(CC) $(CFLAGS) -c src/threading/Thread.cpp -o $(OBJDIR)/Thread.o

$(OBJDIR)/WorkerThread.o:
	$(CC) $(CFLAGS) -c src/threading/WorkerThread.cpp -o $(OBJDIR)/WorkerThread.o

$(OBJDIR)/tinythread.o:
	$(CC) $(CFLAGS) -c src/threading/tinythread.cpp -o $(OBJDIR)/tinythread.o

$(OBJDIR)/Rendering.o:
	$(CC) $(CFLAGS) -c src/Rendering.cpp -o $(OBJDIR)/Rendering.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c src/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++
