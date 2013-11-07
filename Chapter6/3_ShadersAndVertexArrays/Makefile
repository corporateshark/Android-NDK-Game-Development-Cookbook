OBJDIR=obj
CC = gcc

INCLUDE_DIRS=\
	-I src \
	-I src/GL \
	-I src/LGL \
	-I src/core \
	-I src/fs \

CFLAGS=$(INCLUDE_DIRS)

OBJS=\
	$(OBJDIR)/LGL.o \
	$(OBJDIR)/Wrapper_Windows.o \
	$(OBJDIR)/VertexAttribs.o \
	$(OBJDIR)/iIntrusivePtr.o \
	$(OBJDIR)/GLVertexArray.o \
	$(OBJDIR)/GLSL.o \
	$(OBJDIR)/GeomServ.o \
	$(OBJDIR)/VecMath.o

$(OBJDIR)/VecMath.o:
	$(CC) $(CFLAGS) -c src/core/VecMath.cpp -o $(OBJDIR)/VecMath.o

$(OBJDIR)/GeomServ.o:
	$(CC) $(CFLAGS) -c src/GeomServ.cpp -o $(OBJDIR)/GeomServ.o

$(OBJDIR)/iIntrusivePtr.o:
	$(CC) $(CFLAGS) -c src/core/iIntrusivePtr.cpp -o $(OBJDIR)/iIntrusivePtr.o

$(OBJDIR)/VertexAttribs.o:
	$(CC) $(CFLAGS) -c src/VertexAttribs.cpp -o $(OBJDIR)/VertexAttribs.o

$(OBJDIR)/GLVertexArray.o:
	$(CC) $(CFLAGS) -c src/GLVertexArray.cpp -o $(OBJDIR)/GLVertexArray.o

$(OBJDIR)/GLSL.o:
	$(CC) $(CFLAGS) -c src/GLSL.cpp -o $(OBJDIR)/GLSL.o

$(OBJDIR)/LGL.o:
	$(CC) $(CFLAGS) -c src/LGL/LGL.cpp -o $(OBJDIR)/LGL.o

$(OBJDIR)/Wrapper_Windows.o:
	$(CC) $(CFLAGS) -c src/Wrapper_Windows.cpp -o $(OBJDIR)/Wrapper_Windows.o


all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++
