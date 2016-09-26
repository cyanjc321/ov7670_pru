
CC=arm-linux-gnueabihf-gcc

CFLAGS=-c
LDFLAGS=

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

TARGET=prucam_test
all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o ${TARGET}
