TARGET = terminal_typist
LIBS = -lm -lpthread 
CC = gcc
CFLAGS = -g3 -Wall --debug

.PHONY: default all clean

default: $(TARGET)

run: $(TARGET)
	exec ./$(TARGET)

all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
