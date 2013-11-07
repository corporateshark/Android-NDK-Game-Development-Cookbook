OBJDIR=.
CC = gcc

INCLUDE_DIRS=\
	-I src \
	-I src/GL \
	-I src/LGL \

CFLAGS=$(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/LGL.o \

$(OBJDIR)/LGL.o:
	$(CC) $(CFLAGS) -c src/LGL/LGL.cpp -o $(OBJDIR)/LGL.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++
