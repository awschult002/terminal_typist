TARGET = terminal_typist
LIBS = -lm -lpthread 
CC = gcc

ifeq ($(shell test -s "/usr/share/dict/american-english" && echo yes), yes)
	DICTIONARY_FILE := -DDICTIONARY_FILE=\"/usr/share/dict/american-english\"
else ifeq ($(shell test -s "/usr/share/dict/words" && echo yes), yes)
	DICTIONARY_FILE := -DDICTIONARY_FILE=\"/usr/share/dict/words\"
else
$(error System dictionary file not found. Please modify the makefile to point to your file.)
endif

CFLAGS = -g3 -Wall --debug $(DICTIONARY_FILE)


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
