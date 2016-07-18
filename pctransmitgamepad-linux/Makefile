TARGET = pc2snes
CC = gcc
CFLAGS = -Wall -O2
LIBS = 

OBJS = $(TARGET).o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

clean:
	rm -f *.o $(TARGET)
	rm -rf tmp

