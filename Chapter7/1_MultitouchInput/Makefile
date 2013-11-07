OBJDIR=obj
CC = gcc

INCLUDE_DIRS=-I include -I src -I src/core -I src/threading 

CFLAGS=-std=gnu++0x $(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/Event.o \
	$(OBJDIR)/tinythread.o \
	$(OBJDIR)/Thread.o \
	$(OBJDIR)/Rendering.o \
	$(OBJDIR)/Wrapper_Windows.o \
	$(OBJDIR)/Multitouch.o \
	$(OBJDIR)/Gestures.o \
	$(OBJDIR)/VecMath.o \

$(OBJDIR)/VecMath.o:
	$(CC) $(CFLAGS) -c src/core/VecMath.cpp -o $(OBJDIR)/VecMath.o

$(OBJDIR)/Gestures.o:
	$(CC) $(CFLAGS) -c src/Gestures.cpp -o $(OBJDIR)/Gestures.o

$(OBJDIR)/Multitouch.o:
	$(CC) $(CFLAGS) -c src/Multitouch.cpp -o $(OBJDIR)/Multitouch.o

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
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++ -g
