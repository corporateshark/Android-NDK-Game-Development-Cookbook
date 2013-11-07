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
	$(OBJDIR)/VecMath.o \
	$(OBJDIR)/Canvas.o \
	$(OBJDIR)/GLTexture.o \
	$(OBJDIR)/Bitmap.o \
	$(OBJDIR)/FileSystem.o \
	$(OBJDIR)/Archive.o \
	$(OBJDIR)/libcompress.o \

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

$(OBJDIR)/Canvas.o:
	$(CC) $(CFLAGS) -c src/Canvas.cpp -o $(OBJDIR)/Canvas.o

$(OBJDIR)/GLTexture.o:
	$(CC) $(CFLAGS) -c src/GLTexture.cpp -o $(OBJDIR)/GLTexture.o

$(OBJDIR)/Bitmap.o:
	$(CC) $(CFLAGS) -c src/Bitmap.cpp -o $(OBJDIR)/Bitmap.o

$(OBJDIR)/FileSystem.o:
	$(CC) $(CFLAGS) -c src/fs/FileSystem.cpp -o $(OBJDIR)/FileSystem.o

$(OBJDIR)/Archive.o:
	$(CC) $(CFLAGS) -c src/fs/Archive.cpp -o $(OBJDIR)/Archive.o

$(OBJDIR)/libcompress.o:
	$(CC) $(CFLAGS) -c src/fs/libcompress.c -o $(OBJDIR)/libcompress.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o main.exe main.cpp $(OBJS) -lgdi32 -lstdc++ -g
