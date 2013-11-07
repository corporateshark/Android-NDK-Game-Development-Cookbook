OBJDIR=obj
CC = gcc

INCLUDE_DIRS=-I include -I src -I src/core -I src/threading -I src/fs

CFLAGS=-g -std=gnu++0x $(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/Event.o \
	$(OBJDIR)/tinythread.o \
	$(OBJDIR)/Thread.o \
	$(OBJDIR)/Rendering.o \
	$(OBJDIR)/Wrapper_Windows.o \
	$(OBJDIR)/FileSystem.o \
	$(OBJDIR)/libcompress.o \
	$(OBJDIR)/Archive.o \
	$(OBJDIR)/TextRenderer.o

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

$(OBJDIR)/TextRenderer.o:
	$(CC) $(CFLAGS) -c src/TextRenderer.cpp -o $(OBJDIR)/TextRenderer.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c src/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++
