OBJDIR=.
CC = gcc

INCLUDE_DIRS=\
	-I src \
	-I src/GL \
	-I src/LGL \

CFLAGS=$(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/LGL.o \
	$(OBJDIR)/Wrapper_Windows.o

$(OBJDIR)/LGL.o:
	$(CC) $(CFLAGS) -c src/LGL/LGL.cpp -o $(OBJDIR)/LGL.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c src/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o


all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++
